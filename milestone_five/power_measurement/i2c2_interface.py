#Mitch Davis
#ECE4534 Group 6
#
#BusPirate - I2C2 Utility

import serial
import array
import io

#Serial Port Initialization for bus pirate
#
# serial_path is assumed to be a valid serial port
# there is a way to do some data validation using
# the serial module, but I am lazy
#
# If an invalid serial path is specified, the program
# will abort (but not nicely)
def config_serial_port( serial_path ):
	#1s timeout is very important for readline operation
	serial_device = serial.Serial( serial_path, 115200, timeout=1)
	if serial_device.isOpen():
		print "Serial Device Configured"
		print serial_device.name
	else:
		print "Serial Device Configuration FAILED"

	return serial_device

#Serial port closing
def close_serial( serial_device ):
	serial_device.close()
	if ~serial_device.isOpen():
		print "Serial Device Closed"
	else:
		print "Serial Devisc Close FAIL"

#Configure Bus Pirate device for I2C
#
#
# We assume that the BP was just powered on
# An already-powered state could be handled.
def configure_bus_pirate( serial_device ):
	#Wake up bus pirate
	serial_device.write("\n")
	bootup_success = 0
	for i in range(20):
		message = serial_device.readline()
		#Some sophisticated things would catch the BP already
		#initialized in any state
		if message == "Bus Pirate v3a\r\n":
			print "Bus Pirate Detected!"
			bootup_success = -1
		elif (bootup_success == -1) & (message == "HiZ>"):
			bootup_success = 1
			serial_device.flush()
			serial_device.write("m\n") #Change mode
			serial_device.write("4\n") #I2C
			serial_device.write("4\n") #400 kHz for fast data
			return bootup_success
	return bootup_success

#Calibrate the I2C2 Board
def calibrate_i2c2( serial_device, slave_address ):
	#If you want more sample points, change this number
	data_points = 100

	#Instructions
	print "Configure DUT for NORMAL mode"
	print "Connect the I2C2 device to the DUT"
	print "Measure Voltage from MASTER I2C +5V to VSS"
	voltage = raw_input(">")
	
	print "I2C2 Jumper LOAD ENABLE"
	print "Press enter to begin calibration."
	tmp = raw_input(">")
	load_current = 0

	#Capture 8mA test current
	serial_device.flushInput()
	serial_device.flushOutput()
	serial_device.write("[" + hex(slave_address) + " 0x00 [" +hex(slave_address+1) +" r:"+str(2*data_points)+"]\n")
	while 1:
		message = serial_device.readline()
		if 'READ' in message:
			load_data = message
			break
		elif 'I2C>' in message:
			load_data = 0
			break
	if load_data == 0:
		print "I2C Read Failed"
		return (0,0,0)

	#Split and average data
	load_data = load_data.split()
	del load_data[0] #Remove 'READ:'
	load_value = 0 
	for i in range(data_points):
		#Check that we're getting ACKs
		if (load_data[i*4+1] == "ACK"):
			load_value = load_value + int( load_data[4*i], 16 ) * 256 + int( load_data[4*i+2], 16 )
		else:
			return (0,0,0)
	load_value = load_value/data_points

	print "Remove LOAD ENABLE Jumper"
	print "Press enter to continue calibration."
	tmp = raw_input(">")

	#Capture no-load ADC values
	serial_device.flushInput()
	serial_device.flushOutput()
	serial_device.write("[" + hex(slave_address) + " 0x00 [" +hex(slave_address+1) +" r:"+str(2*data_points)+"]\n")
	while 1:
		message = serial_device.readline()
		if 'READ' in message:
			zero_data = message
			break
		elif 'I2C>' in message:
			zero_data = 0
			break
	if zero_data == 0:
		print "I2C Read Failed"
		return (0,0,0)

	#Split and average data
	zero_data = zero_data.split()
	del zero_data[0] #Remove 'READ:'
	zero_value = 0
	for i in range(data_points):
		if zero_data[4*i+1] == "ACK":
			zero_value = zero_value + int( zero_data[4*i], 16 ) * 256 + int( zero_data[4*i+2], 16 )
		else:
			return (0,0,0)
	zero_value = zero_value/data_points


	slope = (load_value - zero_value)/8
	return (voltage,zero_value,slope)


#Main event loop
def main():
	print "Enter path to serial device:"
	serial_path = raw_input(">")
	serial_device = config_serial_port(serial_path)
	if configure_bus_pirate(serial_device) == True:
		print "Successful Bus Pirate configuration"
	else:
		print "Bus Pirate configuration FAILED!"
		return

	print "I2C2 Address (write address in hex: 0x90)"
	ascii_hex = raw_input(">")

	if ("0x" in ascii_hex):
		slave_address = int(ascii_hex, 16)
	else:
		print "Bad Address!"
		serial_device.close()
		return

	(voltage,zero_value,slope) = calibrate_i2c2( serial_device, slave_address )
	print str(voltage) + "V <---> " + str(slope) + " ADC units per mA, zero = " + str(zero_value) + "  " + hex(zero_value)
	serial_device.close()

if __name__ == "__main__":
	main()
