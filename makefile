

# Environment
TARGET = ./bin/runner

CC = g++
OBJS = ./build/main.o ./build/RPi_IMU.o ./build/camera.o ./build/UART.o ./build/Ethernet.o
LFLAGS = -Wall
CFLAGS = -Wall -c -std=c++11
INCLUDES = -lwiringPi -I/home/pi/Pi_1/src

MAINSRC = ./src/main.cpp
IMUSRC = ./src/RPi_IMU/RPi_IMU.cpp
UARTSRC = ./src/UART/UART.cpp
CAMSRC = ./src/camera/camera.cpp
ETHSRC = ./src/Ethernet/Ethernet.cpp

TESTOUT = ./bin/test
TESTOBJS = ./build/test.o ./build/IMU_Tests.o ./build/RPi_IMU.o
TESTSRC = ./tests/test.cpp
IMUTESTSRC = ./tests/IMU_Tests.cpp
TESTINC = -I/home/pi/Pi_1/tests -I/home/pi/Pi_1/src

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

./build/Ethernet.o: $(ETHSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

# build test executable
$(TESTOUT): $(TESTOBJS)
	$(CC) $(LFLAGS) $^ -o $@ $(TESTINC)

./build/test.o: $(TESTSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(TESTINC)

./build/IMU_Tests.o: $(IMUTESTSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(TESTINC)

# clean
clean:
	@echo "Cleaning..."
	\rm -rf ./*.txt ./build/*.o ./Docs ./bin/runner ./bin/test
