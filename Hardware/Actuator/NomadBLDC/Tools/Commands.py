"""Nomad Class Handler for Interaction with NomadBLDC Board"""

#  Commands.py
#
#  Created on: March 20, 2020
#      Author: Quincy Jones
# 
#  Copyright (c) <2020> <Quincy Jones - quincy@implementedrobotics.com/>
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the Software
#  is furnished to do so, subject to the following conditions:
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


from enum import IntEnum
import struct
import threading
from dataclasses import dataclass

#from collections import namedtuple
#from typing import NamedTuple

#@dataclass
#class CommandPacket:
#    self.length: int = None
#    self.command: int = None

@dataclass
class LogPacket:
    log_length: int = None
    log_string: str = None

    @classmethod
    def unpack(cls, data):
        return struct.unpack(f"<{data[0]}s", data[1:])[0].decode("utf-8")

@dataclass
class DeviceStats:
    __fmt = "<BBIffff"
    fault: int = None
    control_status: int = None
    uptime: int = None
    voltage_bus: float = None
    driver_temp: float = None
    fet_temp: float = None
    motor_temp: float = None

    @classmethod
    def unpack(cls, data):
        unpacked = struct.unpack(cls.__fmt, data)
        return DeviceStats(*unpacked)

@dataclass
class DeviceInfo:
    fw_major: int = None
    fw_minor: int = None
    device_id: 'typing.Any' = None



class CommandID(IntEnum):
    # Info/Status Commands
    DEVICE_INFO_READ = 1
    DEVICE_STATS_READ = 2
    DEVICE_CONFIG_READ = 3

    # Configuration Commands
    MEASURE_RESISTANCE = 4
    MEASURE_INDUCTANCE = 5
    MEASURE_PHASE_ORDER = 6

    CALIBRATE_MOTOR = 7
    ZERO_POSITION = 8

    # Motor Control Commands
    ENABLE_CURRENT_CONTROL = 9
    ENABLE_VOLTAGE_CONTROL = 10
    ENABLE_TORQUE_CONTROL  = 11
    ENABLE_SPEED_CONTROL   = 12
    ENABLE_IDLE_MODE       = 13

    # Device Control Commands
    DEVICE_RESTART = 14
    DEVICE_ABORT   = 15

    
    # Set Points
    SEND_VOLTAGE_SETPOINT = 16
    SEND_TORQUE_SETPOINT = 17

    # Status
    LOGGING_OUTPUT = 100

class CommandHandler:
    def __init__(self):
        self.timeout = 10

        # Events
        self.device_info_received = threading.Event()
        self.device_stats_received = threading.Event()
        self.device_stats = 0
        self.logger_cb = None
    
    def set_logger_cb(self, cb):
        self.logger_cb = cb 

    # Measure Motor Resistance
    def measure_motor_resistance(self, transport, calib_voltage=15, calib_current=3):
        command_packet = bytearray(struct.pack("<BB", CommandID.MEASURE_RESISTANCE, 0))
        transport.send_packet(command_packet)
        return True

    # Calibrate motor
    def calibrate_motor(self, transport):
        command_packet = bytearray(struct.pack("<BB", CommandID.CALIBRATE_MOTOR, 0))
        transport.send_packet(command_packet)
        return True
    
    # Restart device
    def restart_device(self, transport):
        command_packet = bytearray(struct.pack("<BB", CommandID.DEVICE_RESTART, 0))
        transport.send_packet(command_packet)
        return True

    # Voltage Control Mode
    def start_voltage_control(self, transport):
        command_packet = bytearray(struct.pack("<BB", CommandID.ENABLE_VOLTAGE_CONTROL, 0))
        transport.send_packet(command_packet)
        return True

    # Torque Control Mode
    def start_torque_control(self, transport):
        command_packet = bytearray(struct.pack("<BB", CommandID.ENABLE_TORQUE_CONTROL, 0))
        transport.send_packet(command_packet)
        return True

    # Enter Idle Mode
    def enter_idle_mode(self, transport):
        command_packet = bytearray(struct.pack("<BB", CommandID.ENABLE_IDLE_MODE, 0))
        transport.send_packet(command_packet)
        return True

    def set_voltage_setpoint(self, transport, v_d, v_q):
        command_packet = bytearray(struct.pack("<BBff", CommandID.SEND_VOLTAGE_SETPOINT, 8, v_d, v_q))
        transport.send_packet(command_packet)
        return True
        
    def set_torque_setpoint(self, transport, k_p, k_d, pos, vel, tau_ff):
        command_packet = bytearray(struct.pack("<BBff", CommandID.SEND_TORQUE_SETPOINT, 8, .99, 0))
        transport.send_packet(command_packet)
        return True

    # Device Stats
    def get_device_stats(self, transport):

        command_packet = bytearray(struct.pack("<BB", CommandID.DEVICE_STATS_READ, 0))
        transport.send_packet(command_packet)

        # Wait for device response
        self.device_stats_received.wait(5)
        if(self.device_stats_received.is_set()):
            self.device_stats_received.clear() # Clear Flag
            return self.device_stats

        return None

    # Read device info.  Blocking.
    def read_device_info(self, transport):
        command_packet = bytearray(struct.pack("<BB", CommandID.DEVICE_INFO_READ, 0))
        transport.send_packet(command_packet)

        # Wait for device response
        self.device_info_received.wait(5)
        if(self.device_info_received.is_set()):
            self.device_info_received.clear() # Clear Flag
            return self.device_info
        
        return None

    def process_packet(self, packet):
        comm_id = packet[0]
        if(comm_id == CommandID.DEVICE_INFO_READ):
            self.device_info = DeviceInfo()
            self.device_info.fw_major = packet[2]
            self.device_info.fw_minor = packet[3]
            self.device_info.device_id = packet[4:16].hex()
            self.device_info_received.set()

        elif(comm_id == CommandID.DEVICE_STATS_READ):
            #print(f"Length: {len(packet[2:])}")
            self.device_stats = DeviceStats.unpack(packet[2:])
            #print(self.device_stats)
            self.device_stats_received.set()

        elif(comm_id == CommandID.LOGGING_OUTPUT):
            if(self.logger_cb is not None):
                log_string = struct.unpack(f"<{packet[1]}s", packet[2:])[0].decode("utf-8")
                self.logger_cb(log_string)


        # TODO: Measurement completes
        # Unpack packet
        # Find


