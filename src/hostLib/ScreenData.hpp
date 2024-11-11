#pragma once

#include "hal/System/MutexInterface.hpp"
#include <stdint.h>

//! Contains monochrome screen data
//! A screen is 48 bits wide and 32 bits tall
class ScreenData
{
    public:
        //! Constructor
        //! @param[in] mutex  Reference to the mutex to use (critical section mutex recommended)
        ScreenData(MutexInterface& mutex);

        //! Set the screen bits
        //! @param[in] data  Screen words to set
        //! @param[in] startIndex  Starting screen word index (left to right, top to bottom)
        //! @param[in] numWords  Number of words to write
        void setData(uint32_t* data, uint32_t startIndex=0, uint32_t numWords=NUM_SCREEN_WORDS);

        //! @returns true if new data is available since last call to readData
        bool isNewDataAvailable() const;

        //! Copies screen data to the given array
        //! @param[out] out  The array to write to (must be at least 48 words in length)
        void readData(uint32_t* out);

    public:
        //! Number of words in a screen
        static const uint32_t NUM_SCREEN_WORDS = 48;

    private:
        //! Mutex used to ensure integrity of data between multiple cores
        MutexInterface& mMutex;
        //! The current screen data
        uint32_t mScreenData[NUM_SCREEN_WORDS];
        //! Flag set to true in setData and set to false in readData
        bool mNewDataAvailable;
};
