

# Environment
TARGET = ./bin/runner

CC = g++
OBJS = ./build/main.o ./build/RPi_IMU.o ./build/camera.o ./build/UART.o
LFLAGS = -Wall
CFLAGS = -Wall -c -std=c++11
INCLUDES = -lwiringPi -I/usr/pi/Pi_1/src

MAINSRC = ./src/main.cpp
IMUSRC = ./src/RPi_IMU/RPi_IMU.cpp
UARTSRC = ./src/UART/UART.cpp
CAMSRC = ./src/camera/camera.cpp

TESTOUT = ./bin/test
TESTSRC = test.cpp IMU_test.cpp
TESTINC = -I/usr/pi/Pi_1/tests

# build
$(TARGET): $(OBJS)
	@echo "Building Project..."
	$(CC) $(LFLAGS) $^ -o $@ $(INCLUDES)

./build/main.o: $(MAINSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/RPi_IMU.o: $(IMUSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/camera.o: $(CAMSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/UART.o: $(UARTSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

# build test executable
$(TESTOUT): $(TESTSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(TESTINC)

# clean
clean:
	@echo "Cleaning..."
	\rm -rf ./build/*.o ./Docs ./bin/runner
