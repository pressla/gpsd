#! /usr/bin/python
# Written by Alex Pressl Jan 2018
# License: GPL 2.0



import time,microclient


class GpsPoller(gpscommon):
    #session = None
    #mode = WATCH_JSON
    def __init__(self, host="127.0.0.1", port=GPSD_PORT, verbose=2, mode=0):
        print ("hhhho",host)
        gpscommon.__init__(self, host, port, verbose)
        #self.session = gpscommon(host="192.168.10.1", verbose=2, port=GPSD_PORT)

        if mode:
            self.stream(mode)


    def run(self):
        i = 0
        while self.running:
            res = self.next()  # this will continue to loop and grab EACH set of gpsd info to clear
            print
            "run:", i, "  ", res
            i = i + 1
            # time.sleep(1) 

    def next(self):
        if self.read() == -1:
            raise StopIteration
        return self.response

    def stream(self, flags=0):
        "Ask gpsd to stream reports at your client."
        arg=""
        if (flags & (WATCH_JSON | WATCH_NMEA | WATCH_RAW)) == 0:
            flags |= WATCH_JSON
        return self.send(arg,flags)


def getOps():
    import getopt, sys

    (options, arguments) = getopt.getopt(sys.argv[1:], "v")
    streaming = False
    verbose = 2
    for (switch, val) in options:
        if switch == '-v':
            verbose = 2
    if len(arguments) > 2:
        print ('Usage: rotmast.py [-v] [host [port]]')
        sys.exit(1)

    opts = {"verbose": verbose}
    if len(arguments) > 0:
        opts["host"] = arguments[0]
    if len(arguments) > 1:
        opts["port"] = arguments[1]
    return opts

if __name__ == '__main__':
    #os.system('clear') #clear the terminal (optional)
    opts = getOps()
    gpsp = GpsPoller(**opts)  # create the interface object
    print (gpsp.stream(WATCH_ENABLE))
    try:
        AWAmtop = 0
        AWSmtop=0
        HDMmtop=0
        TEMPmtop=0
        ROLLmtop=0
        PTCHmtop=0

        while True:
            #os.system('clear')
            strres = gpsp.next()
            if len(strres) > 1:

                cmd = strres.split(",")
                if (cmd[0]=="$IIMWV") & (cmd[2]=="R"):
                    AWAmtop = float(cmd[1])
                    AWSmtop = float(cmd[3])
                elif (cmd[0] == "$HCHDM"):
                    HDMmtop = float(cmd[1])
                elif (cmd[0] == "$IIXDR"):
                    if (cmd[4][:4]=="PTCH"):
                        PTCHmtop = float(cmd[2])
                    elif (cmd[4][:4]=="ROLL"):
                        ROLLmtop = float(cmd[2])
                elif (cmd[0] == "$IIMTA"):
                    TEMPmtop = float(cmd[1])

                else:
                    print ("tranche: ",cmd)

                print ("------------------------------")
                print ("AWAmtop:\t",AWAmtop)
                print ("AWSmtop:\t",AWSmtop)
                print ("HDMmtop:\t",HDMmtop)
                print ("ROLLmtop:\t",ROLLmtop)
                print ("PTCHmtop:\t",PTCHmtop)
                time.sleep(0.1)


    except KeyboardInterrupt:
        # Avoid garble on ^C
        print ("")

    except IndexError:
        print ("Trouble:",cmd)
