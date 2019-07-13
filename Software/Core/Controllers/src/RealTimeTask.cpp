/*
 * RealTimeTask.cpp
 *
 *  Created on: July 12, 2019
 *      Author: Quincy Jones
 *
 * Copyright (c) <2019> <Quincy Jones - quincy@implementedrobotics.com/>
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Controllers/RealTimeTask.hpp>

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <assert.h>

#include <iostream>
#include <string>
#include <chrono>

// TODO: Setup CPU Sets properly for affinity if set
// TODO: Add in ZMQ Messaging+Context Passing and Test
// TODO: Add Thread Manager Class to Hold all running threads.  Static Singleton Instance.

namespace Controllers
{
namespace RealTimeControl
{
RealTimeTaskNode::RealTimeTaskNode(const std::string &name,
                                   const long rt_period,
                                   unsigned int rt_priority,
                                   const int rt_core_id,
                                   const unsigned int stack_size) : task_name_(name),
                                                                    rt_period_(rt_period),
                                                                    rt_priority_(rt_priority),
                                                                    rt_core_id_(rt_core_id),
                                                                    stack_size_(stack_size),
                                                                    thread_status_(-1),
                                                                    process_id_(-1)
{
}

void *RealTimeTaskNode::RunTask(void *task_instance)
{
    // TODO: Setup CPU sets, etc.
    RealTimeTaskNode *task = static_cast<RealTimeTaskNode *>(task_instance);
    std::cout << "[RealTimeTaskNode]: "
              << "Starting Task: " << task->task_name_ << std::endl;

    task->process_id_ = getpid();
    // TODO: Add task to future Task Manager here.

    // TODO: Do we want to have support for adding to multiple CPUs here?
    // TODO: Task manager should keep up with which task are pinned to which CPUs.  Could make sure high priority task maybe get pinned to unused cores
    // TODO: Look up max number of CPUs. And make sure core_id falls within that range
    cpu_set_t cpu_set;
    if (task->rt_core_id_ >= 0)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "Setting Thread Affinity to CPU CORE: " << task->rt_core_id_ << std::endl;

        // Clear out CPU set type
        CPU_ZERO(&cpu_set);

        // Set CPU Core to CPU SET
        CPU_SET(task->rt_core_id_, &cpu_set);

        // Set CPU Core Affinity to desired cpu_set
        const int set_result = pthread_setaffinity_np(task->thread_id_, sizeof(cpu_set_t), &cpu_set);
        if (set_result != 0)
        {
            std::cout << "[RealTimeTaskNode]: "
                      << "Failed to set thread affinity: " << set_result << std::endl;
        }

        // Verify it was set successfully
        if (CPU_ISSET(task->rt_core_id_, &cpu_set))
        {
            std::cout << "[RealTimeTaskNode]: "
                      << "Successfully set thread " << task->thread_id_ << " affinity to CORE: " << task->rt_core_id_ << std::endl;
        }
        else
        {
            std::cout << "[RealTimeTaskNode]: "
                      << "Failed to set thread " << task->thread_id_ << " affinity to CORE: " << task->rt_core_id_ << std::endl;
        }
    }

    // Output what the actually task thread affinity is
    const int set_result = pthread_getaffinity_np(task->thread_id_, sizeof(cpu_set_t), &cpu_set);
    if (set_result != 0)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "Failed to get thread affinity: " << set_result << std::endl;
    }

    std::cout << "[RealTimeTaskNode]: " << task->task_name_ << " running on CORES: " << std::endl;
    for (int j = 0; j < CPU_SETSIZE; j++)
    {
        if (CPU_ISSET(j, &cpu_set))
        {
            std::cout << "CPU " << j << std::endl;
        }
    }

    // Setup thread cancellation.
    // TODO: Look into PTHREAD_CANCEL_DEFERRED
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    // TODO:  Check Control deadlines as well.  If run over we can throw exception here
    while (1)
    {
        auto start = std::chrono::high_resolution_clock::now();
        task->Run();
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        long long total_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        //std::cout << "Run Time: " << total_us << std::endl;

        long int remainder = TaskDelay(task->rt_period_ - total_us);
        //std::cout << "Target: " <<  task->rt_period_ - total_us << " Overrun: " << remainder << " Total: " << task->rt_period_ - total_us + remainder << std::endl;
    }
    std::cout << "[RealTimeTaskNode]: "
              << "Ending Task: " << task->task_name_ << std::endl;

    // Stop the task
    task->Stop();
}

int RealTimeTaskNode::Start(void *task_param)
{
    struct sched_param param;
    pthread_attr_t attr;

    // Set Task Thread Paramters
    task_param_ = task_param;

    // Initialize default thread attributes
    thread_status_ = pthread_attr_init(&attr);
    if (thread_status_)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to init attributes!" << std::endl;
        return thread_status_;
    }

    // Set Stack Size
    thread_status_ = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (thread_status_)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to set stack size!" << std::endl;
        return thread_status_;
    }

    // Set Scheduler Policy to RT(SCHED_FIFO)
    thread_status_ = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (thread_status_)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to set schedule policy!" << std::endl;
        return thread_status_;
    }
    // Set Thread Priority
    param.sched_priority = rt_priority_;
    thread_status_ = pthread_attr_setschedparam(&attr, &param);
    if (thread_status_)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to set thread priority!" << std::endl;
        return thread_status_;
    }

    // Use Scheduling Policy from Attributes.
    thread_status_ = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (thread_status_)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to set scheduling policy from attributes!" << std::endl;
        return thread_status_;
    }

    // Create our pthread.  Pass an instance of 'this' class as a parameter
    thread_status_ = pthread_create(&thread_id_, &attr, &RunTask, this);
    if (thread_status_)
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to create thread!" << std::endl;
        return thread_status_;
    }

    // Set as Detached
    thread_status_ = pthread_detach(thread_id_);
    if (thread_status_)
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to detach thread!" << std::endl;
}

void RealTimeTaskNode::Stop()
{
    // TODO: Check for thread running, etc.
    // TODO: Setup signal to inform run task to stop/exit cleanly.  For now we just kill it
    pthread_cancel(thread_id_);
}

long int RealTimeTaskNode::TaskDelay(long int microseconds)
{
    // TODO: Absolute wait or relative?
    // For now we use relative.
    struct timespec delay;
    struct timespec remainder;
    struct timespec ats;

    // Setup Delay Timespec
    delay.tv_sec = 0;
    delay.tv_nsec = microseconds * 1e3;
    if (delay.tv_nsec >= 1000000000) // Wrap Overruns
    {
        delay.tv_nsec -= 1000000000;
        delay.tv_sec++;
    }

    // Get the start time of the delay.  To be used to know over and underruns
    clock_gettime(CLOCK_MONOTONIC, &ats);

    // Add the delay offset to know when this SHOULD end.
    ats.tv_sec += delay.tv_sec;
    ats.tv_nsec += delay.tv_nsec;

    // Sleep for delay
    clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, &remainder);

    // Get the time now after the delay.
    clock_gettime(CLOCK_MONOTONIC, &remainder);

    // Compute difference and calculcate over/underrung
    remainder.tv_sec -= ats.tv_sec;
    remainder.tv_nsec -= ats.tv_nsec;
    if (remainder.tv_nsec < 0) // Wrap nanosecond overruns
    {
        remainder.tv_nsec += 1000000000;
        remainder.tv_sec++;
    }
    return remainder.tv_nsec / 1000;
}

void RealTimeTaskNode::SetTaskFrequency(const unsigned int frequency_hz)
{
    // Frequency must be greater than 0
    assert(frequency_hz > 0);

    // Period in microseconds
    SetTaskPeriod(long((1.0 / frequency_hz) * 1e6));
}
} // namespace RealTimeControl
} // namespace Controllers