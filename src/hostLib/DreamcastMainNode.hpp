#pragma once

#include "DreamcastNode.hpp"
#include "DreamcastSubNode.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"
#include "DreamcastPeripheral.hpp"
#include "TransmissionTimeliner.hpp"

#include <memory>
#include <vector>

//! Handles communication for the main Dreamcast node for a single bus. In other words, this
//! facilitates communication to test for and identify a main peripheral such as a controller and
//! routes received communication to that peripheral or a sub node under this.
class DreamcastMainNode : public DreamcastNode
{
    public:
        //! Constructor
        //! @param[in] bus  The bus on which this node communicates
        //! @param[in] playerData  The player data passed to any connected peripheral
        DreamcastMainNode(MapleBusInterface& bus,
                          PlayerData playerData,
                          std::shared_ptr<PrioritizedTxScheduler> prioritizedTxScheduler);

        //! Virtual destructor
        virtual ~DreamcastMainNode();

        //! Inherited from DreamcastNode
        virtual void task(uint64_t currentTimeUs) final;

        //! Inherited from DreamcastNode
        virtual inline void txStarted(std::shared_ptr<const Transmission> tx) final
        {}

        //! Inherited from DreamcastNode
        virtual inline void txFailed(bool writeFailed,
                                     bool readFailed,
                                     std::shared_ptr<const Transmission> tx) final
        {}

        //! Inherited from DreamcastNode
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

        //! Called when the main peripheral needs to be disconnected
        //! @param[in] currentTimeUs  The current time as number of microseconds
        void disconnectMainPeripheral(uint64_t currentTimeUs);

    private:
        //! Execute and process read task from the timeliner
        //! @param[in] currentTimeUs  The current time in microseconds
        void readTask(uint64_t currentTimeUs);

        //! Run task of all of my dependents
        //! @param[in] currentTimeUs  The current time in microseconds
        void runDependentTasks(uint64_t currentTimeUs);

        //! Execute and process write task from the timeliner
        //! @param[in] currentTimeUs  The current time in microseconds
        void writeTask(uint64_t currentTimeUs);

        //! Adds an auto reload info request to the transmission schedule
        void addInfoRequestToSchedule(uint64_t currentTimeUs = 0);

    public:
        //! Number of microseconds in between each info request when no peripheral is detected
        static const uint32_t US_PER_CHECK = 16000;
        //! Number of communication failures before main peripheral is disconnected
        static const uint32_t MAX_FAILURE_DISCONNECT_COUNT = 3;

    protected:
        //! The sub nodes under this node
        std::vector<std::shared_ptr<DreamcastSubNode>> mSubNodes;
        //! Executes transmissions from the schedule
        TransmissionTimeliner mTransmissionTimeliner;
        //! ID of the device info request auto reload transmission this object added to the schedule
        int64_t mScheduleId;
        //! Current count of number of communication failures
        uint32_t mCommFailCount;
};