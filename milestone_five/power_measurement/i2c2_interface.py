from pyserial import *
from array import *
from io import *

#Serial Port Initialization for bus pirate
#
def config_serial_port( serial_path )
	serial_device = serial.Serial( serial_path, 115200, timeout=1)
	if serial_device.isOpen()
		print "Serial Device Configured"
		print serial_device.name
	else
		print "Serial Device Configuration FAILED"

	return serial_device

#Serial port closing
def close_serial( serial_device )
	serial_device.close()
	if !serial_device.isOpen()
		print "Serial Device Closed"
	else
		print "Serial Devisc Close FAIL"

#Configure Bus Pirate device for I2C
def configure_bus_pirate( serial_device )
	#Wake up bus pirate
	serial_device.write("\n")
	serial_device.flush()
	bootup_success = false
	for i = 1 to 7
		message = serial_device.readline()
		if message == "Bus Pirate v3a"
			bootup_success = true
	
			serial_device.write("m")
			serial_device.write("4")
			serial_device.write("3")
			serial_device.flush()
			message = serial_device.readline()
			if message != "Ready\n"
				print "Initialization FALED"
				bootup_success = false

	return bootup_success

#Calibrate the I2C2 Board
def calibrate_i2c2( serial_device, slave_address )
	print "Configure DUT for NORMAL mode"
	print "Connect the I2C2 device to the DUT"
	print "Measure Voltage from MASTER I2C +5V to VSS"
	voltage = raw_input(">")
	print "Jumper LOAD ENABLE"
	print "Press enter to begin calibration."
	tmp = raw_input(">")
	#
	#
	#load_current = stuff
	#Add read code here
	print "Remove LOAD ENABLE Jumper"
	print "Press enter to continue calibration."
	tmp = raw_input(">")
	#
	#zero_current = stuff
	#More reads

	zero = zero_current
	slope = (load_current - zero_current)/8

	return (zero,slope)
