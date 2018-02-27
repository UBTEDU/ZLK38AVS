/*
* LedDriver.h  --  IS31FL3731 Wrapper Header
*
* Copyright 2018 Microsemi Inc. All rights reserved.
* Licensed under the MIT License. See LICENSE.txt in the project root for license information.
*/

#ifndef LedDriver_H
#define LedDriver_H

#include <stdio.h>
#include <iostream>
#include <cassert>
#include <cmath>

#include "SampleApp/I2cLib.h"

namespace IS31 {
    // Page register address
    const unsigned char PAGE_REGISTER       = 0xFD;

    // Value for setting the function page
    const unsigned char FUNCTION_PAGE       = 0x0B;

    // Function page register addresses
    const unsigned char MODE_REGISTER       = 0x00;
    const unsigned char FRAME_REGISTER      = 0x01;
    const unsigned char AUTOPLAY1_REGISTER  = 0x02;
    const unsigned char AUTOPLAY2_REGISTER  = 0x03;
    const unsigned char BLINK_REGISTER      = 0x05;
    const unsigned char AUDIOSYNC_REGISTER  = 0x06;
    const unsigned char FRAMESTATE_REGISTER = 0x07;
    const unsigned char BREATH1_REGISTER    = 0x08;
    const unsigned char BREATH2_REGISTER    = 0x09;
    const unsigned char SHUTDOWN_REGISTER   = 0x0A;
    const unsigned char GAIN_REGISTER       = 0x0B;
    const unsigned char ADC_REGISTER        = 0x0C;

    // Operating mode register values
    const unsigned char PICTURE_MODE        = 0x00;
    const unsigned char AUTOPLAY_MODE       = 0x08;
    const unsigned char AUDIOPLAY_MODE      = 0x18;

    // Frame page register address offsets
    const unsigned char ENABLE_OFFSET       = 0x00;
    const unsigned char BLINK_OFFSET        = 0x12;
    const unsigned char PWM_OFFSET          = 0x24;
}

class rgb {
    public:
        // Constructors
        rgb(double red = 0, double green = 0, double blue = 0): r(red), g(green), b(blue) {}
        rgb(const rgb &color) {r = color.r; g = color.g; b = color.b; }

        // Variables
        double r;   // a fraction between 0 and 1
        double g;   // a fraction between 0 and 1
        double b;   // a fraction between 0 and 1
};

class hsv {
    public:
        // Constructors
        hsv(double hue = 0, double saturation = 0, double value = 0): h(hue), s(saturation), v(value) {}
        hsv(const hsv &color) {h = color.h; s = color.s; v = color.v; }

        // Variables
        double h;   // angle in degrees between 0 and 360
        double s;   // a fraction between 0 and 1
        double v;   // a fraction between 0 and 1
};

class LedDriver : private I2cLib {
    public:
        // Constructor
        LedDriver(unsigned char devAddr);

        // Variables
        bool ledDriverOpen;

        // Functions
        int WriteFuncReg(unsigned char regAddr, unsigned char data);
        int ReadFuncReg(unsigned char regAddr, unsigned char *data);
        void SelectFrame(unsigned char frame);
        int WriteFrameReg(unsigned char regAddr, unsigned char data);
        int WriteFrameBuff(unsigned char regAddr, unsigned char *data, unsigned char length);
        void Reset(void);
        void SetMode(unsigned char mode, unsigned char startingFrame = 0);
        void DisplayFrame(unsigned char frame);
        void SetAutoPlay(unsigned char loops, unsigned char frames, unsigned char delay);
        void SetDisplayOptions(unsigned char intensityControl, unsigned char blinkEn, unsigned char blinkPeriod);
        void SetBreathControl(unsigned char enable, unsigned char fadeOutTime, unsigned char fadeInTime, unsigned char extinguishTime);
        void ControlLed(unsigned char index, unsigned char state);
        void ControlBlink(unsigned char index, unsigned char state);
        void ControlPwm(unsigned char index, unsigned char value);
        hsv RgbToHsv(double r, double g, double b);
        hsv RgbToHsv(rgb in);
        rgb HsvToRgb(double h, double s, double v);
        rgb HsvToRgb(hsv in);

    private:
        // Variables
        unsigned char frame;
        unsigned char ledCtrlCache[8][18];
        unsigned char blinkCtrlCache[8][18];

        // Functions
};

#endif
