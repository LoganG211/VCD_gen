PYTHON = python3
CC = gcc
LDFLAGS = -pthread

PY_DIR = py_methods
C_DIR = c_methods
BUILD_DIR = $(C_DIR)/build

SRCS = $(C_DIR)/bin_gen.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

all: $(OBJS)
	$(CC) -o $(BUILD_DIR)/bin_gen
	$(CC) -o $(BUILD_DIR)/vcd_gen

run: run_python run_c

run_c:
	make c
	./$(BUILD_DIR)/bin_gen
#	./$(BUILD_DIR)/vcd_gen

c: $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(C_DIR)/bin_gen.c -o $(BUILD_DIR)/bin_gen
#	$(CC) $(LDFLAGS) $(C_DIR)/vcd_gen.c -o $(BUILD_DIR)/vcd_gen

run_python:
	$(PYTHON) $(PY_DIR)/bin_gen.py
	$(PYTHON) $(PY_DIR)/vcd_gen.py

# Compile source files into object files in the build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	rm -f *.pyc 
	rm -rf __pycache__
	rm -r $(BUILD_DIR)