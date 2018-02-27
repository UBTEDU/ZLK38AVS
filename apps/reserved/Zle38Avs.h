/*
* Zle38Avs.h  --  ZLE38AVS board Header
*
* Copyright 2018 Microsemi Inc. All rights reserved.
* Licensed under the MIT License. See LICENSE.txt in the project root for license information.
*/

#ifndef ZLE38AVS_H
#define ZLE38AVS_H

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>

#include "SampleApp/LedDriver.h"

// Vproc SDK includes
#define VPROC_MAX_NUM_DEVS 3
extern "C" {
    #include "typedefs.h"
    #include "chip.h"
    #include "hbi.h"
}

/*******************************/
/*     -= Frame usage =-       */
/*                             */
/* DOA:         0, 1           */
/* Blank:       2              */
/* Thinking:    3, 4, 5        */
/* Speaking:    6              */
/* Non-DOA:     7              */
/*******************************/

class Zle38Avs : private LedDriver {
    public:
        // Constructor, Destructor
        Zle38Avs(unsigned char devAddr = 0x74);
        ~Zle38Avs(void);

        // Functions
        void StartDoa(void);
        void StopDoa(void);
        void StartThinking(void);
        void StopThinking(void);
        void StartSpeaking(void);
        void StopSpeaking(void);

    private:
        // Defines
        static const unsigned char NB_LEDS = 12;
        static const unsigned short FG_BLUE = 180;
        static constexpr double FG_BLUE_V = 1.0;
        static const unsigned short BG_BLUE = 210;
        static constexpr double BG_BLUE_V = 0.1;
        static const unsigned short FADE_BLUE = (FG_BLUE + BG_BLUE) / 2;
        static constexpr double FADE_BLUE_V = 0.4;
        static constexpr double MIN_POWER_DB = -60;

        // const 2D vector uniformly initialized
        const std::vector<std::vector<unsigned char> > LED_MAP_ARR {
            {98, 130, 114}, {96, 128, 112},
            {54, 86, 70}, {52, 84, 68},
            {50, 82, 66}, {48, 80, 64},
            {6, 38, 22}, {4, 36, 20},
            {2, 34, 18}, {0, 32, 16}, 
            {102, 134, 118}, {100, 132, 116}
        };

        // Variables
        bool doaRunning;
        std::thread doaThread;
        hbi_handle_t hbiHandle;

        // Functions
        void Zle38AvsRingSetup(void);
        int Zle38AvsHbiSetup(void);
        void SetDoaFrame(unsigned short angle, rgb fg, rgb fade, rgb bg);
        void Doa(void);
};

#endif