serveraddr=('localhost',8888)

from socket import *
import sys,time
import threading


total_send = 0
mutex = threading.Lock()

def client(i, msg_total, msg_interval):
    global total_send,mutex
    try:
        s = socket()
        s.connect(serveraddr)
        seq = 0
    except Exception as e:
        print("client %d connect error" % i,e)
    else:
        print("client %d: connect success" % i)
        for j in range(msg_total):
            time.sleep(msg_interval)
            msg = "Hello! from client=%d seq=%d"%(i,j)
            try:
                s.send(msg.encode())
                mutex.acquire()
                total_send+=1
                mutex.release()
            except BrokenPipeError as e:
                print("client %d send error "% i, e)
                break

def test(client_num,client_interval,msg_total,msg_interval):
    for i in range(int(client_num)):
        threading.Thread(target=client,args=(i,msg_total,msg_interval)).start()
        time.sleep(client_interval)

    while(True):
        print("\r total_send: %d" % total_send)
        time.sleep(1)



if __name__ == '__main__':
    client_num =3
    client_interval = 2
    msg_total = 20
    msg_interval = 10
    test(client_num,client_interval,msg_total,msg_interval)
