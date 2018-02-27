#!/usr/bin/env python

# MiTuner_socket_server.py  --  Python 2.7 socket server to be used with MiTuner Bridge

# Copyright 2018 Microsemi Inc. All rights reserved.
#Licensed under the MIT License. See LICENSE.txt in the project root for license information.

from os.path import dirname, realpath, isfile
from itertools import izip
import argparse
import sys
import struct
import socket
sys.path.append(dirname(realpath(__file__)) + "/../../vproc_sdk/libs")
from hbi import *
from tw_firmware_converter import GetFirmwareBinFileB
from hbi_load_firmware import LoadFirmware, SaveFirmwareToFlash, InitFlash, EraseFlash, SaveConfigToFlash, IsFirmwareRunning, LoadFirmwareFromFlash

# Port for the socket (random)
PORT = 5678
BUFFER_SZ = 2048
HEADER_SZ = 6

# ****************************************************************************
def FormatNumber(res_list):
    number = 0

    for byteNum in res_list:
        number = (number << 8) + byteNum

    return number

# ****************************************************************************
def SpiBufferRead(handle, address, numBytes):
    bufferString = ""

    byteList = HBI_read(handle, address, numBytes)
    for byteEl in byteList:
        bufferString += "%02X" % byteEl

    return bufferString

# ****************************************************************************
def SpiBufferWrite(handle, address, bufferString):
    byteList = []
    nbBytes = len(bufferString) / 2

    for i in xrange(nbBytes):
        byteList.append(int(bufferString[i * 2: i * 2 + 2], 16))

    HBI_write(handle, address, byteList)

# ****************************************************************************
def SpiHwBufferRead(handle, address, numBytes):
    bufferString = ""

    # Setup the start address using the page 255 scheme
    addressSeq = struct.unpack("4B", struct.pack(">I", address))
    hbiOffset = addressSeq[3]
    HBI_write(handle, 0x00C, addressSeq)

    # Read the requested bytes
    byteList = HBI_read(handle, 0xFF00 + hbiOffset, numBytes)
    for byteEl in byteList:
        bufferString += "%02X" % byteEl

    return bufferString

# ****************************************************************************
def FirmwareLoading(handle, type, cmd):

    if (type == "FA"):
        # Start to receive a new file
        FirmwareLoading.s3File = cmd
    elif (type == "FB"):
        # Continue to receive a new file
        FirmwareLoading.s3File += cmd
    else:
        # FC, receive the last piece and load
        FirmwareLoading.s3File += cmd

        try:
            # Convert the S3 in BIN (doesn't matter if not a 38040)
            fwBin = GetFirmwareBinFileB(FirmwareLoading.s3File, 38040, 64)

            # Load the FW
            LoadFirmware(handle, fwBin)

        except ValueError as err:
            print err
            return "ERROR"

    return "OK"

# ****************************************************************************
def EraseSpiFlash(handle):

    try:
        EraseFlash(handle)

    except ValueError as err:
        print err
        return "ERROR"

    return "OK"

# ****************************************************************************
def SaveFirmware2Flash(handle):

    try:
        InitFlash(handle)
        SaveFirmwareToFlash(handle)

    except ValueError as err:
        print err
        return "ERROR"

    return "OK"

# ****************************************************************************
def SaveConfig2Flash(handle, index):

    try:
        if not IsFirmwareRunning(handle):
            InitFlash(handle)

        SaveConfigToFlash(handle, index)

    except ValueError as err:
        print err
        return "ERROR"

    return "OK"

# ****************************************************************************
def LoadFwfromFlash(handle, index):

    try:
        InitFlash(handle)
        LoadFirmwareFromFlash(handle, index)

    except ValueError as err:
        print err
        return "ERROR"

    return "OK"

# ****************************************************************************
def ParseCmd(handle, header, cmd):

    if (header[0: 2] == "RD"):
        # 16b read
        retval = "%04X" % FormatNumber(HBI_read(handle, int(cmd[0: 3], 16), 2))
    elif (header[0: 2] == "WR"):
        # 16b write
        HBI_write(handle, int(cmd[0: 3], 16), (int(cmd[3: 5], 16), int(cmd[5: 7], 16)))
        retval = "OK"
    elif (header[0: 2] == "BR"):
        # Buffer read
        retval = SpiBufferRead(handle, int(cmd[0: 3], 16), int(cmd[3: 7], 16) * 2)
    elif (header[0: 2] == "BW"):
        # Buffer write
        retval = SpiBufferWrite(handle, int(cmd[0: 3], 16), cmd[3:])
        retval = "OK"
    elif (header[0: 2] == "HR"):
        # Hardware buffer read (HBI 255 access)
        retval = SpiHwBufferRead(handle, int(cmd[0: 8], 16), int(cmd[8: 10], 16))
    elif (header[0: 2] == "FA") or (header[0: 2] == "FB") or (header[0: 2] == "FC"):
        retval = FirmwareLoading(handle, header[0: 2], cmd)
    elif (header[0: 2] == "ER"):
        retval = EraseSpiFlash(handle)
    elif (header[0: 2] == "SF"):
        retval = SaveFirmware2Flash(handle)
    elif (header[0: 2] == "SC"):
        retval = SaveConfig2Flash(handle, int(cmd, 16))
    elif (header[0: 2] == "LF"):
        retval = LoadFwfromFlash(handle, int(cmd, 16))
    else:
        retval = "ERROR"

    return "ANS" + ("%04X" % len(retval)) + retval

# ****************************************************************************
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Raspberry Pi socket server for MiTuner V1.0.0")
    parser.add_argument("-d", "--debug", help = "debug level 0: none, 1: in, 2: out, 3: in/out", type = int, default = 0)

    # Parse the input arguments
    args = parser.parse_args()

    # Init the HBI driver
    cfg = hbi_dev_cfg_t();
    handle = HBI_open(cfg)

    try:
        # Create a socket and listen on port 'PORT'
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(('', PORT))
        s.listen(1)

        # Accept connections from outside
        print "Socket created on port %d, waiting for a connection" % PORT
        while True:
            clientsocket, address = s.accept()
            print "Incoming connection from: %s" % address[0]

            message = ""
            waitType = "header"
            while True:
                buff = clientsocket.recv(BUFFER_SZ)
                if (buff == ""):
                    print "Connection closed by the client (%s)" % address[0]
                    break
                else:
                    message += buff
                    if ((waitType == "header") and (len(message) >= HEADER_SZ)):
                        header = message[0: HEADER_SZ]
                        message = message[HEADER_SZ:]
                        cmdLen = int(header[2: 6], 16)
                        waitType = "cmd"

                    if ((waitType == "cmd") and (len(message) >= cmdLen)):
                        cmd = message[0: cmdLen]
                        message = message[cmdLen:]
                        if (args.debug & 1):
                            print "header = %s, cmd = %s" % (header, cmd)
                        answer = ParseCmd(handle, header, cmd)
                        if (args.debug & 2):
                            print "\t" + answer
                        clientsocket.send(answer)
                        waitType = "header"

            clientsocket.close()

    except:
        print "Server shut down"

    # Close the Socket
    s.close()

    # Close HBI driver
    HBI_close(handle)
