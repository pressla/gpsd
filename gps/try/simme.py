
import time,socket



class simme:
    def __init__(self):
        self.delay=1
        self.readers = 0
        self.index = 0

    def write(self, line):
        "Throw an error if this superclass is ever instantiated."
        raise ValueError( line)

    def add_checksum(self, str):
        "Concatenate NMEA checksum and trailer to a string"
        sum = 0
        for (i, c) in enumerate(str):
            if i == 0 and c == "$":
                continue
            sum ^= ord(c)
        str += "*%02X\r\n" % sum
        return str

    def feed(self):
        "Feed a line from the contents of the GPS log to the daemon."
        line = "$CCMWV,5,T,0.0,N,A"
        line=line+self.add_checksum(line)+"\r\n"
        time.sleep(int(self.delay))
        # self.write has to be set by the derived class
        self.write(line)
        self.index += 1

class simTCP(simme):
    "A TCP serverlet with a test log ready to be cycled to it."
    def __init__(self, testload,
                 host, port,
                 progress=None):
        simme.__init__(self)
        self.host = host
        self.port = int(port)
        self.byname = "tcp://" + host + ":" + str(port)
        self.dispatcher = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # This magic prevents "Address already in use" errors after
        # we release the socket.
        self.dispatcher.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.dispatcher.bind((self.host, self.port))
        self.dispatcher.listen(5)
        self.readables = [self.dispatcher]

    def read(self):
        "Handle connection requests and data."
        readable, _writable, _errored = select.select(self.readables, [], [], 0)
        for s in readable:
            if s == self.dispatcher:	# Connection request
                client_socket, _address = s.accept()
                self.readables = [client_socket]
                self.dispatcher.close()
            else:			# Incoming data
                data = s.recv(1024)
                if not data:
                    s.close()
                    self.readables.remove(s)

    def write(self, line):
        "Send the next log packet to everybody connected."
        for s in self.readables:
            if s != self.dispatcher:
                s.send(line)

    def drain(self):
        "Wait for the associated device(s) to drain (e.g. before closing)."
        for s in self.readables:
            if s != self.dispatcher:
                s.shutdown(socket.SHUT_RDWR)

class simUDP(simme):
    "A UDP broadcaster with a test log ready to be cycled to it."
    def __init__(self, testload,
                 ipaddr, port,
                 progress=None):
        simme.__init__(self)
        self.ipaddr = ipaddr
        self.port = port
        self.byname = "udp://" + ipaddr + ":" + str(port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def read(self):
        "Discard control strings written by gpsd."
        pass

    def write(self, line):
        #self.bline =
        self.sock.sendto(line.encode(), (self.ipaddr, self.port))

    def drain(self):
        "Wait for the associated device to drain (e.g. before closing)."
        pass	# shutdown() fails on UDP

if __name__ == '__main__':
    #os.system('clear') #clear the terminal (optional)

    sim = simUDP("",'192.168.10.1',7000)
    try:
        AWAmtop = 0
        AWSmtop=0
        HDMmtop=0
        TEMPmtop=0
        ROLLmtop=0
        PTCHmtop=0

        while True:
            #os.system('clear')
            sim.feed()

    except KeyboardInterrupt:
        # Avoid garble on ^C
        print ("")
