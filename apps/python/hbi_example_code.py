#!/usr/bin/env python

from os.path import dirname, realpath, isfile
import sys
from tw_firmware_converter import GetFirmwareBinFile
from hbi_load_firmware import *
sys.path.append(dirname(realpath(__file__)) + "/../../vproc_sdk/libs")
from hbi import *

# Set to 'True' if you have a proper firmware and configuration in the folder
FW_LOAD = False

print "-= Python HBI example code =-"

if FW_LOAD:
    print "\nFirmware and configuration loading example:"
    try:
        # Convert the S3 in BIN
        # Change the line below to point to an actual firmware file
        fw_bin = GetFirmwareBinFile("Microsemi_ZLS38063.1_E0_10_0_firmware.s3", 38063, 64)
        print "    - Firmware converted"

    except ValueError as err:
        print err
        sys.exit()

# Init the HBI driver
cfg = hbi_dev_cfg_t();
handle = HBI_open(cfg)

try:
    if FW_LOAD:
        StopFirmware(handle)
        print "    - Firmware stopped"

        # EraseFlash() automatically initialize the flash first
        # Note: If you don't erase the flash and want to do any flash related
        #   operations when the firmware is stopped you should call InitFlash(handle) first
        # InitFlash(handle)
        # print "    - Flash initialized"
        EraseFlash(handle)
        print "    - Flash erased"

        # Load a firmware
        LoadFirmware(handle, fw_bin)
        print "    - Firmware loaded"

        # Load a configuration record
        # Note: ParseCr2File() only loads the entire file into a buffer, nothing more
        LoadConfigCr2(handle, ParseCr2File("Microsemi_ZLS38063.1_E0_10_0_config.cr2"))
        print "    - Configuration loaded"

        # Save both firmware and configuration to flash
        # Note: Here, as the flash got erased, flash_slot will be 1
        flash_slot = SaveFirmwareToFlash(handle)
        print "    - Firmware saved in slot %d" % flash_slot

        # Start the firmware previously loaded
        StartFirmware(handle)
        print "    - Firmware started"

    print "\nRegister read/write example:"
    # Register read example, reads the PCN (read 2 bytes at address 0x022)
    # Note: FormatNumber() converts a list of bytes in an integer
    # Note: Timberwolf is big endian
    pcn = FormatNumber(HBI_read(handle, 0x022, 2))
    print "    - Product code number: %d" % pcn

    # Register read modify write example, mute Sout in Control register 0
    ctrl0 = HBI_read(handle, 0x300, 2)
    HBI_write(handle, 0x300, (ctrl0[0] | 0x01, ctrl0[1]))
    print "    - Mute Sout"

    # Read Control register 0
    print "    - Control register 0: 0x%04X" % FormatNumber(HBI_read(handle, 0x300, 2))

    # Restore Sout (assumes it was not muted originaly)
    HBI_write(handle, 0x300, ctrl0)
    print "    - Restore Control register 0"

    # Read Control register 0
    print "    - Control register 0: 0x%04X" % FormatNumber(HBI_read(handle, 0x300, 2))

except ValueError as err:
    print err

# Close HBI driver
HBI_close(handle)
