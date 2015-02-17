#!/usr/bin/python

#
# (C) 2015 Andrew Koenig, KE5GDB - Project Engineer, KUTD-FM
#

import sys
import os
import serial
import time
import SocketServer
import json

ser = serial.Serial('/dev/ttyUSB1', 9600, timeout=1)

def is_number(s):
	try:
		int(s)
		return True
	except ValueError:
		return False

class MyTCPServer(SocketServer.ThreadingTCPServer):
	allow_reuse_address = True

class MyTCPServerHandler(SocketServer.BaseRequestHandler):
	def handle(self):
		data = json.loads(self.request.recv(1024).strip())
		print data
		#self.request.sendall(json.dumps({'return':'ok'}))

		try:
			while(1):
				line = ser.readline()
				if (line != "") & (is_number(line[:1])):
					os.system("clear")
					paCurrent = line.split(",")[0]
					paVoltage = line.split(",")[1]
					rfForward = line.split(",")[2]
					rfReflected = line.split(",")[3]
					compression = line.split(",")[4]

					if(paCurrent < 0):
						paCurrent = 0
					if(paVoltage < 0):
						paVoltage = 0
					if(rfForward < 0):
						rfForward = 0
					if(rfReflected < 0):
						rfReflected = 0
					if(compression < 0):
						compression = 0


					print "PA I:   " + str(paCurrent)
					print "PA V:   " + str(paVoltage)
					print "RF FWD: " + str(rfForward)
					print "RF REF: " + str(rfReflected)
					print "G/R:    " + str(compression)
					
					self.request.sendall(json.dumps({'return':'ok'}))
					self.request.sendall(json.dumps({'Compression':compression}))

				else:
					print line,

		except KeyboardInterrupt:
			print "Exiting..."
			ser.close()

server = MyTCPServer(('127.0.0.1', 13373), MyTCPServerHandler)
server.serve_forever()


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


