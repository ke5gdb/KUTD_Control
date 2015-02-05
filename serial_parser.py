#!/usr/bin/python

#
# (C) 2015 Andrew Koenig, KE5GDB - Project Engineer, KUTD-FM
#

import sys
import os
import serial

ser = serial.Serial('/dev/ttyUSB1', 9600, timeout=1)

# Telemetry Arrays - [value, coefficient, offset]
#
# Example: 
#
# A -> D
# y=mx+b where m=var[1] and b=var[2]
#
# D -> A
# x=(y-b)/m where m=var[1] and b=var[2]
#
# The value stored is NOT the raw reading
#

paCurrent = [0, 48.4, 159.4]
paVoltage = [0, 82, 5]
rfForward = [0, 33.125, 285.75]
rfReflected = [0, 217, 131.5]
compression = [0, 18, 424]

def is_number(s):
	try:
		int(s)
		return True
	except ValueError:
		return False

try:
	while(1):
		line = ser.readline()
		if (line != "") & (is_number(line[:3])):
			os.system("clear")
			paCurrent[0] = round((float(line.split(",")[0]) - paCurrent[2])/paCurrent[1], 3)
			paVoltage[0] = round((float(line.split(",")[1]) - paVoltage[2])/paVoltage[1], 3)
			rfForward[0] = round((float(line.split(",")[2]) - rfForward[2])/rfForward[1], 3)
			rfReflected[0] = round((float(line.split(",")[3]) - rfReflected[2])/rfReflected[1], 3)
			compression[0] = round((float(line.split(",")[4]) - compression[2])/compression[1], 3)

			if(paCurrent[0] < 0):
				paCurrent[0] = 0
			if(paVoltage[0] < 0):
				paVoltage[0] = 0
			if(rfForward[0] < 0):
				rfForward[0] = 0
			if(rfReflected[0] < 0):
				rfReflected[0] = 0
			if(compression[0] < 0):
				compression[0] = 0


			print "PA I:   " + str(paCurrent[0])
			print "PA V:   " + str(paVoltage[0])
			print "RF FWD: " + str(rfForward[0])
			print "RF REF: " + str(rfReflected[0])
			print "G/R:    " + str(compression[0])
			#print ("PA I = " + paCurrent + ", PA V = " + paVoltage + ", RF FWD = " + rfForward + ", RF REF = " + rfReflected + ", G/R" + compression),
			#print line
		else:
			print line,

except KeyboardInterrupt:
	print "Exiting..."
	ser.close()
