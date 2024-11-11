#ifndef __MAPLE_BUS_INTERFACE_H__
#define __MAPLE_BUS_INTERFACE_H__

#include <stdint.h>
#include <memory>
#include "configuration.h"
#include "utils.h"
#include "MaplePacket.hpp"

//! Maple Bus interface class
class MapleBusInterface
{
    public:
        //! Enumerates the phase in the state machine
        enum class Phase : uint8_t
        {
            //! Initialized phase and phase after completion and events are processed
            IDLE = 0,
            //! Write is currently in progress
            WRITE_IN_PROGRESS,
            //! Write has failed (impulse response used only as a result of processing events)
            WRITE_FAILED,
            //! Write completed and no read was expected
            WRITE_COMPLETE,
            //! Write completed, waiting for start sequence from device
            WAITING_FOR_READ_START,
            //! Currently waiting for response
            READ_IN_PROGRESS,
            //! Read has failed (impulse response used only as a result of processing events)
            READ_FAILED,
            //! Write and read cycle completed
            READ_COMPLETE,
            //! Initialized value
            INVALID
        };

        //! Enumerates different types of read/write errors
        enum class FailureReason : uint8_t
        {
            //! No error
            NONE = 0,
            //! CRC doesn't match computed value
            CRC_INVALID,
            //! Received less data than expected
            MISSING_DATA,
            //! Read DMA buffer overflowed
            BUFFER_OVERFLOW,
            //! Timeout occurred before data could be fully written or read
            TIMEOUT
        };

        //! Status due to processing events (see MapleBusInterface::processEvents)
        struct Status
        {
            //! The phase of the state machine
            Phase phase;
            //! Set to failure reason when phase is WRITE_FAILED or READ_FAILED
            FailureReason failureReason;
            //! A pointer to the bytes read or nullptr if no new data available
            const uint32_t* readBuffer;
            //! The number of words received or 0 if no new data available
            uint32_t readBufferLen;

            Status() :
                phase(Phase::INVALID),
                readBuffer(nullptr),
                readBufferLen(0)
            {}
        };

    public:
        //! Virtual desturctor
        virtual ~MapleBusInterface() {}

        //! Writes a packet to the maple bus
        //! @post processEvents() must periodically be called to check status
        //! @param[in] packet  The packet to send (sender address will be overloaded)
        //! @param[in] autostartRead  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When autostartRead is true, the read timeout to set
        //! @returns true iff the bus was "open" and send has started
        virtual bool write(const MaplePacket& packet,
                           bool autostartRead,
                           uint64_t readTimeoutUs=MAPLE_RESPONSE_TIMEOUT_US) = 0;

        //! Begins waiting for input
        //! @post processEvents() must periodically be called to check status
        //! @note This is NOT meant to be called if bus is setup as a host
        //! @note Keep in mind that the maple_in state machine doesn't  sample the full end
        //!       sequence. The application side should wait a sufficient amount of time after bus
        //!       goes neutral before responding in that case. Waiting for neutral bus within
        //!       write() may be enough though (as long as MAPLE_OPEN_LINE_CHECK_TIME_US is set to
        //!       at least 2).
        //! @param[in] readTimeoutUs  Minimum number of microseconds to read for (optional)
        //! @returns true iff bus was not busy and read started
        virtual bool startRead(uint64_t readTimeoutUs=std::numeric_limits<uint64_t>::max()) = 0;

        //! Processes timing events for the current time. This should be called before any write
        //! call in order to check timeouts and clear out any used resources.
        //! @param[in] currentTimeUs  The current time to process for
        //! @returns updated status since last call
        virtual Status processEvents(uint64_t currentTimeUs) = 0;

        //! @returns true iff the bus is currently busy reading or writing.
        virtual bool isBusy() = 0;
};

//! Creates a maple bus
//! @param[in] pinA  GPIO index for pin A. The very next GPIO will be designated as pin B.
extern std::shared_ptr<MapleBusInterface> create_maple_bus(uint32_t pinA, int32_t dirPin = -1, bool dirOutHigh = true);

#endif // __MAPLE_BUS_INTERFACE_H__
