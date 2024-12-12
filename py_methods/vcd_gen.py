import struct
from datetime import datetime
import threading
import os
import argparse

h_entries = []
entries = []
global current_time
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
    if args.debug:
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
        if args.debug: print(element), print()
    
    file.write('$enddefinitions $end\n')
    file.write('#0\n$dumpvars\n')

    for element in h_entries:
        file.write(f'b0 {element.get_hex()}\n')
    
    global position 
    position = file.tell()
    file.close()


def vcd_timestamps():
    global position

    # Read the binary data from the file
    with open(bin_input, 'rb') as in_file:
        tid = 0
        while(threads[tid].ident != threading.get_ident()):
            tid = tid + 1
        offset = (int)(tid * ((os.path.getsize(bin_input) / entry_size) / thread_count) * entry_size)
        in_file.seek(offset)
        while chunk := in_file.read(size_of_data):
            if((tid+1) < thread_count and (in_file.tell()-size_of_data) == (tid+1) * ((os.path.getsize(bin_input)/entry_size)/thread_count)*entry_size):
                break
            mutex.acquire()
            try:
                value32, value64 = struct.unpack(format_string, chunk)

                if args.debug: print(f"T: {tid}, 32-bit value: {value32:#010x}, 64-bit value: {value64}")
                entries.append(Entry(value32, value64))
            finally:
                mutex.release()


def write_vcd():
    file = open(vcd_output, 'a')
    file.seek(position)

    for entry in entries:
        e_byte = 1 if(entry.id & 0xFF) == 255 else (entry.id & 0xFF)
        if(current_time < entry.time_stamp): file.write(f'#{entry.time_stamp:020}\n')
        file.write(f'b{e_byte} {hex(entry.id>>24)}\n')

    file.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true', help='enable debug mode')
    parser.add_argument('--bin_file', required=True, type=str, help='input binary file')
    parser.add_argument('--vcd_file', required=True,type=str, help='output vcd file')
    parser.add_argument('--name_file', required=True, type=str, help='input text file')
    parser.add_argument('--threads', required=True, type=int, help='number of threads')

    global args
    args = parser.parse_args()
    
    global thread_count
    thread_count = args.threads

    global vcd_output
    vcd_output = args.vcd_file

    global bin_input
    bin_input = args.bin_file

    global names_input
    names_input = args.name_file

    vcd_header() # Works

    for _ in range(thread_count):
        threads.append(threading.Thread(target=vcd_timestamps))

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()
    
    entries.sort(key=lambda Entry: Entry.time_stamp)

    write_vcd() # Works

    if(args.debug): dump_entries()

    print("\nVCD file created successfully!")


if __name__ == "__main__":
    main()
