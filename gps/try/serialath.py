import time

port = "/dev/ttyATH0"

#ser = serial.Serial(port,4800)

while 1:
    ser = open(port)
    print (ser.readline())
    ser.close()
    time.sleep(0.1)