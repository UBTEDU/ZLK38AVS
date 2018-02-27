/*
* I2cLib.h  --  I2C Wrapper based on the Linux I2C_RDWR IOCTL Header
*
* Copyright 2018 Microsemi Inc. All rights reserved.
* Licensed under the MIT License. See LICENSE.txt in the project root for license information.
*/

#ifndef I2cLib_H
#define I2cLib_H

#include <stdio.h>
#include <iostream>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>


class I2cLib {
    public:
        // Constructor, Destructor
        I2cLib(unsigned char devAddr);
        ~I2cLib(void);

        // Functions
        int WriteReg(unsigned char regAddr, unsigned char data);
        int ReadReg(unsigned char regAddr, unsigned char *data, bool printErr = true);
        int WriteBuff(unsigned char regAddr, unsigned char *data, unsigned char length);
        int ReadBuff(unsigned char regAddr, unsigned char *data, unsigned char length);

    private:
        // Defines
        static const unsigned char MAX_I2C_BUFF_LEN = 128;

        // Variables
        std::string  i2cFileName;
        int i2cDescriptor;
        unsigned char deviceAddress;
        unsigned char buffData[MAX_I2C_BUFF_LEN];

        // Functions
        int OpenI2c();  // Called only in constructors
        int CloseI2c(); // Called only in destructor
};

#endif
