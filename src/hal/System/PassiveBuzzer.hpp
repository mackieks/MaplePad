#pragma once

#include <stdint.h>
#include <vector>
#include <list>
#include "pico/time.h"

//! Controls a passive buzzer through PWM
//!
//! All public interfaces are not "thread safe" meaning external locks must be used if being called
//! from multiple cores or interrupts.
//!
//! Since each PWM slice control a pair of outputs, multiple PassiveBuzzers which share the same
//! PWM slice must be set using the same base frequency e.x. gpio 0 and gpio 1 share the same PWM
//! slice which means that PassiveBuzzers created using both of these gpios must be set with the
//! same baseFreqHz in the constructor.
class PassiveBuzzer
{
public:
    //! Describes a buzz using frequency and duty cycle
    struct BuzzProfile
    {
        //! Priority where 0 means to use default and lower value otherwise means higher priority
        uint32_t priority = 0;
        //! Frequency in Hz
        double frequency = 1000.0;
        //! Duty cycle between 0.0 and 1.0
        double dutyCycle = 0.5;
        //! Amount of time to buzz in seconds or negative for infinite
        double seconds = -1.0;
        //! Set to true in order to block until tone is complete
        //! When true, seconds must be positive and buzz cannot be executed within callback context.
        //! All currently executing jobs will be canceled.
        bool blocking = false;
    };

    //! Describes a buzz using PWM values which are dependent on the base frequency
    struct RawBuzzProfile
    {
        //! Priority where 0 means to use default and lower value otherwise means higher priority
        uint32_t priority = 0;
        //! The count in which to repeat PWM cycle
        uint16_t wrapCount = 546;
        //! Number of counts to keep signal high within a PWM cycle (must be less than wrapCount)
        uint16_t highCount = 273;
        //! Amount of time to buzz in seconds or negative for infinite
        double seconds = -1;
        //! Set to true in order to block until tone is complete
        //! When true, seconds must be positive and buzz cannot be executed within callback context.
        //! All currently executing jobs will be canceled.
        bool blocking = false;
    };

    //! Describes an internal buzz job
    struct BuzzJob
    {
        //! The count in which to repeat PWM cycle
        uint16_t wrapCount;
        //! Number of counts to keep signal high within a PWM cycle (must be less than wrapCount)
        uint16_t highCount;
        //! Absolute time when this job is complete
        uint64_t endTimeUs;
    };

    //! Callback definition, called when buzz alarm expires
    typedef void (*BuzzCompleteFn)(PassiveBuzzer* passiveBuzzer, uint32_t priority, const BuzzJob& job);

public:
    //! Constructor
    //! @param[in] gpio  The GPIO number [0,30] associated with the passive buzzer
    //! @param[in] maxPriority  The maximum priority allowed by the instance (min 1)
    //! @param[in] systemFreqHz  The system's clock frequency in Hz
    //! @param[in] baseFreqHz  The base PWM frequency to use
    PassiveBuzzer(uint32_t gpio,
                  uint32_t maxPriority,
                  double systemFreqHz,
                  double baseFreqHz = DEFAULT_BASE_FREQUENCY);

    //! Adjusts internal values with the current system frequency
    //! @warning adjusting this while buzzer is working on jobs will cause incorrect buzz freq
    //! @param[in] systemFreqHz  The system's clock frequency in Hz
    void setSystemFreq(double systemFreqHz);

    //! @returns the base PWM frequency used
    inline double getBaseFreq() { return mBaseFreqHz; }

    //! Sets the default priority when 0 is given for priority (default is 1)
    void setDefaultPriority(uint32_t priority);

    //! Sets the callback which is called when buzz job completes
    //! @warning callback is executed within interrupt context
    //! @param[in] callback  The callback to set
    void setCallback(BuzzCompleteFn callback);

    //! Stops all jobs for the given priority
    //! @param[in] priority  The priority set to stop buzz jobs for
    void stop(uint32_t priority = 0);

    //! Stops all current buzz jobs
    void stopAll();

    //! Execute buzz using BuzzProfile
    //! @param[in] buzzProfile  The profile to execute
    //! @returns true iff job was enqueued, executing, or seconds set to 0
    bool buzz(BuzzProfile buzzProfile);

    //! Execute buzz using RawBuzzProfile
    //! @param[in] rawBuzzProfile  The profile to execute
    //! @returns true iff job was enqueued, executing, or seconds set to 0
    bool buzzRaw(RawBuzzProfile rawBuzzProfile);

private:
    //! Updates the base PWM frequency to use
    //! @warning adjusting this while buzzer is working on jobs will cause incorrect buzz freq
    //! @param[in] baseFreqHz  The base PWM frequency to use
    void setBaseFreq(double baseFreqHz);

    //! Immediately executes a job without any checks
    //! @param[in] priority  The priority of the job
    //! @param[in] job  The job to execute
    //! @param[in] startAlarm  Set to true if alarm should be started when necessary
    //! @returns true iff job was successfully started
    bool runJob(uint32_t priority, BuzzJob job, bool startAlarm);

    //! Computes the job's end time based on given number of seconds
    //! @param[in] seconds  Amount of time to buzz in seconds or negative for infinite
    //! @returns absolute time when job is complete
    uint64_t toJobEndTime(double seconds);

    //! Computes the job's end time based on current time and given number of seconds
    //! @param[in] currentTimeUs  Current time to use
    //! @param[in] seconds  Amount of time to buzz in seconds or negative for infinite
    //! @returns absolute time when job is complete
    uint64_t toJobEndTime(uint64_t currentTimeUs, double seconds);

    //! Enqueue job
    //! @param[in] priority  The priority of the job
    //! @param[in] job  The job to execute
    //! @param[in] pushBack  Set to true to push back or false to push front
    //! @returns true if the job was queued
    //! @returns false if the queue was full and job could not be added
    bool enqueueJob(uint32_t priority, const BuzzJob& job, bool pushBack);

    //! Dequeues and runs the next available job
    //! @param[in] currentTime  The current time in microseconds
    //! @param[in] startAlarm  Set to true if alarm should be started when necessary
    //! @returns the number of microseconds from currentTime that the dequeued job should be stopped
    //! @returns 0 if dequeued job should never be stopped or if no job was dequeued
    uint64_t dequeueNextJob(uint64_t currentTime, bool startAlarm);

    //! Cancels the current job and alarm
    void cancelCurrentJob();

    //! Cancels current job and alarm, and then the buzzer is silenced
    void goIdle();

    //! Callback executed by the alarm - dequeues and runs next job or goes idle
    //! @param[in] id  The ID of the alarm that expired
    //! @returns 0 if the alarm should be canceled
    //! @returns number of microseconds since alarm's expiration which the alarm should be rearmed
    int64_t stopAlarmCallback(alarm_id_t id);

    //! Alarm callback passed to alarm function interfaces
    //! @param[in] id  The ID of the alarm that expired
    //! @param[in] passiveBuzzer  The user data given to alarm function interface which points to an
    //!                           instance of PassiveBuzzer
    //! @returns 0 if the alarm should be canceled
    //! @returns number of microseconds since alarm's expiration which the alarm should be rearmed
    static int64_t stopAlarmCallback(alarm_id_t id, void *passiveBuzzer);

public:
    //! The default base frequency to use when none is given to constructor
    static const double DEFAULT_BASE_FREQUENCY;
    //! The maximum number of jobs to queue for each priority
    static const uint32_t MAX_ITEMS_PER_PRIORITY = 10;

private:
    //! The GPIO number [0,30] associated with the passive buzzer
    const uint32_t mGpio;
    //! The PWM slice associated with the passive buzzer
    const uint32_t mPwmSlice;
    //! The PWM channel associated with the passive buzzer
    const uint32_t mPwmChan;
    //! The default priority to use when 0 is given for a priority
    uint32_t mDefaultPriority;
    //! The system's clock frequency in Hz
    double mSystemFreqHz;
    //! The base PWM frequency to use in Hz
    double mBaseFreqHz;
    //! The PWM divider set in order to go from mSystemFreqHz to mBaseFreqHz
    double mDivider;
    //! The current alarm being executed or 0 if no alarm running
    alarm_id_t mCurrentAlarm;
    //! Set to true while alarm callback is being processed
    bool mAlarmExpired;
    //! The core number executed in alarm stop context
    uint32_t mAlarmCoreNum;
    //! Priority of mWorkingJob or 0 if not currently working on a job
    uint32_t mWorkingPriority;
    //! When mWorkingPriority is not 0, the job that is currently executing on the buzzer
    BuzzJob mWorkingJob;
    //! The queued buzzer jobs, sorted first by priority then order in which they were added
    std::vector<std::list<BuzzJob>> mBuzzJobs;
    //! Callback function called when buzz job completes
    BuzzCompleteFn mCallback;
};
