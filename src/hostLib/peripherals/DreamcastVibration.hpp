#pragma once

#include "DreamcastPeripheral.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast vibration peripheral
class DreamcastVibration : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] fd  Function definition from the device info for this peripheral
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this vibration device
        DreamcastVibration(uint8_t addr,
                           uint32_t fd,
                           std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                           PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastVibration();

        //! Inherited from DreamcastPeripheral
        virtual void task(uint64_t currentTimeUs) final;

        //! Called when transmission has been sent
        //! @param[in] tx  The transmission that was sent
        virtual void txStarted(std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

        //! Sends vibration
        //! @param[in] timeUs  The time to send vibration (optional)
        //! @param[in] power  Starting power intensity [0,7] (0 to stop vibration)
        //! @param[in] inclination  -1: ramp down, 0: constant, 1: ramp up
        //! @param[in] desiredFreq  When 0: limit pulsation while maximizing for duration
        //!                         Otherwise: The desired pulsation freq value; a lower value will
        //!                         cause more noticeable pulsations [7, 59]
        //! @param[in] durationMs  The length of time in ms to vibrate (0 for minimum pulse)
        void send(uint64_t timeUs, uint8_t power, int8_t inclination, uint8_t desiredFreq, uint32_t durationMs);

        //! Starts indefinite vibration
        //! @param[in] power  Power intensity [1,7]
        //! @param[in] desiredFreq  When 0: set to maximum frequency
        //!                         Otherwise: The desired pulsation freq value; a lower value will
        //!                         cause more noticeable pulsations [7, 59]
        void start(uint8_t power, uint8_t desiredFreq=0);

        //! Immediately stops current vibration
        void stop();

    private:
        //! Computes the number of power increments that will be executed
        //! @param[in] power  Starting power intensity [1,7]
        //! @param[in] inclination  -1: ramp down, 0: constant, 1: ramp up
        //! @returns the number of power increments
        uint8_t computeNumIncrements(uint8_t power, int8_t inclination);

        //! @returns the maximum frequency that can achieve the given duration and # of increments
        uint8_t maxFreqForDuration(uint8_t numIncrements, uint32_t durationMs);

    public:
        //! Function code for vibration
        static const uint32_t FUNCTION_CODE = DEVICE_FN_VIBRATION;
        //! Maximum value for frequency byte
        static const uint8_t MAX_FREQ_VALUE = 0x3B;
        //! Minimum value for frequency byte
        static const uint8_t MIN_FREQ_VALUE = 0x07;
        //! Total number of frequency values
        static const uint32_t NUM_FREQ_VALUES = MAX_FREQ_VALUE - MIN_FREQ_VALUE + 1;
        //! Minimum value for duration byte
        static const uint8_t MIN_CYCLES = 0x00;
        //! Maximum value for duration byte
        static const uint8_t MAX_CYCLES = 0xFF;
        //! Maximum value for power
        static const uint8_t MAX_POWER = 0x07;
        //! Minimum value for power
        static const uint8_t MIN_POWER = 0x01;

    private:
        //! The transmission ID of the last scheduled vibration condition
        uint32_t mTransmissionId;
        //! Initialized to true and set to false on first task execution
        bool mFirst;
        //! Lookup table used to maximize pulsation frequency for a given duration
        static const uint32_t MAX_DURATION_MS_LOOKUP[NUM_FREQ_VALUES];
};
