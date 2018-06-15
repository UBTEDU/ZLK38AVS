#!/usr/bin/env python

# hbi_load_firmware.py  --  Python 2.7 script to manage Timberwolf firmwares and configurations

# Copyright 2018 Microsemi Inc. All rights reserved.
#Licensed under the MIT License. See LICENSE.txt in the project root for license information.

# Functions designed for programmatic interface:
# IsFirmwareRunning(handle)
#     Returns 0x8000 (essentially True) if the firmware is running, 0 otherwise
#
# SoftReset(handle)
#     Issue a soft reset
#
# StopFirmware(handle)
#     Stop the firmware and wait for the bootrom to be ready
#
# StartFirmware(handle, skip_error = False)
#     Start the firmware and throw an error (if enabled) if it can't start
#
# InitFlash(handle, skip_error = False)
#     Dectect the flash and throw an error (if enabled) if it can't find one
#     Note: Firmware has to be stopped
#
# EraseFlash(handle)
#     Erase the entire flash then initialize it
#     Note: Firmware has to be stopped
#
# LoadFirmwareFromFlash(handle, firmware_index)
#     Load the firmware specified by its index from flash
#     Note: Firmware has to be stopped
#
# LoadFirmware(handle, firmware_buf)
#     Load a .bin firmware image
#     Note: Firmware has to be stopped
#
# LoadConfigCr2(handle, config_record_buf)
#     Load a .cr2 configuration record
#     Note: Firmware can be stopped or running
#
# flash_slot = SaveFirmwareToFlash(handle)
#     Save the RAM firmware to flash and returns the flash slot used
#     Note: Firmware has to be stopped
#
# SaveConfigToFlash(handle, flash_slot = 1)
#     Save the RAM configuration to the specified flash slot
#     Note: Firmware can be stopped or running
#
# cr2_buf = ParseCr2File(path)
#     Reads the CR2 file and returns it as a buffer without processing , beware no error checking!

from os.path import dirname, realpath, isfile
import sys
import re
import os
import struct
import time
import argparse
sys.path.append(dirname(realpath(__file__)) + "/../../vproc_sdk/libs")
from hbi import *

CR2_BLOCK_SIZE = 16

# Globals
programmatic = True

# ****************************************************************************
def FormatNumber(res_list):
    number = 0

    for byteNum in res_list:
        number = (number << 8) + byteNum

    return number

# ****************************************************************************
def BusySpinWait(handle):
    start_time = time.time()

    # The register 0x032 reads 0xFFFF while the Timberwolf is processing a command
    while (FormatNumber(HBI_read(handle, 0x032, 2)) == 0xFFFF):
        if ((time.time() - start_time) > 30):
            raise ValueError("Error - BusySpinWait(): Timeout")

# ****************************************************************************
def IsFirmwareRunning(handle):
    return FormatNumber(HBI_read(handle, 0x028, 2)) & 0x8000

# ****************************************************************************
def SoftReset(handle):
    HBI_write(handle, 0x006, (0x00, 0x02))
    BusySpinWait(handle)

# ****************************************************************************
def StopFirmware(handle):
    start_time = time.time()

    # Stop the firmware (reboot into the bootrom)
    HBI_write(handle, 0x014, (0x00, 0x01))

    # The register 0x034 should read 0xD3D3 when the reboot is compete
    while (FormatNumber(HBI_read(handle, 0x034, 2)) != 0xD3D3):
        if ((time.time() - start_time) > 5):
            raise ValueError("Error - StopFirmware(): Timeout in StopFirmware")

    if not programmatic:
        print("Info - Firmware stopped")

# ****************************************************************************
def StartFirmware(handle, skip_error = False):
    global programmatic

    # Start the firmware
    HBI_write(handle, 0x032, (0x00, 0x08))
    HBI_write(handle, 0x006, (0x00, 0x01))
    BusySpinWait(handle)

    if not IsFirmwareRunning(handle):
        if skip_error:
            if not programmatic:
                print("Warning - Firmware stopped")
        else:
            raise ValueError("Error - StartFirmware(): Firmware cannot start")
    elif not programmatic:
        print("Info - Firmware started")

# ****************************************************************************
def InitFlash(handle, skip_error = False):
    global programmatic

    # Firmware has to be stopped to initialize the flash
    if IsFirmwareRunning(handle):
        raise ValueError("Error - InitFlash(): Firmware running")

    # Start the firmware
    HBI_write(handle, 0x032, (0x00, 0x0B))
    HBI_write(handle, 0x006, (0x00, 0x01))
    BusySpinWait(handle)

    # Check for error
    status = FormatNumber(HBI_read(handle, 0x034, 2)) & 0x000F
    if (status != 0):
        if skip_error:
            if not programmatic:
                print("Info - No flash detected")
        else:
            raise ValueError("Error - InitFlash(): Cannot initialize the flash (0x%X)" % status)

# ****************************************************************************
def EraseFlash(handle):
    global programmatic

    # Firmware has to be stopped to erase the flash
    if IsFirmwareRunning(handle):
        raise ValueError("Error - EraseFlash(): Firmware running")

    InitFlash(handle)

    # Erase the flash
    HBI_write(handle, 0x032, (0x00, 0x09))
    HBI_write(handle, 0x034, (0xAA, 0x55))
    HBI_write(handle, 0x006, (0x00, 0x01))
    BusySpinWait(handle)

    # Check for error
    status = FormatNumber(HBI_read(handle, 0x034, 2)) & 0x000F
    if (status != 0):
        raise ValueError("Error - EraseFlash(): Cannot erase the flash (0x%X)" % status)

    if not programmatic:
        print("Info - Flash erase completed")

# ****************************************************************************
def LoadFirmwareFromFlash(handle, firmware_index):
    global programmatic

    # Firmware has to be stopped to load from flash
    if IsFirmwareRunning(handle):
        raise ValueError("Error - LoadFirmwareFromFlash(): Firmware running")

    # Load firmware from flash
    HBI_write(handle, 0x032, (0x00, 0x02))
    HBI_write(handle, 0x034, (0x00, firmware_index))
    HBI_write(handle, 0x006, (0x00, 0x01))
    BusySpinWait(handle)

    # Check for error
    status = FormatNumber(HBI_read(handle, 0x034, 2)) & 0x000F
    if (status != 0):
        raise ValueError("Error - LoadFirmwareFromFlash(): Cannot load the firmware %d from flash (%d)" % (firmware_index, status))

    elif not programmatic:
        print("Info - Firmware and configuration from slot %d were successfully loaded" % firmware_index)

# ****************************************************************************
def LoadFirmware(handle, firmware_buf):
    global programmatic

    # Firmware has to be stopped to load a new one
    if IsFirmwareRunning(handle):
        raise ValueError("Error - LoadFirmware(): Firmware running")

    # Get the firmware info
    data = hbi_data_t()
    data.size = len(firmware_buf)
    data.pData = firmware_buf
    header = hbi_img_hdr_t()
    status = HBI_get_header(data, header)

    if (status != HBI_STATUS_SUCCESS):
        raise ValueError("Error - LoadFirmware(): Invalid Image Header Found (%d)" % status)

    # Strip the firmware header
    firmware_buf = firmware_buf[header.hdr_len : header.hdr_len + header.img_len]

    # Convert the block size in bytes (from 16b words)
    block_size = header.block_size * 2

    # Send the firmware "block_size" at a time
    image_len = header.img_len
    block_num = image_len // block_size
    if ((image_len % block_size) != 0):
        raise ValueError("Error - LoadFirmware(): The firmware size is not a multiple of block_size")

    for i in range(0, block_num):
        offset = i * block_size
        data.pData = firmware_buf[offset : offset + block_size]
        data.size = block_size
        status = HBI_set_command(handle, HBI_CMD_LOAD_FWR_FROM_HOST, data)
        if (status != HBI_STATUS_SUCCESS):
            raise ValueError("Error - LoadFirmware(): Firmware loading issue (%d)" % status)

    status = HBI_set_command(handle, HBI_CMD_LOAD_FWR_COMPLETE, None)
    if (status != HBI_STATUS_SUCCESS):
        raise ValueError("Error - LoadFirmware(): Firmware final loading issue (%d)" % status)

    if not programmatic:
        print("Info - Firmware successfully loaded")

# ****************************************************************************
def LoadConfigCr2(handle, config_record_buf):
    global programmatic

    dataBuffer = []
    prev_addr = 0
    current_addr = 0
    block_size = 0
    for match in re.finditer(r"^\s*(0x[0-9a-fA-F]+).*(0x[0-9a-fA-F]+)", config_record_buf, re.MULTILINE):
            addr = int(match.group(1), 16)
            data = int(match.group(2), 16)

            # Setup the starting values
            if (prev_addr == 0):
                current_addr = addr
                prev_addr = current_addr - 2

            if (addr != (prev_addr + 2)):
                # Non consecutive addresses detected, ship the current buffer
                HBI_write(handle, current_addr, dataBuffer)
                dataBuffer = []
                block_size = 0
                current_addr = addr

            # Accumulate the data
            block_size += 1
            dataBuffer.append((data >> 8) & 0x00FF)
            dataBuffer.append(data & 0x00FF)
            prev_addr = addr

            if (block_size == CR2_BLOCK_SIZE):
                # Reached the set block size
                HBI_write(handle, current_addr, dataBuffer)
                dataBuffer = []
                block_size = 0
                prev_addr = 0

    if not programmatic:
        print("Info - Configuration record successfully loaded")

# ****************************************************************************
def SaveFirmwareToFlash(handle):
    global programmatic

    # In case a configuration was also loaded (bootrom will recompute the checksum)
    HBI_write(handle, 0x01F2, (0x00, 0x00))

    # Save firmware to flash
    HBI_write(handle, 0x032, (0x00, 0x04))
    HBI_write(handle, 0x006, (0x00, 0x01))
    BusySpinWait(handle)

    # Check for error
    status = FormatNumber(HBI_read(handle, 0x034, 2)) & 0x000F
    if (status != 0):
        raise ValueError("Error - SaveFirmwareToFlash(): Cannot save the firmware and configuration to flash (0x%X)" % status)

    # Read in which flash slot the firmware was saved
    slot = FormatNumber(HBI_read(handle, 0x026, 2))

    if not programmatic:
        print("Info - Firmware and configuration saved to flash slot %d" % slot)

    return slot

# ****************************************************************************
def SaveConfigToFlash(handle, flash_slot = 1):
    global programmatic

    # Save the configuration in the specified flash slot
    if IsFirmwareRunning(handle):
        HBI_write(handle, 0x032, (0x80, 0x02))
        HBI_write(handle, 0x034, (0x00, flash_slot))
        HBI_write(handle, 0x006, (0x00, 0x04))
        # An extra delay is required after a software semaphore flag and before BusySpinWait()
        time.sleep(0.01)
    else:
        # Must set the config record checksum to 0 before saving to flash (bootrom will recompute)
        HBI_write(handle, 0x01F2, (0x00, 0x00))

        HBI_write(handle, 0x032, (0x00, 0x07))
        HBI_write(handle, 0x034, (0x00, flash_slot))
        HBI_write(handle, 0x006, (0x00, 0x01))

    BusySpinWait(handle)

    # Check for error
    status = FormatNumber(HBI_read(handle, 0x034, 2)) & 0x000F
    if (status != 0):
        raise ValueError("Error - SaveConfigToFlash(): Cannot save the configuration to flash (0x%X)" % status)

    if not programmatic:
        print("Info - Configuration saved to flash slot %d" % args.indexSlot)

# ****************************************************************************
def ParseBinFile(path):
    # Note: "b" in "rb" is only required for Windows to read in binary but doesn't hurt on Linux
    with open(path, "rb") as f:
        buf = f.read()

    return buf

# ****************************************************************************
def ParseCr2File(path):
    with open(path, "r") as f:
        buf = f.read()

    return buf

# ****************************************************************************
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class = argparse.RawDescriptionHelpFormatter,
        description = "Timberwolf firmware/config/flash manager",
        epilog = ("""
Firmware images (*.bin) and/or configuration records (*.cr2) can be loaded
to RAM and saved to flash. If only a configuration record is selected, the
firmware will not be stopped and a soft reset will automatically be generated
once the configuration is loaded.
Firmware and configuration can also be loaded from flash if available.

ex: Erase the flash, load a new firmware and configuration, save to flash:
    %s -e -f ZLS38040_firmware.bin -c ZLS38040_config.cr2 -s
ex: Load a configuration at runtime and save it with the firmware stored in
    flash slot 2:
    %s -c ZLS38040_config.cr2 -s -i 2
ex: Load a firmware and configuration from flash slot 3:
    %s -l 3
""" % (sys.argv[0], sys.argv[0], sys.argv[0])))
    parser.add_argument("-e", "--eraseFlash", help = "erase the flash", action = "store_true")
    parser.add_argument("-f", "--firmwarePath", help = "firmware image path (*.bin)")
    parser.add_argument("-c", "--configPath", help = "configuration record path (*.cr2)")
    parser.add_argument("-s", "--saveToFlash", help = "save the firmware and/or configuration to flash", action = "store_true")
    parser.add_argument("-i", "--indexSlot", help = "flash slot index when saving the configuration at run time (default = 1)", type = int, default = 1)
    parser.add_argument("-l", "--loadFromFlash", help = "load the firmware and configuration associated with the specified index", type = int)
    parser.add_argument("-cs", "--chipSelect", help = "SPI chip select (default = 0)", type = int, default = 0)

    # Print the help if no arguments are passed
    if (len(sys.argv[1:]) == 0):
        parser.print_help()
        parser.exit()

    # Parse the input arguments
    args = parser.parse_args()

    # The program was called through the CLI
    programmatic = False

    # Parse the firmware file
    if ((args.firmwarePath != None) and isfile(args.firmwarePath)):
        firmware_buf = ParseBinFile(args.firmwarePath)
    else:
        firmware_buf = ""

    # Parse the configuration record file
    if ((args.configPath != None) and isfile(args.configPath)):
        config_record_buf = ParseCr2File(args.configPath)
    else:
        config_record_buf = ""

    # Error checking
    if ((firmware_buf == "") and (config_record_buf == "") and args.saveToFlash):
        raise ValueError("Error - Main(): Both firmware and configuration are invalid, nothing to save to flash")

    if ((firmware_buf == "") and (config_record_buf != "") and args.eraseFlash):
        raise ValueError("Error - Main(): Cannot erase the flash and only load a configuration")

    # Init the HBI driver
    cfg = hbi_dev_cfg_t();
    cfg.deviceId = args.chipSelect
    handle = HBI_open(cfg)
    print("Info - Device open on SPI CS%d" % args.chipSelect)

    try:
        # Check if the firmware is stopped
        fw_running = IsFirmwareRunning(handle)

        # Do not stop the firmware to only load a config
        if ((firmware_buf != "") or args.eraseFlash or args.loadFromFlash):
            if fw_running:
                StopFirmware(handle)
                fw_running = False

            # Initialize the flash (if any) and skip any errors
            InitFlash(handle, True)

        if args.loadFromFlash:
            if ((firmware_buf != "") or args.eraseFlash):
                raise ValueError("Error - Main(): Incompatible input arguments for loading from flash")
            else:
                LoadFirmwareFromFlash(handle, args.loadFromFlash)

        if args.eraseFlash:
            EraseFlash(handle)

        if (firmware_buf != ""):
            LoadFirmware(handle, firmware_buf)

            if args.saveToFlash:
                slot = SaveFirmwareToFlash(handle)

        if (config_record_buf != ""):
            # Load the configuration record
            if (config_record_buf != ""):
                filename, file_extension = os.path.splitext(args.configPath)
                if (file_extension.lower() == ".cr2"):
                    LoadConfigCr2(handle, config_record_buf)
                else:
                    raise ValueError("Error - Main(): Unknown configuration extension (%s)" % file_extension)

                # Soft reset required if loading a config at run time (doesn't hurt even if the config was not loaded)
                if fw_running:
                    if args.saveToFlash:
                        SaveConfigToFlash(handle, args.indexSlot)
                    SoftReset(handle)
                elif args.saveToFlash:
                    # Save the configuration in teh same slot as the previously loaded firmware
                    SaveConfigToFlash(handle, slot)

        if not fw_running:
            StartFirmware(handle, True)

    except ValueError as err:
        print(err)

    # Close HBI driver
    HBI_close(handle)
