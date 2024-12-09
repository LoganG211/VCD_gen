import struct
from datetime import datetime
import threading
import os
import sys

DEBUG = 1
THREAD_COUNT = 2

h_entries = []
entries = []
current_time = 0
global position
entry_size = 12

# Define the format string
format_string = '<I Q' # Little-Endian - bin_gen.py might write with Big-Endian
size_of_data = struct.calcsize(format_string)

threads = []
mutex = threading.Lock()

class h_entry:
    def __init__(self, hex, name):
        self.hex = hex
        self.name = name
    
    def __str__(self):
        return f"Node({self.hex}, {self.name})"
    
    def get_name(self):
        return self.name
    
    def get_hex(self):
        return hex(self.hex)

class Entry:
    def __init__(self, id, time_stamp):
        self.id = id
        self.time_stamp = time_stamp
    
    def __str__(self):
        return f"Entry({self.id:#010x}, {self.time_stamp})"
    
    def get_id(self):
        return self.id

    def get_time_stamp(self):
        return self.time_stamp

def dump_entries():
    print()
    for element in entries:
        print(element)

def read_names():
    with open(names_input, 'r') as n:
        for line in n:
            hex, name = line.strip().split(',')
            hex = int(hex.strip(), 16)
            name = name.strip()
            h_entries.append(h_entry(hex, name))
    if DEBUG:
        print()
        for element in h_entries:
            print(element)
        print()

def vcd_header():
    file = open(vcd_output, 'w')

    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    file.write(f'$date\n     {now}\n$end\n')
    file.write('$version\n  LG211 VCDv1 File\n$end\n')
    file.write('$timescale\n    1ns\n$end\n')

    read_names()
    for element in h_entries:
        file.write(f'$var wire 1 {element.get_hex()} {element.get_name()} $end\n')
        if DEBUG: print(element), print()
    
    file.write('$enddefinitions $end\n')
    file.write('#0\n$dumpvars\n')

    for element in h_entries:
        file.write(f'b0 {element.get_hex()}\n')
    
    global position 
    position = file.tell()
    file.close()

def vcd_timestamps():
    global position
    global current_time

    file = open(vcd_output, 'a')
    file.seek(position)
    e_bit = 0

    # Read the binary data from the file
    with open(bin_input, 'rb') as in_file:
        tid = 0
        while(threads[tid].ident != threading.get_ident()):
            tid = tid + 1
        offset = (int)(tid * ((os.path.getsize(bin_input) / entry_size) / THREAD_COUNT) * entry_size)
        in_file.seek(offset)
        while chunk := in_file.read(size_of_data):
            if((tid+1) < THREAD_COUNT and (in_file.tell()-size_of_data) == (tid+1) * ((os.path.getsize(bin_input)/entry_size)/THREAD_COUNT)*entry_size):
                break
            mutex.acquire()
            try:
                value32, value64 = struct.unpack(format_string, chunk)

                if DEBUG: print(f"T: {tid}, 32-bit value: {value32:#010x}, 64-bit value: {value64}")
                entries.append(Entry(value32, value64))

                # if(value64 > current_time):
                #     current_time = value64
                #     file.write(f'#{current_time}\n')
                
                # e_bit = value32 & 0xFF
                # if((value32 & 0xFF) == 0xFF):
                #     e_bit = 1
                # elif((value32 & 0xFF) == 0):
                #     e_bit = 0
                # else:
                #     e_bit = 2
                
                # file.write(f'b{e_bit} {hex(value32>>24)}\n')
                # position = file.tell()
            finally:
                mutex.release()
    file.close()

def main():
    if len(sys.argv) < 4:
        print("Try: python3 vcd_gen.py <input.bin> <output.vcd> <names.txt>")
        sys.exit(1)
    
    os.system("make run_c")
    
    global bin_input
    bin_input = sys.argv[1]
    global vcd_output
    vcd_output = sys.argv[2]
    global names_input
    names_input = sys.argv[3]

    # print(f"Argument 1: {bin_input}")
    # print(f"Argument 2: {vcd_output}")

    vcd_header()

    for _ in range(THREAD_COUNT):
        threads.append(threading.Thread(target=vcd_timestamps))

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()
    
    entries.sort(key=lambda Entry: Entry.time_stamp)

    if(DEBUG): dump_entries()

    print("\nVCD file created successfully!")

if __name__ == "__main__":
    main()
