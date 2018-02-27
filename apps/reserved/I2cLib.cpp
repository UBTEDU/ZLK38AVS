/*
* I2cLib.cpp  --  I2C Wrapper based on the Linux I2C_RDWR IOCTL Implementation
*
* Copyright 2018 Microsemi Inc. All rights reserved.
* Licensed under the MIT License. See LICENSE.txt in the project root for license information.
*/

#include "SampleApp/I2cLib.h"

using namespace std;

/*******************************************************************
 * Constructor
 * *****************************************************************/
I2cLib::I2cLib(unsigned char devAddr) {


    this->i2cFileName = "/dev/i2c-1";
    this->deviceAddress = devAddr;
    this->i2cDescriptor = -1;
    this->OpenI2c();
}

/**********************************************************************
 * Destructor
 * *********************************************************************/
I2cLib::~I2cLib(void) {

    this->CloseI2c();
}

/**********************************************************************
 * System call - Open the device and keep track of the descriptor
 * ********************************************************************/
int I2cLib::OpenI2c() {

    this->i2cDescriptor = open(i2cFileName.c_str(), O_RDWR);
    if (this->i2cDescriptor < 0) {
        cerr << "OpenI2c(): Could not open I2C file driver " << this->i2cDescriptor << endl;
        exit(1);
    }

    return this->i2cDescriptor;
}

/*********************************************************************
 * System call - Close the device based on the descriptor
 * *******************************************************************/
int I2cLib::CloseI2c() {
    int retVal;

    retVal = close(this->i2cDescriptor);
    if (retVal < 0) {
        cerr << "CloseI2c(): Could not close I2C file driver " << retVal << endl;
    }

    return retVal;
}

/********************************************************************
 * Write one byte using a 8b address via the ioctl(I2C_RDWR) system function
 ********************************************************************/
int I2cLib::WriteReg(unsigned char regAddr, unsigned char data) {
    unsigned char buff[2];
    int retVal;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    buff[0] = regAddr;
    buff[1] = data;

    // Build the message
    messages[0].addr = this->deviceAddress;
    messages[0].flags = 0;
    messages[0].len = sizeof(buff);
    messages[0].buf = buff;

    packets.msgs = messages;
    packets.nmsgs = 1;

    retVal = ioctl(this->i2cDescriptor, I2C_RDWR, &packets);
    if (retVal < 0) {
        cerr << "WriteReg(8bit): ioctl access failed " << retVal << endl;
    }

    return retVal;
}

/********************************************************************
 * Read one byte using a 8b address via the ioctl(I2C_RDWR) system function
 *  Write the register address to read then read one byte
 * "printErr" can be set to false if the function is used to check silently
 *  if a device is present on the bus 
 ********************************************************************/
int I2cLib::ReadReg(unsigned char regAddr, unsigned char *data, bool printErr) {
    unsigned char outBuff;
    int retVal;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    // Build the outgoing message
    outBuff = regAddr;
    messages[0].addr = this->deviceAddress;
    messages[0].flags= 0;
    messages[0].len = sizeof(outBuff);
    messages[0].buf = &outBuff;

    // Build the input message
    messages[1].addr = this->deviceAddress;
    messages[1].flags = I2C_M_RD;
    messages[1].len = sizeof(*data);
    messages[1].buf = data;

    packets.msgs = messages;
    packets.nmsgs = 2;

    retVal = ioctl(this->i2cDescriptor, I2C_RDWR, &packets);
    if ((retVal < 0) && printErr) {
        cerr << "ReadReg(8bit): ioctl access failed " << retVal << endl;
    }

    return retVal;
}

/********************************************************************
 * Write a buffer using a 8b address via the ioctl() system function
 ********************************************************************/
int I2cLib::WriteBuff(unsigned char regAddr, unsigned char *data, unsigned char length) {
    int retVal;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    if (length > (this->MAX_I2C_BUFF_LEN - 1)) {
        cerr << "WriteBuff(8bit): buffed exceeds " << (this->MAX_I2C_BUFF_LEN - 1) << " bytes" << endl;
        return -1;
    }

    // Fill the data buffer starting with the address
    this->buffData[0] = regAddr;
    for (unsigned char i = 0; i < length; i++) {
        this->buffData[i + 1] = data[i];
    }

    // Build the message
    messages[0].addr = this->deviceAddress;
    messages[0].flags = 0;
    messages[0].len = length + 1;
    messages[0].buf = this->buffData;

    packets.msgs = messages;
    packets.nmsgs = 1;

    retVal = ioctl(this->i2cDescriptor, I2C_RDWR, &packets);
    if (retVal < 0) {
        cerr << "WriteBuff(8bit): ioctl access failed " << retVal << endl;
    }

    return retVal;
}

/********************************************************************
 * Read a buffer using a 8b address via the ioctl() system function
 *  Write the register address to read then read the buffer
 ********************************************************************/
int I2cLib::ReadBuff(unsigned char reg8BitAddr, unsigned char *data, unsigned char length) {
    int retVal;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    if (length > (this->MAX_I2C_BUFF_LEN)) {
        cerr << "ReadBuff(8bit): buffed exceeds " << (this->MAX_I2C_BUFF_LEN) << " bytes" << endl;
        return -1;
    }

    // Build the outgoing message
    messages[0].addr = this->deviceAddress;
    messages[0].flags= 0;
    messages[0].len = 1;
    messages[0].buf = &reg8BitAddr;

    // Build the input message
    messages[1].addr = this->deviceAddress;
    messages[1].flags = I2C_M_RD;
    messages[1].len = length;
    messages[1].buf = data;

    packets.msgs = messages;
    packets.nmsgs = 2;

    retVal = ioctl(this->i2cDescriptor, I2C_RDWR, &packets);
    if (retVal < 0) {
        cerr << "ReadBuff(8bit): ioctl access failed " << retVal << endl;
    }

    return retVal;
}
