

# Environment
TARGET = ./bin/runner

CC = g++
OBJS = ./build/main.o ./build/RPi_IMU.o ./build/camera.o ./build/UART.o
LFLAGS = -Wall
CFLAGS = -Wall -c -std=c++11
INCLUDES = -lwiringPi

MAINSRC = ./src/main.cpp
IMUSRC = ./src/RPi_IMU/RPi_IMU.cpp
UARTSRC = ./src/UART/UART.cpp
CAMSRC = ./src/camera/camera.cpp

# build
$(TARGET): $(OBJS)
	@echo "Building Project..."
	$(CC) $(LFLAGS) $^ -o $@ $(INCLUDES)

./build/main.o: $(MAINSRC)
	$(CC) $(CFLAGS) -o $@ $^

./build/RPi_IMU.o: $(IMUSRC)
	$(CC) $(CFLAGS) -o $@ $^

./build/camera.o: $(CAMSRC)
	$(CC) $(CFLAGS) -o $@ $^

./build/UART.o: $(UARTSRC)
	$(CC) $(CFLAGS) -o $@ $^

# clean
clean:
	@echo "Cleaning..."
	\rm -rf ./build/*.o ./Docs ./bin/runner
