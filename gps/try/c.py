import socket


def Main():
    host = '127.0.0.1'
    port = 7000

    mySocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #mySocket.connect((host, port))


    message = "wobble "

    while message != 'q':
        mySocket.sendto(message.encode(),(host,port))
        #data = mySocket.recv(1024).decode()

        #print ('Received from server: ' + data)

        message = input(" -> ")

    #mySocket.close()


if __name__ == '__main__':
    Main()