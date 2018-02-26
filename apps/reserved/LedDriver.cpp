/**********************/
/* IS31FL3731 Wrapper */
/**********************/

#include "SampleApp/LedDriver.h"

using namespace std;

/*******************************************************************
 * Constructor
 * *****************************************************************/
LedDriver::LedDriver(unsigned char devAddr) : I2cLib(devAddr) {
    unsigned char initArr[18] = {0};
    unsigned char temp;

    // Only proceed if the LED driver was successfully opened
    if (I2cLib::ReadReg(0, &temp, false) < 0) {
        this->ledDriverOpen = false;
        return;
    } else {
        this->ledDriverOpen = true;
    }

    // Toggle the software shutdown
    this->Reset();

    // Set to picture mode
    this->SetMode(IS31::PICTURE_MODE, 0);

    // Disable all LEDs, disable blink control, and set brightness to 0 in all frames
    for (unsigned char i = 0; i < 8; i++) {
        this->SelectFrame(i);
        this->WriteFrameBuff(IS31::ENABLE_OFFSET, initArr, 18);
        this->WriteFrameBuff(IS31::BLINK_OFFSET, initArr, 18);
        for (unsigned char j = 0; j < 8; j++) {
            this->WriteFrameBuff(IS31::PWM_OFFSET + j * 18, initArr, 18);
        }
    }

    // Display frame 0
    this->SelectFrame(0);
    this->DisplayFrame(0);

    // Initialize the cache (for all 8 frames)
    for (unsigned char i = 0; i < 8; i++) {
        for (unsigned char j = 0; j < 18; j++) {
            this->ledCtrlCache[i][j] = 0;
            this->blinkCtrlCache[i][j] = 0;
        }
    }
}

/**********************************************************************
 * Write a register of the function page and return to the current
 * frame selection
 * ********************************************************************/
int LedDriver::WriteFuncReg(unsigned char regAddr, unsigned char data) {
    int retVal;

    retVal = I2cLib::WriteReg(IS31::PAGE_REGISTER, IS31::FUNCTION_PAGE);
    if (retVal < 0) return retVal;
    retVal = I2cLib::WriteReg(regAddr, data);
    if (retVal < 0) return retVal;
    retVal = I2cLib::WriteReg(IS31::PAGE_REGISTER, this->frame);

    return retVal;
}

/**********************************************************************
 * Read a register of the function page and return to the current
 * frame selection
 * ********************************************************************/
int LedDriver::ReadFuncReg(unsigned char regAddr, unsigned char *data) {
    int retVal;

    retVal = I2cLib::WriteReg(IS31::PAGE_REGISTER, IS31::FUNCTION_PAGE);
    if (retVal < 0) return retVal;
    retVal = I2cLib::ReadReg(regAddr, data);
    if (retVal < 0) return retVal;
    retVal = I2cLib::WriteReg(IS31::PAGE_REGISTER, this->frame);

    return retVal;
}

/**********************************************************************
 * Select the frame to be written to (0 -> 7)
 * Note: to be called when changing the frame register bank
 * ********************************************************************/
void LedDriver::SelectFrame(unsigned char frame) {

    assert(frame < 8);
    I2cLib::WriteReg(IS31::PAGE_REGISTER, frame);
    this->frame = frame;
}

/**********************************************************************
 * Write a register of the current frame
 * Note: use SelectFrame() to select/change the frame
 * ********************************************************************/
int LedDriver::WriteFrameReg(unsigned char regAddr, unsigned char data) {

    return I2cLib::WriteReg(regAddr, data);
}

/**********************************************************************
 * Write a buffer in the current frame
 * Note: use SelectFrame() to select/change the frame
 * ********************************************************************/
int LedDriver::WriteFrameBuff(unsigned char regAddr, unsigned char *data, unsigned char length) {

    return I2cLib::WriteBuff(regAddr, data, length);
}

/**********************************************************************
 * Maintain the IS31FL3731 in shutdown for 10ms
 * ********************************************************************/
void LedDriver::Reset() {

    this->WriteFuncReg(IS31::SHUTDOWN_REGISTER, 0);
    usleep(10000);
    this->WriteFuncReg(IS31::SHUTDOWN_REGISTER, 1);
}

/**********************************************************************
 * Select the mode for a given frame
 * mode: PICTURE_MODE, AUTOPLAY_MODE, AUDIOPLAY_MODE
 * startingFrame (only for AUTOPLAY_MODE): 0 -> 7
 * ********************************************************************/
void LedDriver::SetMode(unsigned char mode, unsigned char startingFrame) {

    assert(startingFrame < 8);
    this->WriteFuncReg(IS31::MODE_REGISTER, mode | startingFrame);
}

/**********************************************************************
 * Select the active frame to display
 * frame: 0 -> 7
 * ********************************************************************/
void LedDriver::DisplayFrame(unsigned char frame) {

    assert(frame < 8);
    this->WriteFuncReg(IS31::FRAME_REGISTER, frame);
}

/**********************************************************************
 * Loop through a selected number of frames waiting a given delay
 * loops: 0 (endless), 1->7 (# loops then stop)
 * frames: 0->7 (# of frames in the loop)
 * delay: 0 = 11ms, 1->63 x 11ms
 * ********************************************************************/
void LedDriver::SetAutoPlay(unsigned char loops, unsigned char frames, unsigned char delay) {

    assert(loops < 8);
    assert(frames < 8);
    assert(delay < 64);
    this->WriteFuncReg(IS31::AUTOPLAY1_REGISTER, (loops << 4) | frames);
    this->WriteFuncReg(IS31::AUTOPLAY2_REGISTER, delay);
}

/**********************************************************************
 * Loop through a selected number of frames waiting a given delay
 * intensityControl: 0 (frames intensity independent), 1 (frames intensity based on frame 1)
 * blinkEn: 0 (disabled), 1 (enabled)
 * blinkPeriod: 0->7 x 270ms
 * ********************************************************************/
void LedDriver::SetDisplayOptions(unsigned char intensityControl, unsigned char blinkEn, unsigned char blinkPeriod) {

    assert(intensityControl < 2);
    assert(blinkEn < 2);
    assert(blinkPeriod < 8);
    this->WriteFuncReg(IS31::BLINK_REGISTER, (intensityControl << 5) | (blinkEn << 3) | blinkPeriod);
}

/**********************************************************************
 * Loop through a selected number of frames waiting a given delay
 * enable: 0 (disabled), 1 (enabled)
 * fadeOutTime: 0->7 (26ms x 2^fadeOutTime)
 * fadeInTime: 0->7 (26ms x 2^fadeInTime)
 * extinguishTime: 0->7 (3.5ms x 2^extinguishTime)
 * ********************************************************************/
void LedDriver::SetBreathControl(unsigned char enable, unsigned char fadeOutTime, unsigned char fadeInTime, unsigned char extinguishTime) {

    assert(enable < 2);
    assert(fadeOutTime < 8);
    assert(fadeInTime < 8);
    assert(extinguishTime < 8);
    this->WriteFuncReg(IS31::BREATH1_REGISTER, (fadeOutTime << 4) | fadeInTime);
    this->WriteFuncReg(IS31::BREATH2_REGISTER, (enable << 4) | extinguishTime);
}

/**********************************************************************
 * Turn on/off a given LED
 * index: 0->143 (LED index in the matrix)
 * state: 0 (off), 1 (on)
 * ********************************************************************/
void LedDriver::ControlLed(unsigned char index, unsigned char state) {
    unsigned char offset = index / 8;
    unsigned char idx = index % 8;

    assert(index < 144);
    assert(state < 2);
    this->ledCtrlCache[this->frame][offset] = (this->ledCtrlCache[this->frame][offset] & ~(1 << idx)) | (state << idx);
    this->WriteFrameReg(IS31::ENABLE_OFFSET + offset, this->ledCtrlCache[this->frame][offset]);
}

/**********************************************************************
 * Turn on/off the blinker on a given LED
 * index: 0->143 (LED index in the matrix)
 * state: 0 (disabled), 1 (enabled)
 * ********************************************************************/
void LedDriver::ControlBlink(unsigned char index, unsigned char state) {
    unsigned char offset = index / 8;
    unsigned char idx = index % 8;

    assert(index < 144);
    assert(state < 2);
    this->blinkCtrlCache[this->frame][offset] = (this->blinkCtrlCache[this->frame][offset] & ~(1 << idx)) | (state << idx);
    this->WriteFrameReg(IS31::BLINK_OFFSET + offset, this->blinkCtrlCache[this->frame][offset]);
}

/**********************************************************************
 * Control the brightness of a given LED
 * index: 0->143 (LED index in the matrix)
 * value: 0->63 (LED brightness)
 * ********************************************************************/
void LedDriver::ControlPwm(unsigned char index, unsigned char value) {
    const unsigned char gammaTable[] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 10, 12, 14, 16, 18, 20, 22,
        24, 26, 29, 32, 35, 38, 41, 44,
        47, 50, 53, 57, 61, 65, 69, 73,
        77, 81, 85, 89, 94, 99, 104, 109,
        114, 119, 124, 129, 134, 140, 146, 152,
        158, 164, 170, 176, 182, 188, 195, 202,
        209, 216, 223, 230, 237, 244, 251, 255
    };

    assert(index < 144);
    assert(value < 64);
    this->WriteFrameReg(IS31::PWM_OFFSET + index, gammaTable[value]);
}

/**********************************************************************
 * Convert a color from RGB to HSV
 * r, g, b: 0.0->1.0
 * ********************************************************************/
hsv LedDriver::RgbToHsv(double r, double g, double b) {
    rgb in(r, g, b);

    return RgbToHsv(in);
}

hsv LedDriver::RgbToHsv(rgb in) {
    hsv out;
    double min, max, delta;

    assert((in.r >= 0.0) && (in.r <= 1.0));
    assert((in.g >= 0.0) && (in.g <= 1.0));
    assert((in.b >= 0.0) && (in.b <= 1.0));

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;
    delta = max - min;
    if (delta < 0.00001) {
        out.s = 0;
        out.h = NAN;
        return out;
    }

    if (max > 0.0) {
        out.s = delta / max;
    } else {
        out.s = 0.0;
        out.h = NAN;
        return out;
    }

    if (in.r >= max) {
        out.h = (in.g - in.b )/ delta;          // between yellow & magenta
    } else if (in.g >= max) {
        out.h = 2.0 + (in.b - in.r) / delta;    // between cyan & yellow
    } else {
        out.h = 4.0 + (in.r - in.g) / delta;    // between magenta & cyan
    }

    out.h *= 60.0;

    if (out.h < 0.0) {
        out.h += 360.0;
    }

    return out;
}

/**********************************************************************
 * Convert a color from HSV to RGB
 * h: 0.0->359.0
 * s, v: 0.0->1.0
 * ********************************************************************/
rgb LedDriver::HsvToRgb(double h, double s, double v) {
    hsv in(h, s, v);

    return HsvToRgb(in);
}

rgb LedDriver::HsvToRgb(hsv in) {
    double hh, p, q, t, ff;
    long i;
    rgb out;

    assert((in.s >= 0.0) && (in.s <= 1.0));
    assert((in.v >= 0.0) && (in.v <= 1.0));

    if (in.s <= 0.0) {
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }

    hh = in.h;
    while (hh >= 360.0) {
        hh -= 360.0;
    }
    hh /= 60.0;
    i = static_cast<long>(hh);

    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
        case 0:
            out.r = in.v;
            out.g = t;
            out.b = p;
            break;

        case 1:
            out.r = q;
            out.g = in.v;
            out.b = p;
            break;

        case 2:
            out.r = p;
            out.g = in.v;
            out.b = t;
            break;

        case 3:
            out.r = p;
            out.g = q;
            out.b = in.v;
            break;

        case 4:
            out.r = t;
            out.g = p;
            out.b = in.v;
            break;

        default:
            out.r = in.v;
            out.g = p;
            out.b = q;
            break;
    }

    return out;
}
