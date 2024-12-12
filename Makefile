PYTHON = python3
CC = gcc
LDFLAGS = -pthread

PY_DIR = py_methods
C_DIR = c_methods
R_DIR = resources
BUILD_DIR = $(C_DIR)/build

SRCS = $(C_DIR)/bin_gen.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(C_DIR)/bin_gen.c -o $(BUILD_DIR)/bin_gen

run: c
	./$(BUILD_DIR)/bin_gen -o $(C_DIR)/output.bin -n 90000000 -t 10
	$(PYTHON) $(PY_DIR)/vcd_gen.py --bin_file $(C_DIR)/output.bin --vcd_file $(PY_DIR)/output.vcd --name_file $(R_DIR)/names.txt --threads 10

# py:
# 	$(PYTHON) $(PY_DIR)/vcd_gen.py --bin_file $(C_DIR)/output.bin --vcd_file $(PY_DIR)/output.vcd --name_file $(R_DIR)/names.txt --threads 10

c: $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(C_DIR)/bin_gen.c -o $(BUILD_DIR)/bin_gen
# 	./$(BUILD_DIR)/bin_gen -o $(C_DIR)/output.bin -n 90000000 -t 10
#	./$(BUILD_DIR)/vcd_gen
#	./$(BUILD_DIR)/bin_gen -o $(C_DIR)/output.bin -n 100000 -t 5 --debug
#	$(CC) $(LDFLAGS) $(C_DIR)/vcd_gen.c -o $(BUILD_DIR)/vcd_gen

p:
	$(PYTHON) $(PY_DIR)/vcd_gen.py --bin_file $(C_DIR)/output.bin --vcd_file $(PY_DIR)/output.vcd --name_file $(R_DIR)/names.txt --threads 10

format:
	astyle --style=google $(C_DIR)/bin_gen.c
	astyle --style=google $(C_DIR)/vcd_gen.c
	python3 -m yapf --style=google -i $(PY_DIR)/vcd_gen.py
	python3 -m yapf --style=google -i $(PY_DIR)/bin_gen.py

# Compile source files into object files in the build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	rm -f *.pyc 
	rm -rf __pycache__
	if [ -d $(BUILD_DIR) ]; then rm -r $(BUILD_DIR); fi
	find . -type f -name '*.bin' -exec rm -f {} +
	find . -type f -name '*.orig' -exec rm -f {} +
	find . -path ./$(R_DIR) -prune -o -type f -name '*.vcd' -exec rm -f {} +

.PHONY: clean