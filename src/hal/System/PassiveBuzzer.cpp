#include "PassiveBuzzer.hpp"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

#include <cmath>

// Somewhat arbitrary, but this will allow for handling all audible frequencies
const double PassiveBuzzer::DEFAULT_BASE_FREQUENCY = 500000.0;

PassiveBuzzer::PassiveBuzzer(uint32_t gpio,
                             uint32_t maxPriority,
                             double systemFreqHz,
                             double baseFreqHz) :
    mGpio(gpio),
    mPwmSlice(pwm_gpio_to_slice_num(gpio)),
    mPwmChan(pwm_gpio_to_channel(gpio)),
    mDefaultPriority(1),
    mSystemFreqHz(systemFreqHz),
    mBaseFreqHz(baseFreqHz),
    mDivider(),
    mCurrentAlarm(0),
    mAlarmExpired(false),
    mAlarmCoreNum(0),
    mWorkingPriority(0),
    mWorkingJob(),
    mBuzzJobs(),
    mCallback(nullptr)
{
    gpio_init(mGpio);
    gpio_put(mGpio, false);
    gpio_set_dir(mGpio, true);
    gpio_set_function(mGpio, GPIO_FUNC_PWM);

    if (maxPriority <= 0)
    {
        maxPriority = 1;
    }
    mBuzzJobs.resize(maxPriority);

    setBaseFreq(baseFreqHz);

    // The initial value here doesn't matter - just setting to something
    pwm_set_wrap(mPwmSlice, 100);
    // Setting channel level to 0 for all channels essentially silences the output
    // This is done because there is sometimes a delay when using pwm_set_enabled
    assert(mPwmChan == 0 || mPwmChan == 1);
    pwm_set_chan_level(mPwmSlice, 0, 0);
    pwm_set_chan_level(mPwmSlice, 1, 0);
    // PWM is always enabled
    pwm_set_enabled(mPwmSlice, true);
}

void PassiveBuzzer::setSystemFreq(double systemFreqHz)
{
    mSystemFreqHz = systemFreqHz;
    setBaseFreq(mBaseFreqHz);
}

void PassiveBuzzer::setDefaultPriority(uint32_t priority)
{
    assert(priority <= mBuzzJobs.size());
    mDefaultPriority = priority;
}

void PassiveBuzzer::setCallback(BuzzCompleteFn callback)
{
    mCallback = callback;
}

void PassiveBuzzer::setBaseFreq(double baseFreqHz)
{
    mBaseFreqHz = baseFreqHz;
    mDivider = mSystemFreqHz / mBaseFreqHz;

    pwm_set_clkdiv(mPwmSlice, mDivider);
}

void PassiveBuzzer::stop(uint32_t priority)
{
    if (priority == 0)
    {
        priority = mDefaultPriority;
    }

    assert(priority <= mBuzzJobs.size());

    // For simplicity, disable all interrupts so that an alarm cannot interrupt while dequeueing
    uint32_t interruptStatus = save_and_disable_interrupts();

    mBuzzJobs[priority - 1].clear();

    if (mWorkingPriority == priority)
    {
        // Cancel the alarm if it was set
        if (mCurrentAlarm != 0)
        {
            cancel_alarm(mCurrentAlarm);
        }
        mCurrentAlarm = 0;
        // Stop this job by dequeueing next
        dequeueNextJob(time_us_64(), true);
    }

    restore_interrupts(interruptStatus);
}

void PassiveBuzzer::stopAll()
{
    // Stop current buzz
    goIdle();

    // Clear all queued items
    for (std::vector<std::list<BuzzJob>>::iterator pIter = mBuzzJobs.begin();
         pIter != mBuzzJobs.end();
         ++pIter)
    {
        pIter->clear();
    }
}

bool PassiveBuzzer::buzz(BuzzProfile buzzProfile)
{
    uint16_t wrap = std::ceil(mBaseFreqHz / buzzProfile.frequency) - 1;
    RawBuzzProfile rawBuzzProfile = {
        .priority = buzzProfile.priority,
        .wrapCount = wrap,
        .highCount = static_cast<uint16_t>(wrap * buzzProfile.dutyCycle),
        .seconds = buzzProfile.seconds,
        .blocking = buzzProfile.blocking
    };
    return buzzRaw(rawBuzzProfile);
}

bool PassiveBuzzer::buzzRaw(RawBuzzProfile rawBuzzProfile)
{
    if (rawBuzzProfile.priority == 0)
    {
        rawBuzzProfile.priority = mDefaultPriority;
    }

    assert(rawBuzzProfile.priority <= mBuzzJobs.size());

    if (rawBuzzProfile.seconds == 0)
    {
        // Do nothing
        return true;
    }

    BuzzJob job = {
        .wrapCount = rawBuzzProfile.wrapCount,
        .highCount = rawBuzzProfile.highCount
        // .endTimeUs to be computed later
    };

    if (mAlarmExpired && mAlarmCoreNum == get_core_num())
    {
        // buzz() was called within callback context
        // The callback should still be in the process of clearing out the current job
        assert(mWorkingPriority > 0);
        assert(mCurrentAlarm > 0);

        // Compute end time from the completed job's end time
        job.endTimeUs = toJobEndTime(mWorkingJob.endTimeUs, rawBuzzProfile.seconds);

        // Enqueue and exit
        enqueueJob(rawBuzzProfile.priority, job, true);
        return true;
    }
    else if (rawBuzzProfile.blocking)
    {
        // Stop everything
        stopAll();

        // Execute in line
        pwm_set_chan_level(mPwmSlice, mPwmChan, 0);
        pwm_set_wrap(mPwmSlice, rawBuzzProfile.wrapCount);
        pwm_set_chan_level(mPwmSlice, mPwmChan, rawBuzzProfile.highCount);
        sleep_ms(rawBuzzProfile.seconds * 1000);
        // Setting channel level to 0 silences PWM
        pwm_set_chan_level(mPwmSlice, mPwmChan, 0);

        // Done
        return true;
    }

    // For simplicity, disable all interrupts so that an alarm cannot interrupt while queueing
    uint32_t interruptStatus = save_and_disable_interrupts();

    // Compute end time from now
    job.endTimeUs = toJobEndTime(rawBuzzProfile.seconds);

    if (mWorkingPriority > 0)
    {
        if (mWorkingPriority <= rawBuzzProfile.priority)
        {
            // Currently working on higher priority item or item currently stopping
            // Queue the job and exit
            bool enqueued = enqueueJob(rawBuzzProfile.priority, job, true);

            // All done here
            restore_interrupts(interruptStatus);
            return enqueued;
        }
        else
        {
            // Enqueue the working job (in front) to allow the given job to run
            (void)enqueueJob(mWorkingPriority, mWorkingJob, false); // This shouldn't fail
            cancelCurrentJob();
        }
    }

    // Done with critical section
    restore_interrupts(interruptStatus);

    return runJob(rawBuzzProfile.priority, job, true);
}

bool PassiveBuzzer::runJob(uint32_t priority, BuzzJob job, bool startAlarm)
{
    // Start this job
    mWorkingPriority = priority;
    mWorkingJob = job;

    pwm_set_chan_level(mPwmSlice, mPwmChan, 0);
    pwm_set_wrap(mPwmSlice, mWorkingJob.wrapCount);
    pwm_set_chan_level(mPwmSlice, mPwmChan, mWorkingJob.highCount);

    if (job.endTimeUs < UINT64_MAX && startAlarm)
    {
        // schedule off
        alarm_id_t id = add_alarm_at(from_us_since_boot(job.endTimeUs), stopAlarmCallback, this, true);
        if (id < 0)
        {
            // Failure
            return false;
        }
        else if (id > 0)
        {
            mCurrentAlarm = id;
        }
    }
    // else: buzz until manually stopped

    return true;
}

uint64_t PassiveBuzzer::toJobEndTime(double seconds)
{
    return toJobEndTime(time_us_64(), seconds);
}

uint64_t PassiveBuzzer::toJobEndTime(uint64_t currentTimeUs, double seconds)
{
    if (seconds < 0)
    {
        // Infinite
        return UINT64_MAX;
    }
    else
    {
        return (currentTimeUs + (seconds * 1000000));
    }
}

bool PassiveBuzzer::enqueueJob(uint32_t priority, const BuzzJob& job, bool pushBack)
{
    std::list<BuzzJob>& priorityList = mBuzzJobs[priority - 1];
    if (pushBack)
    {
        // Only add job if there is room
        if (priorityList.size() >= MAX_ITEMS_PER_PRIORITY)
        {
            // See if stale jobs can be popped
            uint64_t currentTimeUs = time_us_64();
            while (priorityList.size() >= MAX_ITEMS_PER_PRIORITY
                && priorityList.front().endTimeUs <= currentTimeUs)
            {
                priorityList.pop_front();
            }
        }

        if (priorityList.size() < MAX_ITEMS_PER_PRIORITY)
        {
            priorityList.push_back(job);
            return true;
        }
    }
    else
    {
        // Make room for the job before pushing to front
        while (priorityList.size() >= MAX_ITEMS_PER_PRIORITY)
        {
            priorityList.pop_back();
        }
        priorityList.push_front(mWorkingJob);
        return true;
    }

    return false;
}

uint64_t PassiveBuzzer::dequeueNextJob(uint64_t currentTime, bool startAlarm)
{
    uint32_t priority = 1;
    for (std::vector<std::list<BuzzJob>>::iterator pIter = mBuzzJobs.begin();
         pIter != mBuzzJobs.end();
         ++pIter, ++priority)
    {
        if (!pIter->empty())
        {
            BuzzJob front = pIter->front();
            pIter->pop_front();
            if (currentTime < front.endTimeUs)
            {
                if (runJob(priority, front, startAlarm))
                {
                    if (front.endTimeUs == UINT64_MAX)
                    {
                        // No alarm to execute for this
                        return 0;
                    }
                    else
                    {
                        return (front.endTimeUs - currentTime);
                    }
                }
            }
        }
    }

    // Nothing run
    goIdle();
    return 0;
}

void PassiveBuzzer::cancelCurrentJob()
{
    if (mCurrentAlarm != 0)
    {
        cancel_alarm(mCurrentAlarm);
    }
    mCurrentAlarm = 0;
    mWorkingPriority = 0;
}

void PassiveBuzzer::goIdle()
{
    cancelCurrentJob();
    // Setting channel level to 0 silences PWM
    pwm_set_chan_level(mPwmSlice, mPwmChan, 0);
}

int64_t PassiveBuzzer::stopAlarmCallback(alarm_id_t id)
{
    if (id != mCurrentAlarm)
    {
        // Error
        return 0;
    }

    // If the callback adds a new job, it must be queued instead of run
    mAlarmCoreNum = get_core_num();
    mAlarmExpired = true;

    if (mCallback)
    {
        mCallback(this, mWorkingPriority, mWorkingJob);
    }

    mAlarmExpired = false;

    // Determine what the current time is
    uint64_t currentTime = 0;
    if (mWorkingPriority > 0)
    {
        // Use the current job's end time as current time
        currentTime = mWorkingJob.endTimeUs;
    }
    else
    {
        currentTime = time_us_64();
    }

    uint64_t val = dequeueNextJob(currentTime, false);
    if (val == 0)
    {
        // Nothing more to do; alarm will be deleted on exit
        mCurrentAlarm = 0;
    }

    return val;
}

int64_t PassiveBuzzer::stopAlarmCallback(alarm_id_t id, void *passiveBuzzer)
{
    return static_cast<PassiveBuzzer*>(passiveBuzzer)->stopAlarmCallback(id);
}
