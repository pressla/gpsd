import socket


def Main():
    host = "127.0.0.1"
    port = 7000

    mySocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    mySocket.bind((host, port))

    #mySocket.listen(1)
    #conn, addr = mySocket.accept()
    while True:
        data, addr = mySocket.recvfrom(1024)
        if not data:
            break
        print ("Connection from: " + str(addr))
        print ("from connected  user: " + str(data))

        data = str(data).upper()
        print ("sending: " + str(data))
        #conn.send(data.encode())

    #conn.close()


if __name__ == '__main__':
    Main()