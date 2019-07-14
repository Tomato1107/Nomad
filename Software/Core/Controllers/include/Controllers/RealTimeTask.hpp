/*
 * RealTimeTask.hpp
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

// C Includes
#include <limits.h>
#include <pthread.h>
#include <sched.h>

// C++ Includes
#include <iostream>
#include <string>
#include <map>

// Third Party Includes
#include <zmq.hpp>

#ifndef NOMAD_CORE_CONTROLLERS_REALTIMETASK_H_
#define NOMAD_CORE_CONTROLLERS_REALTIMETASK_H_

namespace Controllers
{
namespace RealTimeControl
{
enum Priority
{
    LOWEST = 99,
    LOW = 80,
    MEDIUM = 50,
    HIGH = 20,
    HIGHEST = 1
};

class RealTimeTaskNode
{
    friend class RealTimeTaskManager;

public:
    // Base Class Real Time Task Node
    // name = Task Name
    // rt_period = Task Execution Period (microseconds), default = 10000uS/100hz
    // rt_priority = Task Thread Priority
    // rt_core_id = CPU Core to pin the task.  -1 for no affinity
    // stack_size = Task Thread Stack Size
    RealTimeTaskNode(const std::string &name = "RT_Task",
                     const long rt_period = 10000,
                     const unsigned int rt_priority = Priority::MEDIUM,
                     const int rt_core_id = -1,
                     const unsigned int stack_size = PTHREAD_STACK_MIN);

    ~RealTimeTaskNode();

    // Task Start
    int Start(void *task_param = NULL);

    // Task Stop
    void Stop();

    // Set Task Name
    void SetTaskName(const std::string &name) { task_name_ = name; }

    // Set Stack Size
    void SetStackSize(const unsigned int stack_size) { stack_size_ = stack_size; }

    // Set Task Priority
    void SetTaskPriority(const unsigned int priority) { rt_priority_ = priority; }

    // Set Task Frequency (Convenience to set a "rate" in HZ)
    void SetTaskFrequency(const unsigned int freqeuncy_hz);

    // Set Task Period (Microseconds)
    void SetTaskPeriod(const long period) { rt_period_ = period; }

    // Set CPU Core Affinity
    void SetCoreAffinity(const int core_id) { rt_core_id_ = core_id; }

protected:
    // Override Me for thread function
    virtual void Run() = 0;

    // Using ZMQ for thread sync and message passing

    // ZMQ Context
    zmq::context_t *context_;

    // ZMQ Socket
    zmq::socket_t *socket_;

    // ZMQ Transport
    // TODO: Need a enum for types, i.e. TCP, UDP, IPC, INPROC
    // TODO: Also a port int
    // TODO: Also a Socket Type
    // TODO: Also Queue/Message Options, i.e. HWM and CONFLATE
    std::string transport_;

private:
    // STATIC Task Delay
    static long int TaskDelay(long int microseconds);

    // STATIC Member Task Run
    static void *RunTask(void *task_instance);

    // Task Name
    std::string task_name_;

    // Stack Size
    unsigned int stack_size_;

    // Task Priority (0 to 99)
    unsigned int rt_priority_;

    // Task Period (microseconds)
    long rt_period_;

    // Task CPU Affinboolity/CoreID
    int rt_core_id_;

    // Thread ID
    pthread_t thread_id_;

    // Process ID
    pid_t process_id_;

    // Thread Status
    int thread_status_;

    // Task Parameter
    void *task_param_;
};

class RealTimeTaskManager
{

public:
    // Base Class Real Time Task Manager
    RealTimeTaskManager();

    // STATIC Singleton Instance
    static RealTimeTaskManager *Instance();

    int GetCPUCount() { return cpu_count_; }

    bool AddTask(RealTimeTaskNode *task);
    bool EndTask(RealTimeTaskNode *task);
    bool EndTask(const std::string &name);
    void PrintActiveTasks();

    const zmq::context_t *GetZMQContext() const { return context_; }

protected:
    // Using ZMQ for thread sync and message passing
    // ZMQ Context
    // Thread inproc messaging must share a single context.  We put it in the singleton here to keep it unique;
    zmq::context_t *context_;

private:
    // Singleton Instance
    static RealTimeTaskManager *manager_instance_;

    // Vector to hold running tasks.  Assumed to not be THAT many running task.  Hence the std::vector
    std::vector<RealTimeTaskNode *> task_map_;

    // Max numbers of CPUs
    int cpu_count_;
};
} // namespace RealTimeControl
} // namespace Controllers

#endif // NOMAD_CORE_CONTROLLERS_REALTIMETASK_H_
