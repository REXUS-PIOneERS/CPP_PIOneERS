

# Environment
TARGET = ./bin/runner

CC = g++
OBJS = ./build/main.o ./build/RPi_IMU.o ./build/camera.o ./build/UART.o
LFLAGS = -Wall
CFLAGS = -Wall -c -std=c++11
INCLUDES = -lwiringPi

# build
$(TARGET): $(OBJS)
	@echo "Building Project..."
	$(CC) $(LFLAGS) $(OBJS) -o $(TARGET) $(INCLUDES)

./build/main.o: main.cpp RPi_IMU.h
	$(CC) $(CFLAGS) main.cpp -o $(BUILDDIR)/main.o

./build/RPi_IMU.o: cd src/RPi_IMU RPi_IMU.cpp RPi_IMU.h LSM9DS0.h timer.h
	$(CC) $(CFLAGS) RPi_IMU.cpp $(BUILDDIR)/RPi_IMU.o

./build/camera.o: cd src/camera camera.cpp camera.h
	$(CC) $(CFLAGS) camera.cpp $(BUILDDIR)/camera.o

./build/UART.o: cd src/UART UART.cpp UART.h
	$(CC) $(CFLAGS) UART.cpp $(BUILDDIR)/UART.o

# clean
clean:
	@echo "Cleaning..."
	\rm -rf $(BUILDDIR)/*.o ./Docs ./bin/runner
