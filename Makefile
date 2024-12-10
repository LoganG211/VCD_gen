PYTHON = python3
CC = gcc
LDFLAGS = -pthread

PY_DIR = py_methods
C_DIR = c_methods
R_DIR = resources
BUILD_DIR = $(C_DIR)/build

SRCS = $(C_DIR)/bin_gen.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

all: $(OBJS)
	$(CC) -o $(BUILD_DIR)/bin_gen
	$(CC) -o $(BUILD_DIR)/vcd_gen

run: c
	$(PYTHON) $(PY_DIR)/vcd_gen.py --bin_file $(C_DIR)/output.bin --vcd_file $(PY_DIR)/output.vcd --name_file $(R_DIR)/names.txt --threads 5 --debug

run_c:
	make c
	./$(BUILD_DIR)/bin_gen $(C_DIR)/output.bin 10 5
#	./$(BUILD_DIR)/vcd_gen

c: $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(C_DIR)/bin_gen.c -o $(BUILD_DIR)/bin_gen
	./$(BUILD_DIR)/bin_gen -o $(C_DIR)/output.bin -n 10 -t 5 --debug
#	$(CC) $(LDFLAGS) $(C_DIR)/vcd_gen.c -o $(BUILD_DIR)/vcd_gen

run_python:
#	$(PYTHON) $(PY_DIR)/bin_gen.py
	$(PYTHON) $(PY_DIR)/vcd_gen.py --binary_file $(C_DIR)/output.bin --vcd_file $(PY_DIR)/output.vcd --name_file $(R_DIR)/names.txt --threads 5 --debug

# Compile source files into object files in the build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	rm -f *.pyc 
	rm -rf __pycache__
	if [ -d $(BUILD_DIR) ]; then rm -r $(BUILD_DIR); fi
	find . -type f -name '*.bin' -exec rm -f {} +
	find . -path ./$(R_DIR) -prune -o -type f -name '*.vcd' -exec rm -f {} +

.PHONY: clean