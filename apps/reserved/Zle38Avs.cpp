/*
* Zle38Avs.cpp  --  ZLE38AVS board Implementation
*
* Copyright 2018 Microsemi Inc. All rights reserved.
* Licensed under the MIT License. See LICENSE.txt in the project root for license information.
*/

#include "SampleApp/Zle38Avs.h"

using namespace std;

/*******************************************************************
 * Constructor
 * *****************************************************************/
Zle38Avs::Zle38Avs(unsigned char devAddr) : LedDriver(devAddr) {

    this->Zle38AvsHbiSetup();
    if (LedDriver::ledDriverOpen) {
        this->Zle38AvsRingSetup();
    }
}

/**********************************************************************
 * Destructor
 * *********************************************************************/
Zle38Avs::~Zle38Avs(void) {

    if (LedDriver::ledDriverOpen) {
        // Display the empty frame 2 to turn off the LEDs
        LedDriver::DisplayFrame(2);
    }

    // Close Vproc driver
    HBI_close(this->hbiHandle);
}

/**********************************************************************
 * Ring setup
 * *********************************************************************/
void Zle38Avs::Zle38AvsRingSetup(void) {

    /*******************************/
    /* Setup the "thinking" frames */
    /* Frames 3, 4, 5              */
    /*******************************/
    unsigned char pattern[] = {0, 0, 1};
    rgb color, darkColor, lightColor;

    // Alternate the two blue with the same intensity
    darkColor = HsvToRgb(this->BG_BLUE, 1, this->FADE_BLUE_V);
    lightColor = HsvToRgb(this->FG_BLUE, 1, this->FADE_BLUE_V);

    // Set frames 3, 4, 5
    // Shift the LEDs by 1 from ring to ring
    for (unsigned char i = 0; i < 3; i++) {
        LedDriver::SelectFrame(3 + i);

        for (unsigned char j = 0; j < this->NB_LEDS; j++) {
            if (pattern[(i + j) % 3] == 0) {
                color = darkColor;
            } else {
                color = lightColor;
            }

            // Set the color
            LedDriver::ControlPwm(this->LED_MAP_ARR[j][0], static_cast<unsigned char>(color.r * 63));
            LedDriver::ControlPwm(this->LED_MAP_ARR[j][1], static_cast<unsigned char>(color.g * 63));
            LedDriver::ControlPwm(this->LED_MAP_ARR[j][2], static_cast<unsigned char>(color.b * 63));

            // Turn the LEDs on
            LedDriver::ControlLed(this->LED_MAP_ARR[j][0], 1);
            LedDriver::ControlLed(this->LED_MAP_ARR[j][1], 1);
            LedDriver::ControlLed(this->LED_MAP_ARR[j][2], 1);
        }
    }

    /******************************/
    /* Setup the "speaking" frame */
    /* Frame 6                    */
    /******************************/
    color = HsvToRgb(this->BG_BLUE, 1, FADE_BLUE_V);

    LedDriver::SelectFrame(6);

    for (unsigned char i = 0; i < this->NB_LEDS; i++) {
        // Set the color
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][0], static_cast<unsigned char>(color.r * 63));
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][1], static_cast<unsigned char>(color.g * 63));
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][2], static_cast<unsigned char>(color.b * 63));

        // Turn the LEDs on
        LedDriver::ControlLed(this->LED_MAP_ARR[i][0], 1);
        LedDriver::ControlLed(this->LED_MAP_ARR[i][1], 1);
        LedDriver::ControlLed(this->LED_MAP_ARR[i][2], 1);
    }

    /***************************************/
    /* Setup the non-DOA (1, 2 mics) frame */
    /* Frame 7                             */
    /***************************************/
    bool light = false;

    // Alternate the two blue to point to all directions
    lightColor = HsvToRgb(this->FG_BLUE, 1, this->FG_BLUE_V);
    darkColor = HsvToRgb(this->FADE_BLUE, 1, this->FADE_BLUE_V);

    LedDriver::SelectFrame(7);

    for (unsigned char i = 0; i < this->NB_LEDS; i++) {
        if (light) {
            color = lightColor;
        } else {
            color = darkColor;
        }
        light ^= true;

        // Set the color
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][0], static_cast<unsigned char>(color.r * 63));
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][1], static_cast<unsigned char>(color.g * 63));
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][2], static_cast<unsigned char>(color.b * 63));

        // Turn the LEDs on
        LedDriver::ControlLed(this->LED_MAP_ARR[i][0], 1);
        LedDriver::ControlLed(this->LED_MAP_ARR[i][1], 1);
        LedDriver::ControlLed(this->LED_MAP_ARR[i][2], 1);
    }
}

/**********************************************************************
 * Vproc SDK setup
 * ********************************************************************/
int Zle38Avs::Zle38AvsHbiSetup(void) {
    hbi_status_t status;
    hbi_dev_cfg_t devConfig;
    user_buffer_t buf[2];

    devConfig.deviceId = 0;
    devConfig.pDevName = NULL;

    status = HBI_open(&this->hbiHandle, &devConfig);
    if(status != HBI_STATUS_SUCCESS) {
        printf("Error - Main(): HBI_open failed (%d)\n", status);
        return -1;
    }

    // Set Timberwolf GPIO8 to output
    // Disable fixed function
    HBI_read(this->hbiHandle, 0x2D8, buf, 2);
    buf[0] &= ~0x01;
    HBI_write(this->hbiHandle, 0x2D8, buf, 2);
    // Soft reset
    buf[0] = 0x00;
    buf[1] = 0x01;
    HBI_write(this->hbiHandle, 0x006, buf, 2);

    //Clear GPIO8
    HBI_read(this->hbiHandle, 0x2DA, buf, 2);
    buf[0] &= ~0x01;
    HBI_write(this->hbiHandle, 0x2DA, buf, 2);

    //Set GPIO8 as output
    HBI_read(this->hbiHandle, 0x2DC, buf, 2);
    buf[0] |= 0x01;
    HBI_write(this->hbiHandle, 0x2DC, buf, 2);

    return 0;
}

/**********************************************************************
 * Display the sound location on the LED ring
 * angle: 0 -> 359
 * ********************************************************************/
void Zle38Avs::SetDoaFrame(unsigned short angle, rgb fg, rgb fade, rgb bg) {
    unsigned char ledIndex, ledTail1, ledTail2;
    unsigned short resolution = 360 / this->NB_LEDS;
    rgb color;
    static unsigned char lastFrame = 0;

    assert(angle < 360);

    // Get the LED index corresponding to the angle rounded to the closest
    ledIndex = static_cast<unsigned char>(((angle + resolution / 2) / resolution) % this->NB_LEDS);
    ledTail1 = (ledIndex + 1) % this->NB_LEDS;
    ledTail2 = (ledIndex + this->NB_LEDS - 1) % this->NB_LEDS;

    // Alternate the frames (0, 1) to prevent twinkle
    lastFrame ^= 1;
    LedDriver::SelectFrame(lastFrame);

    for (unsigned char i = 0; i < this->NB_LEDS; i++) {
        if (i == ledIndex) {
            // Direction indication
            color = fg;
        } else if ((i == ledTail1) || (i == ledTail2)) {
            // Faded tail
            color = fade;
        } else {
            // Background color
            color = bg;
        }

        // Configure the colors
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][0], static_cast<unsigned char>(color.r * 63));
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][1], static_cast<unsigned char>(color.g * 63));
        LedDriver::ControlPwm(this->LED_MAP_ARR[i][2], static_cast<unsigned char>(color.b * 63));
    }

    // Display the newly programmed frame
    LedDriver::DisplayFrame(lastFrame);
}

/**********************************************************************
 * Should run in a thread
 * Init the LEDs and display the DOA read from the Timberwolf
 * ********************************************************************/
void Zle38Avs::Doa(void) {
    rgb fg, fade, bg;
    user_buffer_t buf[4];
    unsigned short angle, backAngle = 1000;
    unsigned long squarePower;
    double soutPower;

    // Enable the LEDs on the two first frames (0, 1)
    for (unsigned char i = 0; i < 2; i++) {
        LedDriver::SelectFrame(i);

        for (unsigned char j = 0; j < this->NB_LEDS; j++) {
            LedDriver::ControlLed(this->LED_MAP_ARR[j][0], 1);
            LedDriver::ControlLed(this->LED_MAP_ARR[j][1], 1);
            LedDriver::ControlLed(this->LED_MAP_ARR[j][2], 1);
        }
    }

    fg = HsvToRgb(this->FG_BLUE, 1, this->FG_BLUE_V);
    fade = HsvToRgb(this->FADE_BLUE, 1, this->FADE_BLUE_V);
    bg = HsvToRgb(this->BG_BLUE, 1, this->BG_BLUE_V);

    // Init the power meter 2 (Sout, 32ms)
    buf[0] = 0x00;
    buf[1] = 0x17;
    HBI_write(this->hbiHandle, 0xE20, buf, 2);
    buf[0] = 0x00;
    buf[1] = 0x0B;
    HBI_write(this->hbiHandle, 0xE22, buf, 2);

    while (this->doaRunning) {
        // Read the number of mics and configuration
        // DOA is only available in 3 mics triangular
        // Allow for a firmware swap in the middle of the DOA
        HBI_read(this->hbiHandle, 0x4C0, buf, 2);

        if (buf[1] == 0x13) {
            // Read the DOA from Timberwolf
            HBI_read(this->hbiHandle, 0x0A0, buf, 2);
            angle = (buf[0] << 8) + buf[1];

            // Read the Sout power (32b)
            HBI_read(this->hbiHandle, 0xEA0, buf, 4);
            squarePower = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];

            // Reset the power meter
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
            buf[3] = 0;
            HBI_write(this->hbiHandle, 0xEA0, buf, 4);

            if (squarePower > 0) {
                soutPower = 10 * log10(squarePower) - 10 * log10(pow(2, 30));
            } else {
                soutPower = - 90;
            }

            // Only update the DOA if the source moved and the sound level is high enough
            // Use "backAngle == 1000" as a marker to identify the first iteration
            if ((soutPower > this->MIN_POWER_DB) || (backAngle == 1000)){
                if (abs(angle - backAngle) > 15) {
                    backAngle = angle;
                    this->SetDoaFrame(angle, fg, fade, bg);
                } else {
                    this_thread::sleep_for (chrono::milliseconds(32));
                }
            } else {
                this_thread::sleep_for (chrono::milliseconds(32));
            }
        } else {
            LedDriver::DisplayFrame(7);
            this_thread::sleep_for (chrono::milliseconds(30));
        }
    }

    // Display the empty frame 2 to turn off the LEDs
    LedDriver::DisplayFrame(2);
}

/**********************************************************************
 * Start the DOA
 * ********************************************************************/
void Zle38Avs::StartDoa(void) {
    user_buffer_t buf[2];

    if (LedDriver::ledDriverOpen) {
        this->doaRunning = true;
        this->doaThread = thread(&Zle38Avs::Doa, this);
    } else {
        // Turn on the LED (GPIO 8)
        HBI_read(this->hbiHandle, 0x2DA, buf, 2);
        buf[0] |= 0x01;
        HBI_write(this->hbiHandle, 0x2DA, buf, 2);
    }
}

/**********************************************************************
 * Stop the DOA
 * ********************************************************************/
void Zle38Avs::StopDoa(void) {
    user_buffer_t buf[2];

    if (LedDriver::ledDriverOpen) {
        this->doaRunning = false;

        // Wait for the thread to finish before exiting
        this->doaThread.join();
    } else {
        // Turn off the LED (GPIO 8)
        HBI_read(this->hbiHandle, 0x2DA, buf, 2);
        buf[0] &= ~0x01;
        HBI_write(this->hbiHandle, 0x2DA, buf, 2);
    }
}

/**********************************************************************
 * Display the "thinking" ring
 * ********************************************************************/
void Zle38Avs::StartThinking(void) {

    if (LedDriver::ledDriverOpen) {
        // Configure the autoplay (endless, 3 frames changing every 99ms)
        LedDriver::SetAutoPlay(0, 3, 9);

        // Start the pattern
        LedDriver::SetMode(IS31::AUTOPLAY_MODE, 3);
    }
}

/**********************************************************************
 * Stop the "thinking" ring
 * ********************************************************************/
void Zle38Avs::StopThinking(void) {

    if (LedDriver::ledDriverOpen) {
        // Go back to default "picture mode"
        LedDriver::SetMode(IS31::PICTURE_MODE);

        // Display the empty frame 2 to turn off the LEDs
        LedDriver::DisplayFrame(2);
    }
}

/**********************************************************************
 * Display the "speaking" ring
 * ********************************************************************/
void Zle38Avs::StartSpeaking(void) {

    if (LedDriver::ledDriverOpen) {
        // Configure the autoplay (enabled, 416ms fade in-out)
        LedDriver::SetBreathControl(1, 4, 4, 0);
        LedDriver::SetAutoPlay(0, 1, 0);

        // Start breathing
        LedDriver::SetMode(IS31::AUTOPLAY_MODE, 6);
    }
}

/**********************************************************************
 * Stop the "speaking" ring
 * ********************************************************************/
void Zle38Avs::StopSpeaking(void) {

    if (LedDriver::ledDriverOpen) {
        // Stop breathing
        LedDriver::SetBreathControl(0, 0, 0, 0);

        // Go back to default "picture mode"
        LedDriver::SetMode(IS31::PICTURE_MODE);

        // Display the empty frame 2 to turn off the LEDs
        LedDriver::DisplayFrame(2);
    }
}
