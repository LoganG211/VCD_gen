import struct
import threading
import os

DEBUG = 0
THREAD_COUNT = 2

output_dir = "py_methods"
os.makedirs(output_dir, exist_ok=True)

threads = []
mutex = threading.Lock()

values = [
    # Define the values
    # first 2 digits in 32 are id last 2 are entry(00)/exit(FF)
    # (0xfe222200, 0x0000000000000001),
    # (0x9a222200, 0x0000000000000002),
    # (0x13222200, 0x0000000000000003)
]
index = 1


# Write the values to a list
def create_values():
    global values
    global index
    while index < 101:
        if index % 2 == 0:
            temp = 0x9A222200
        if index % 2 > 0:
            temp = 0xFE222200
        mutex.acquire()
        try:
            values.append((temp, int(index)))
            index += 1
        finally:
            mutex.release()

    # Pack and write the values
    bin_output = os.path.join(output_dir, "output.bin")
    with open(bin_output, "wb") as f:
        for value32, value64 in values:
            mutex.acquire()
            try:
                f.write(struct.pack("<I Q", value32, value64))
            finally:
                mutex.release()


for _ in range(THREAD_COUNT):
    threads.append(threading.Thread(target=create_values))

for thread in threads:
    thread.start()

for thread in threads:
    thread.join()

print("Binary file created successfully!")
