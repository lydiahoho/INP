import socket
import subprocess, threading, time, sys
from scapy.all import rdpcap


def receive_messages(id):
    # Define the server address and port
    server_address = ("127.0.0.1", 10495)
    # Create a UDP socket
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Send data to the server
    message = "hello" + id  # Replace with your desired message
    udp_socket.sendto(message.encode(), server_address)

    # Receive data from the server
    data, server = udp_socket.recvfrom(1024)
    challenge_id = data.decode()[3:]
    print( data.decode())
    command = ["sudo", "tcpdump", "-ni", "any", "-Xxnv", "udp", "and", "port", "10495", "-w", "capture.pcap"]
    password = "lydia0201"
    # Start tcpdump process
    tcpdump_process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    tcpdump_process.stdin.write(password + '\n')
    tcpdump_process.stdin.flush()
    print("tcpdump started")
    time.sleep(3)
    # Send "chals" message
    message = "chals_" + challenge_id  # Replace with your desired message
    udp_socket.sendto(message.encode(), server_address)
    udp_socket.settimeout(5)
    while True:
        try:
            data, server = udp_socket.recvfrom(1024)
        except socket.timeout:
            break
    print("Receive Packets Finish")
    # Terminate tcpdump process
    tcpdump_process.terminate()
    tcpdump_process.wait()
    print("tcpdump closed")
    
    # Read the pcap file
    pcap_file_path = 'capture.pcap'
    packets = rdpcap(pcap_file_path)
    dict = {}
    i, begin_flag, end_flag = 0, 0, 0
    # Iterate through each packet in the pcap file
    res=""
    for packet in packets:
        if 'UDP' in packet:
            udp_layer = packet['UDP']
            udp_payload = bytes(udp_layer.payload)
            data = udp_payload.decode('utf-8')
            if data[0:3] == "SEQ" :
                num = int(data[4:9])
                dict[num] = len(packet)-48
                if data[10:20] == "BEGIN FLAG":
                    begin_flag = num
                elif data[10:18] == "END FLAG":
                    end_flag = num
                else: 
                    continue
    print("BEGIN FLAG", begin_flag, "/ END FLAG", end_flag)
    for j in range(begin_flag+1, end_flag):
        ascii = chr(dict[j])
        res+=ascii
    
    mes = "verfy" + res  # Replace with your desired message
    print(mes)
    udp_socket.sendto(mes.encode(), server_address)
    verfy = udp_socket.recv(1024)
    print("Result:", verfy.decode())
    # Close the socket
    udp_socket.close()
    
if __name__ == '__main__':
    #id = input("Enter your id: ")
    id = "110550080"
    receive_messages(id) 
    print("end")

