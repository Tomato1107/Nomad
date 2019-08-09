/*
 * NomadPlant.cpp
 *
 *  Created on: August 5, 2019
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

// Primary Include
#include <Systems/NomadPlant.hpp>

// C System Includes

// C++ System Includes
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>

// Third-Party Includes
#include <Eigen/Dense>

// Project Includes
#include <Realtime/RealTimeTask.hpp>


namespace Systems
{
namespace Nomad
{

NomadPlant::NomadPlant(const std::string &name, const double T_s) : 
                               Realtime::RealTimeTaskNode(name, 20000, Realtime::Priority::MEDIUM, -1, PTHREAD_STACK_MIN), num_states_(13), T_s_(T_s)
{

    // TODO: Should be SET from outside
    // Create Rigid Body
    block_ = RigidBlock1D(20.0, Eigen::Vector3d(1.0, 0.5, 0.25), T_s);

    // TODO: Move to "CONNECT"
    // Create Ports
    // State Estimate Input Port
    input_port_map_[InputPort::FORCES] = std::make_shared<Realtime::Port>("FORCES", Realtime::Port::Direction::INPUT, Realtime::Port::DataType::DOUBLE, 1, rt_period_);

    // Optimal Force Solution Output Port
    output_port_map_[OutputPort::STATE] = std::make_shared<Realtime::Port>("STATE", Realtime::Port::Direction::OUTPUT, Realtime::Port::DataType::DOUBLE, num_states_, rt_period_);

     // Create Messages
    output_state_.length = num_states_;
    output_state_.data.resize(num_states_);

    // Initial State:
    output_state_.data[Idx::X] = 0.0; // X Position
    output_state_.data[Idx::Y] = 0.0; // Y Position
    output_state_.data[Idx::Z] = 0.0; // Z Position
    output_state_.data[Idx::X_DOT] = 0.0; // X Velocity
    output_state_.data[Idx::Y_DOT] = 0.0; // Y Velocity
    output_state_.data[Idx::Z_DOT] = 0.0; // Z Velocity
    output_state_.data[Idx::PHI] = 0.0; // Roll Orientation
    output_state_.data[Idx::THETA] = 0.0; // Pitch Orientation
    output_state_.data[Idx::PSI] = 0.0; // Yaw Orientation
    output_state_.data[Idx::W_X] = 0.0; // Roll Rate
    output_state_.data[Idx::W_Y] = 0.0; // Pitch Rate
    output_state_.data[Idx::W_Z] = 0.0; // Yaw Rate
    output_state_.data[Idx::GRAVITY] = 9.81; // Gravity

    Eigen::VectorXd initial_state(2);
    initial_state[0] = 0.0; // X    
    initial_state[1] = 0.0; // X_DOT

    block_.SetState(initial_state);
}

void NomadPlant::Run()
{
    // Get Inputs
    // std::cout << "Time to RECEIVE in CONVEXMPC" << std::endl;
    // Receive State Estimate and Unpack
    bool force_recv = GetInputPort(InputPort::FORCES)->Receive(forces_in); // Receive State Estimate

    if (force_recv)
    {
        Eigen::VectorXd U = Eigen::Map<Eigen::VectorXd>(forces_in.data.data(), 1);
       // std::cout << "U: " << U << std::endl;
        block_.Step(U);
    }
    else
    {
        std::cout << "[NomadPlant]: Receive Buffer Empty!" << std::endl;
    }
    
    //std::cout << "NOMAD PLANT X: " << block_.GetState() << std::endl;

    output_state_.data[Idx::X] = block_.GetState()[0];
    output_state_.data[Idx::X_DOT] = block_.GetState()[1];


   // std::cout << "NOMAD PLANT X: " << output_state_.data[Idx::X] << std::endl;
   // std::cout << "NOMAD PLANT X DOT: " << output_state_.data[Idx::X_DOT] << std::endl;

    // Output Optimal Forces
    bool send_status = GetOutputPort(OutputPort::STATE)->Send(output_state_);
}

void NomadPlant::Setup()
{
    // Connect Input Ports
    GetInputPort(InputPort::FORCES)->Connect(); // Forces

    // Bind Output Ports
    GetOutputPort(OutputPort::STATE)->Bind(); // Optimal Force Output

    std::cout << "[NomadPlant]: "
              << "NomadPlant Task Node Running!" << std::endl;
}

} // namespace Nomad
} // namespace Systems
