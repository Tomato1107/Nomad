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
#include <map>

// TODO: Setup CPU Sets properly for affinity if set
// TODO: Add in ZMQ Messaging+Context Passing and Test

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
    // Add to task manager
    RealTimeTaskManager::Instance()->AddTask(this);

    // Reserve Ports
    input_port_map_.reserve(MAX_PORTS);
    output_port_map_.reserve(MAX_PORTS);
}

RealTimeTaskNode::~RealTimeTaskNode()
{
    // Remove from task manager
    RealTimeTaskManager::Instance()->EndTask(this);
}

void *RealTimeTaskNode::RunTask(void *task_instance)
{
    RealTimeTaskNode *task = static_cast<RealTimeTaskNode *>(task_instance);
    std::cout << "[RealTimeTaskNode]: "
              << "Starting Task: " << task->task_name_ << std::endl;

    task->process_id_ = getpid();

    // TODO: Do we want to have support for adding to multiple CPUs here?
    // TODO: Task manager keeps up with which task are pinned to which CPUs.  Could make sure high priority task maybe get pinned to unused cores?
    cpu_set_t cpu_set;
    if (task->rt_core_id_ >= 0 && task->rt_core_id_ < RealTimeTaskManager::Instance()->GetCPUCount())
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
    else if (task->rt_core_id_ >= RealTimeTaskManager::Instance()->GetCPUCount())
    {
        std::cout << "[RealTimeTaskNode]: " << task->task_name_
                  << "\tERROR.  Desired CPU Affinity exceeds number of available cores!" << std::endl
                  << "Please check system configuration." << std::endl;
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

    // Call Setup
    task->Setup();

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
    {
        std::cout << "[RealTimeTaskNode]: "
                  << "POSIX Thread failed to detach thread!" << std::endl;
        return thread_status_;
    }
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

// Get Output Port
Port *RealTimeTaskNode::GetOutputPort(const int port_id) const
{
    assert(port_id >= 0 && port_id < MAX_PORTS);
    return output_port_map_[port_id];
}

// Get Input Port
Port *RealTimeTaskNode::GetInputPort(const int port_id) const
{
    assert(port_id >= 0 && port_id < MAX_PORTS);
    return input_port_map_[port_id];
}

// TODO: Add a "type" (TCP/UDP, THREAD ETC)
void RealTimeTaskNode::SetPortOutput(const int port_id, const std::string &path)
{
    assert(port_id >= 0 && port_id < MAX_PORTS);
    output_port_map_[port_id]->SetTransport("inproc://" + path); // TODO: Prefix depends on type/port.  For now INPROC only.
}

// Task Manager Source

// Global static pointer used to ensure a single instance of the class.
RealTimeTaskManager *RealTimeTaskManager::manager_instance_ = NULL;

RealTimeTaskManager::RealTimeTaskManager()
{
    // ZMQ Context
    context_ = new zmq::context_t(1);

    // Get CPU Count
    cpu_count_ = sysconf(_SC_NPROCESSORS_ONLN);

    std::cout << "[RealTimeTaskManager]: Task manager RUNNING.  Total Number of CPUS available: " << cpu_count_ << std::endl;
}

RealTimeTaskManager *RealTimeTaskManager::Instance()
{
    if (manager_instance_ == NULL)
    {
        manager_instance_ = new RealTimeTaskManager();
    }
    return manager_instance_;
}

bool RealTimeTaskManager::AddTask(RealTimeTaskNode *task)
{
    assert(task != NULL);

    for (int i = 0; i < task_map_.size(); i++)
    {
        if (task_map_[i] == task)
        {
            std::cout << "[RealTimeTaskManager]: Task " << task->task_name_ << " already exists." << std::endl;
            return false;
        }
    }

    // TODO: Why does this not work with pointers properly?
    //std::vector<RealTimeTaskNode* >::iterator it;

    //std::find(task_map_.begin(), task_map_.end(), task);
    //if(it != task_map_.end())
    //{
    //   printf("FOUND: %p\n", *it);
    //   std::cout << "[RealTimeTaskManager]: Task " << task->task_name_ << " already exists." << std::endl;
    //   return false;
    //}
    task_map_.push_back(task);

    std::cout << "[RealTimeTaskManager]: Task " << task->task_name_ << " successfully added." << std::endl;
    return true;
}

bool RealTimeTaskManager::EndTask(RealTimeTaskNode *task)
{
    assert(task != NULL);

    for (int i = 0; i < task_map_.size(); i++)
    {
        if (task_map_[i] == task)
        {
            task->Stop();
            task_map_.erase(task_map_.begin() + i);
            std::cout << "[RealTimeTaskManager]: Task " << task->task_name_ << " successfully removed" << std::endl;
            return true;
        }
    }

    // std::vector<RealTimeTaskNode *>::iterator it;
    // std::find(task_map_.begin(), task_map_.end(), task);
    // if (it != task_map_.end())
    // {
    //     task->Stop();
    //     task_map_.erase(it);

    //     // TODO: Should we clean up memory here?
    //     std::cout << "[RealTimeTaskManager]: Task " << task->task_name_ << " successfully removed" << std::endl;
    //     return true;
    // }

    std::cout << "[RealTimeTaskManager]: No Task " << task->task_name_ << " currently running" << std::endl;
    return false;
}

void RealTimeTaskManager::PrintActiveTasks()
{
    for (auto task : task_map_)
    {
        std::cout << "[RealTimeTaskManager]: Task: " << task->task_name_ << "\tPriority: " << task->rt_priority_ << "\tCPU Affinity: " << task->rt_core_id_ << std::endl;
    }
}

Port::Port(const std::string &name, zmq::context_t *ctx, const std::string &transport, int period) : name_(name),
                                                                       context_(ctx),
                                                                       transport_(transport),
                                                                       update_period_(period)
{
}
Port::~Port()
{
}

bool Port::Map(Port *input, Port *output)
{
    input->transport_ = output->transport_;
}

// Connect Port
bool Port::Connect()
{
    // TODO: For now always a subscriber
    socket_ = new zmq::socket_t(*context_, ZMQ_SUB);

    // Keep only most recent message.  Drop all others from state estimator publisher
    socket_->setsockopt(ZMQ_CONFLATE, 1);

    std::cout<<"Connecting: " << transport_ << std::endl;
    // Connect to Publisher
    socket_->connect(transport_);

    // TODO: Topics later?
    // Setup Message Filter(None)
    socket_->setsockopt(ZMQ_SUBSCRIBE, "", 0);
}

// Bind Port
bool Port::Bind()
{
    // TODO: For now always a publisher
    socket_ = new zmq::socket_t(*context_, ZMQ_PUB);
    socket_->bind(transport_);
}

// Send data on port
bool Port::Send(zmq::message_t &tx_msg, int flags)
{
    // TODO: Zero Copy Publish
    socket_->send(tx_msg);
    return true;
}
bool Port::Send(void *buffer, const unsigned int length, int flags)
{
    zmq::message_t message(length);
    memcpy(message.data(), buffer, length);
    return Send(message, flags);
}

// Receive data on port
bool Port::Receive(zmq::message_t &rx_msg, int flags)
{
    socket_->recv(&rx_msg);
    return true;
}
bool Port::Receive(void *buffer, const unsigned int length, int flags)
{
    zmq::message_t rx_msg;
    Receive(rx_msg); // Receive State Estimate
    memcpy(buffer, rx_msg.data(), rx_msg.size());
    return true;
}
} // namespace RealTimeControl
} // namespace Controllers