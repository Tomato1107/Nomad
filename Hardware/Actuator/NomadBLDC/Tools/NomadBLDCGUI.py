from PyQt5 import QtWidgets, QtCore, uic
import sys
from NomadBLDC import NomadBLDC

def Connect():
    nomad = NomadBLDC()
    nomad.connect()

#import sys
#from NomadBLDC import NomadBLDC

class NomadBLDCGUI(QtWidgets.QMainWindow):
    def __init__(self):
        super(NomadBLDCGUI, self).__init__()
        uic.loadUi("NomadBLDCGUI.ui", self) 
        self.InitWindow()
        

        self.nomad_dev = NomadBLDC()
        self.connectButton.clicked.connect(self.ConnectDevice)
        self.calibrateButton.clicked.connect(self.CalibrateDevice)
        self.restartButton.clicked.connect(self.RestartDevice)
        self.voltageControlButton.clicked.connect(self.StartVoltageControl)
        self.torqueControlButton.clicked.connect(self.StartTorqueControl)
        self.idleModeButton.clicked.connect(self.EnterIdleMode)

        self.voltageControlSlider.valueChanged.connect(self.SetVoltageSetPoint)
        self.k_p_spin.valueChanged.connect(self.SetTorqueSetPoint)
        self.k_d_spin.valueChanged.connect(self.SetTorqueSetPoint)
        self.pos_spin.valueChanged.connect(self.SetTorqueSetPoint)
        self.vel_spin.valueChanged.connect(self.SetTorqueSetPoint)
        self.torqueFF_spin.valueChanged.connect(self.SetTorqueSetPoint)


    def InitWindow(self):
        self.statusBar().showMessage('Not Connected')
        self.show()

    def ConnectDevice(self):
        if(self.nomad_dev.connect()): # Connected
            self.statusBar().showMessage(f'NomadBLDC Device Connected: {self.nomad_dev.port}')
    
    def CalibrateDevice(self):
        self.nomad_dev.calibrate_motor()

    def RestartDevice(self):
        self.nomad_dev.restart_device()
    
    def StartVoltageControl(self):
        self.nomad_dev.start_voltage_control()
    
    def StartTorqueControl(self):
        self.nomad_dev.start_torque_control()
        #self.timer = QtCore.QTimer()
        #self.timer.timeout.connect(self.SetTorqueSetPoint)
        #self.timer.start(100)
        # Begin Update Timer

    def EnterIdleMode(self):
        self.nomad_dev.enter_idle_mode()

    def SetVoltageSetPoint(self):
        max_voltage = 5.0
        v_q = max_voltage * self.voltageControlSlider.value()/1000.0
        self.nomad_dev.set_voltage_setpoint(0, v_q) 
    
    def SetTorqueSetPoint(self):
        self.nomad_dev.set_torque_setpoint(self.k_p_spin.value(), self.k_d_spin.value(), self.pos_spin.value(), self.vel_spin.value(), self.torqueFF_spin.value()) 

    def closeEvent(self, event):
        self.nomad_dev.disconnect()


0
if __name__ == '__main__':

    app = QtWidgets.QApplication([])
    window = NomadBLDCGUI()
    sys.exit(app.exec())




