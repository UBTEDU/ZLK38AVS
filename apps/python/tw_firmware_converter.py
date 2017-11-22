#!/usr/bin/env python

# Functions designed for programmatic interface:
# fw_bin = GetFirmwareBinFile(in_path, fw_opn, block_size = 16)
#     in_path: input path
#     Returns a buffer containing an SDK compatible firmware
#
# fw_bin = GetFirmwareBinFileB(in_file, fw_opn, block_size = 16)
#     in_file: input file in a buffer
#     Returns a buffer containing an SDK compatible firmware

import sys
import os
import re
import struct
import argparse
import time
import ntpath
from array import array

# Constants
IMG_HDR_VERSION = 0
IMG_HDR_FORMAT = 0
HBI_MAX_PAGE_LEN = 256

# Globals
page_select = True
left_over_bytes = 0
programmatic = True

# ****************************************************************************
def ParseFile(path):
    with open(path, "r") as f:
        buf = f.read()

    return buf

# ****************************************************************************
def ParseS3Segment(list, seg_addr, seg_len):
    global programmatic

    found = False
    seg_buffer = []
    seg_counter = 0
    for line in list:
        if (line[0: 2].lower() != "s3"):
            # Skip non S3 lines
            continue

        # Get the address (faster than parsing the full line)
        addr = int(line[4: 12], 16)
        if (addr == seg_addr):
            # Found the segment
            found = True
            # Variable used to find gaps
            addr_expected = addr

        if found:
            # Parse the line in a list of bytes
            split_line = re.findall(r"[0-9a-fA-F]{2}", line[2:])
            split_line = map(lambda x: int(x, 16), split_line)

            # Payload (in between the address and the checksum)
            payload_byte_list = split_line[5: -1]

            # Check the checksum
            checksum = (sum(split_line) + 1) & 0xFF
            if checksum:
                raise ValueError("Error - ParseS3Segment(): S3 checksum mismatch")

            # Payload size in bytes
            nb_bytes = len(payload_byte_list)

            # Check for gaps (missing bytes from the previous line)
            gap = addr - addr_expected
            if (gap > 0):
                # Add 0s to fill the gap
                seg_buffer += [0] * gap
                if not programmatic:
                    print ("Info - Found a gap of %d bytes at address 0x%08X" % (gap, addr - gap))

            # Append the current payload
            seg_buffer += payload_byte_list
            seg_counter += gap + nb_bytes

            # Update the expected address
            addr_expected += gap + nb_bytes

            if (seg_counter == seg_len):
                # Parsing done
                break

    return seg_buffer

# ****************************************************************************
def DecodeTable(header):
    seg_list = []
    # To easily match the data in the header with the register addresses in the FW manual,
    # apply an offset of 0x128 (address of the first register in the header)
    offset = 0x128

    # The S3 starts with the configuration record
    seg_list.append([0x00080200, header[(0x1F0 - offset) / 2]])

    # Add all the firmware segments
    num_seg = header[(0x13E - offset) / 2]
    for i in xrange(num_seg):
        seg_size = (header[((0x140 + i * 8) - offset) / 2] << 16) + header[((0x142 + i * 8) - offset) / 2]
        seg_size &= 0x00FFFFFF
        seg_addr = (header[((0x144 + i * 8) - offset) / 2] << 16) + header[((0x146 + i * 8) - offset) / 2]
        seg_list.append([seg_addr, seg_size])

    return seg_list

# ****************************************************************************
def FormatSegmentToHbi(segment_list, start_address, block_size_words):
    global page_select
    global left_over_bytes

    # Initial data
    addr = start_address
    seg_len_bytes = len(segment_list)
    seg_index = 0
    temp_list_bytes = []
    rem_page_bytes = 0
    debug_list = []

    if (left_over_bytes > 0):
        block_size_bytes = left_over_bytes
    else:
        block_size_bytes = block_size_words * 2

    while (seg_index < seg_len_bytes):
        # HBI block with start address
        if (rem_page_bytes == 0):
            rem_page_bytes = HBI_MAX_PAGE_LEN * 2

            # Page 255 selection plus base address
            if page_select:
                block_list_bytes = [0xFE, 0xFF, 0x86, 0x81]
            else:
                block_list_bytes = [0x86, 0x81]
            block_list_bytes.append((addr >> 24) & 0xFF)
            block_list_bytes.append((addr >> 16) & 0xFF)
            block_list_bytes.append((addr >> 8) & 0xFF)
            block_list_bytes.append(0x00)

            # Paged offset access
            offset_words = (addr & 0xFF) / 2
            block_list_bytes.append(offset_words)
            rem_page_bytes -= offset_words * 2

            # Check if we have enough data to fill the block, otherwise add NOP to pad (account for the 1 byte length)
            block_size_bytes -= len(block_list_bytes) + 1
            rem_seg_bytes = seg_len_bytes - seg_index
            if (block_size_bytes > rem_seg_bytes):
                pad_bytes = block_size_bytes - rem_seg_bytes
                block_size_bytes = rem_seg_bytes
            else:
                pad_bytes = 0

            # Append the length with the Write (0x80)
            block_list_bytes.append((block_size_bytes / 2 - 1) + 0x80)

            # Append the data
            block_list_bytes += segment_list[seg_index: seg_index + block_size_bytes]

            # End of the block (Leave the room for the begining of the next segment)
            if (page_select and (pad_bytes >= 12)):
                left_over_bytes = pad_bytes
            elif (pad_bytes >= 10):
                left_over_bytes = pad_bytes
            else:
                # Not enough room, fill with NOP (if any)
                left_over_bytes = 0
                block_list_bytes += [0xFF] * pad_bytes

            # Update the indexes and current address
            seg_index += block_size_bytes
            rem_page_bytes -= block_size_bytes
            addr += block_size_bytes
            page_select = False

        # HBI Continue type block
        elif (rem_page_bytes >= (block_size_bytes - 2)):
            block_list_bytes = [0xFB]

            # Check if we have enough data to fill the block (account for the 2 bytes of HBI control)
            block_size_bytes -= 2
            rem_seg_bytes = seg_len_bytes - seg_index
            if (block_size_bytes > rem_seg_bytes):
                pad_bytes = block_size_bytes - rem_seg_bytes
                block_size_bytes = rem_seg_bytes
            else:
                pad_bytes = 0

            # Append the length
            block_list_bytes.append(block_size_bytes / 2 - 1)

            # Append the data
            block_list_bytes += segment_list[seg_index: seg_index + block_size_bytes]

            # End of the block (Leave the room for the begining of the next segment)
            if (pad_bytes >= 10):
                left_over_bytes = pad_bytes
            else:
                # Not enough room, fill with NOP (if any)
                left_over_bytes = 0
                block_list_bytes += [0xFF] * pad_bytes

            # Update the indexes and current address
            seg_index += block_size_bytes
            rem_page_bytes -= block_size_bytes
            addr += block_size_bytes

        # HBI hybrid block, page remaining space is too small for a full block
        else:
            # Remaining bytes to process in the segment
            rem_seg_bytes = seg_len_bytes - seg_index

            # Check if the rest of the segment fits in the current page
            if (rem_page_bytes >= rem_seg_bytes):
                continue_bytes = rem_seg_bytes
            else:
                continue_bytes = rem_page_bytes

            # Write the Continue partition
            block_list_bytes = [0xFB]
            block_list_bytes.append(continue_bytes / 2 - 1)

            # Append the data
            block_list_bytes += segment_list[seg_index: seg_index + continue_bytes]

            # Update the indexes and current address
            seg_index += continue_bytes
            rem_page_bytes = HBI_MAX_PAGE_LEN * 2
            addr += continue_bytes

            # Space left accounting for the 2 bytes of HBI control
            block_size_bytes -= continue_bytes + 2

            # If there's enough room and some data left, start a new HBI page, otherwise fill with NOP
            rem_seg_bytes -= continue_bytes
            if (block_size_bytes < 10) or (rem_seg_bytes == 0):
                # Pad with NOP and start a new page write on the next block
                block_list_bytes += [0xFF] * (block_size_bytes)
                rem_page_bytes = 0
            else:
                block_list_bytes += [0x86, 0x81]
                block_list_bytes.append((addr >> 24) & 0xFF)
                block_list_bytes.append((addr >> 16) & 0xFF)
                block_list_bytes.append((addr >> 8) & 0xFF)
                block_list_bytes.append(0x00)

                # Paged offset access
                offset_words = (addr & 0xFF) / 2
                block_list_bytes.append(offset_words)
                rem_page_bytes -= offset_words * 2

                # Check if we have enough data to fill the block (account for the 8 bytes of HBI control)
                block_size_bytes -= 8
                rem_seg_bytes = seg_len_bytes - seg_index
                if (block_size_bytes > rem_seg_bytes):
                    pad_bytes = block_size_bytes - rem_seg_bytes
                    block_size_bytes = rem_seg_bytes
                else:
                    pad_bytes = 0

                # Append the length with the Write (0x80)
                block_list_bytes.append((block_size_bytes / 2 - 1) + 0x80)

                # Append the data and NOP (if any)
                block_list_bytes += segment_list[seg_index: seg_index + block_size_bytes]

                # End of the block (Leave the room for the begining of the next segment)
                if (pad_bytes >= 10):
                    left_over_bytes = pad_bytes
                else:
                    # Not enough room, fill with NOP (if any)
                    left_over_bytes = 0
                    block_list_bytes += [0xFF] * pad_bytes

                # Update the indexes and current address
                seg_index += block_size_bytes
                rem_page_bytes -= block_size_bytes
                addr += block_size_bytes

        # Reset the block size to the max
        block_size_bytes = block_size_words * 2

        # Append the block
        temp_list_bytes += block_list_bytes

    return temp_list_bytes

# ****************************************************************************
def FormatS7ToHbi(list, block_size):
    global left_over_bytes

    # Parse the S3 file in reverse order as the S7 record is usualy at the end
    for line in reversed(list):
        if (line[0: 2].lower() == "s7"):
            split_line = re.findall(r"[0-9a-fA-F]{2}", line[2:])
            split_line = map(lambda x: int(x, 16), split_line)

            # Check the checksum
            checksum = (sum(split_line) + 1) & 0xFF
            if checksum:
                raise ValueError("Error - FormatS7ToHbi(): S7 checksum mismatch")

            # Write the 32b execution address to the registers 0x12C 0x12E
            temp_list_bytes = [0xFE, 0x00, 0x2C >> 1, 0x01 | 0x80]
            temp_list_bytes.append(split_line[1])
            temp_list_bytes.append(split_line[2])
            temp_list_bytes.append(split_line[3])
            temp_list_bytes.append(split_line[4])

            if (left_over_bytes >= 8):
                # Fits in the last block, add NOP to fill the rest
                temp_list_bytes += [0xFF] * (left_over_bytes - 8)
            else:
                # Fill the previous block with NOP then add this segment in its own block
                temp_list_bytes = ([0xFF] * left_over_bytes) + temp_list_bytes
                temp_list_bytes += [0xFF] * (block_size * 2 - 8)

    return temp_list_bytes

# ****************************************************************************
def GenerateFwFile(in_file, in_path, out_path, fw_opn, block_size):
    global page_select
    global left_over_bytes

    s3_buffer_list = in_file.splitlines()

    # Get the S3firmware header in a list of bytes (the size is known and fixed... 216 bytes)
    header_bytes = ParseS3Segment(s3_buffer_list, 0x00080128, 216)
    if (header_bytes == []):
        return ""

    # Decode the S3 firmware header to have all the segment addresses and sizes
    header_words = struct.unpack(">108H", struct.pack("216B", *header_bytes))
    seg_table = DecodeTable(header_words)

    # Create a destination buffer, initialized with an empty 12 bytes header
    out_list_bytes = [0] * 12

    # Write all the segments to the out buffer with the S3 header at the end
    for seg in seg_table:
        out_list_bytes += FormatSegmentToHbi(ParseS3Segment(s3_buffer_list, seg[0], seg[1]), seg[0], block_size)
    out_list_bytes += FormatSegmentToHbi(header_bytes, 0x00080128, block_size)

    # Process the S7 segment
    out_list_bytes += FormatS7ToHbi(s3_buffer_list, block_size)

    # Cleanup
    page_select = True
    left_over_bytes = 0

    # Buffer length minus the header
    out_list_len = len(out_list_bytes) - 12

    # Populate the header (non populated are 0)
    out_list_bytes[0] = IMG_HDR_VERSION
    out_list_bytes[1] = IMG_HDR_FORMAT
    out_list_bytes[2] = (fw_opn >> 8) & 0xFF
    out_list_bytes[3] = fw_opn & 0xFF
    out_list_bytes[4] = 0
    out_list_bytes[5] = block_size
    out_list_bytes[6] = (out_list_len >> 24) & 0xFF
    out_list_bytes[7] = (out_list_len >> 16) & 0xFF
    out_list_bytes[8] = (out_list_len >> 8) & 0xFF
    out_list_bytes[9] = out_list_len & 0xFF

    # When used programmatically, this function returns the firmware in a buffer
    if (out_path == None):
        out_array = array("B", out_list_bytes)
        return out_array.tostring()

    # Save to a file
    filename, file_extension = os.path.splitext(out_path)
    if (file_extension.lower() == ".bin"):
        with open(out_path, "wb") as out_file:
            out_array = array("B", out_list_bytes)
            out_array.tofile(out_file)
    if ((file_extension.lower() == ".h") or (file_extension.lower() == ".c")):
        with open(out_path, "w") as out_file:
            print >> out_file, ("/* Generated from %s on %s */" % (ntpath.basename(in_path), time.strftime("%c")))
            print >> out_file, ("#ifndef __%s_H_" % fw_opn)
            print >> out_file, ("#define __%s_H_" % fw_opn)
            print >> out_file, "const unsigned char buffer[] = {"
            temp_str = "%s" % map(lambda x: ("0x%02X" % x), out_list_bytes[0: 12])
            print >> out_file, ("     " + temp_str.strip("[]").translate(None, "\'\""))
            for i in xrange(12, out_list_len, 16):
                temp_str = "%s" % map(lambda x: ("0x%02X" % x), out_list_bytes[i: i + 16])
                print >> out_file, ("    ," + temp_str.strip("[]").translate(None, "\'\""))
            print >> out_file, "};"
            print >> out_file, "#endif"

# ****************************************************************************
# This function is designed to be called programmatically
def GetFirmwareBinFile(in_path, fw_opn, block_size = 16):

    if not os.path.isfile(in_path):
        raise ValueError("Error - GetFirmwareBinFile(): Invalid input file")

    return GenerateFwFile(ParseFile(in_path), in_path, None, fw_opn, block_size)

# ****************************************************************************
# This function is designed to be called programmatically
def GetFirmwareBinFileB(in_file, fw_opn, block_size = 16):

    return GenerateFwFile(in_file, "", None, fw_opn, block_size)

# ****************************************************************************
def FormatData(addr, data_list):
    out_string = "    {0x%04X, {" % addr

    block_size = len(data_list)
    for i in xrange(0, block_size, 10):
        if (i > 0):
            # Add the proper padding
            out_string += " " * 14

        sub_str = map(lambda x: ("0x%04X" % x), data_list[i: i + 10])
        sub_str = ("%s" % sub_str).translate(None, "\'\"\[\]")

        if ((i + 10) >= block_size):
            # End of the block
            out_string += sub_str + "}},\n"
        else:
            out_string += sub_str + ",\n"

    return out_string

# ****************************************************************************
def GenerateConfigFile(inputPath, outputPath, blockSize):
    data_buffer = []
    prev_addr = 0
    current_addr = 0
    block_size = 0
    stream_len = 0
    out_string = ""

    config_record_buf = ParseFile(inputPath)
    # Only match the addresses and values in the configuration record
    for match in re.finditer(r"^\s*(0x[0-9a-fA-F]+).*(0x[0-9a-fA-F]+)", config_record_buf, re.MULTILINE):
            addr = int(match.group(1), 16)
            data = int(match.group(2), 16)

            # Setup the starting values
            if (prev_addr == 0):
                current_addr = addr
                prev_addr = current_addr - 2

            if (addr != (prev_addr + 2)):
                raise ValueError("Error - GenerateConfigFile(): Address discontinuity detected")

            # Accumulate the data
            block_size += 1
            data_buffer.append(data)
            prev_addr = addr

            if (block_size == blockSize):
                # Reached the set block size
                out_string += FormatData(addr - 2 * (block_size - 1), data_buffer)
                stream_len += 1
                data_buffer = []
                block_size = 0
                prev_addr = 0

    # If the config record is not a multiple of blockSize, pad with 0s
    if (block_size > 0):
        data_buffer += [0] * (blockSize - block_size)
        out_string += FormatData(addr - 2 * (block_size - 1), data_buffer)
        stream_len += 1
        print ("Info - Pad with %d zero(s)" % (blockSize - block_size))

    # Trim the last \n and coma
    out_string = out_string.rstrip("\n").rstrip(",")

    with open(outputPath, "w") as out_file:
        print >> out_file, ("/* Generated from %s on %s */\n" % (ntpath.basename(inputPath), time.strftime("%c")))
        print >> out_file, "const dataArr st_twConfig[] = {"
        print >> out_file, out_string
        print >> out_file, "};\n"
        print >> out_file, ("const unsigned short zl_configBlockSize = %d;" % blockSize)
        print >> out_file, ("const unsigned short configStreamLen = %d;" % stream_len)

# ****************************************************************************
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class = argparse.RawDescriptionHelpFormatter,
        description = "Timberwolf firmware/config converter",
        epilog = ("""
Firmware images (*.s3) can be converted in binary (*.bin) to be
dynamically loaded or C (*.c) to be statically compiled.
Configuration records (*.cr2) only need to be converted if they
are statically compiled as (*.cr2) can be dynamically loaded.

ex: %s ZLS38040_firmware.s3 ZLS38040_firmware.bin -b 64 -f 38040
ex: %s ZLS38040_firmware.s3 ZLS38040_firmware.c -b 64 -f 38040
ex: %s ZLS38040_configuration.cr2 ZLS38040_configuration.c -b 64
""" % (sys.argv[0], sys.argv[0], sys.argv[0])))
    parser.add_argument("inputPath", help = "input firmware image path (*.s3) or configuration record path (*.cr2)")
    parser.add_argument("outputPath", help = "output firmware image path (*.bin, *.c) or configuration record path (*.c)")
    parser.add_argument("-b", "--blockSize", help = "block size of 16b words, multiple of 16 (default = 16, max = 128)", type = int, default = 16)
    parser.add_argument("-f", "--firmwareOPN", help = "firmware numerical OPN (ex: 38040), required only for firmware images", type = int)

    # Print the help if no arguments are passed
    if (len(sys.argv[1:]) == 0):
        parser.print_help()
        parser.exit()

    # Parse the input arguments
    args = parser.parse_args()

    programmatic = False

    try:
        # Figure out the type of conversion
        if not os.path.isfile(args.inputPath):
            raise ValueError("Error - Main(): Invalid input file")

        filename, file_extension = os.path.splitext(args.inputPath)

        if (file_extension.lower() == ".s3"):
            if (args.firmwareOPN == None):
                raise ValueError("Error - Main(): firmware numerical OPN (-f) is missing")
            # Process a firmware image
            GenerateFwFile(ParseFile(args.inputPath), args.inputPath, args.outputPath, args.firmwareOPN, args.blockSize)

        elif (file_extension.lower() == ".cr2"):
            # Process a configuration record
            GenerateConfigFile(args.inputPath, args.outputPath, args.blockSize)

        else:
            raise ValueError("Error - Main(): unsupported file format (%s)" % file_extension)

    except ValueError as err:
        print err
