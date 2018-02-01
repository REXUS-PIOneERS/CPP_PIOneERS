TARGET1 = ./bin/raspi1
TARGET2 = ./bin/raspi2

CC = g++
PI1OBJS = ./build/raspi1.o ./build/tests.o ./build/logger.o ./build/packet.o ./build/protocol.o ./build/transceiver.o ./build/pipes.o ./build/RPi_IMU.o ./build/camera.o ./build/UART.o ./build/Ethernet.o
PI2OBJS = ./build/raspi2.o ./build/tests.o ./build/logger.o ./build/packet.o ./build/protocol.o ./build/transceiver.o ./build/pipes.o ./build/RPi_IMU.o ./build/camera.o ./build/UART.o ./build/Ethernet.o
LFLAGS = -Wall
CFLAGS = -Wall -c -std=c++11
INCLUDES = -lwiringPi -I./src

RASPI1SRC = ./src/raspi1.cpp
RASPI2SRC = ./src/raspi2.cpp
IMUSRC = ./src/RPi_IMU/RPi_IMU.cpp
UARTSRC = ./src/UART/UART.cpp
CAMSRC = ./src/camera/camera.cpp
ETHSRC = ./src/Ethernet/Ethernet.cpp
PIPESRC = ./src/comms/pipes.cpp
TRANSRC = ./src/comms/transceiver.cpp
PROTOSRC = ./src/comms/protocol.cpp
PACKSRC = ./src/comms/packet.cpp
LOGSRC = ./src/logger/logger.cpp
TESTSSRC = ./src/tests/tests.cpp

TESTOUT = ./bin/test
TESTOBJS = ./build/test.o ./build/IMU_Tests.o ./build/RPi_IMU.o
TESTSRC = ./tests/test.cpp
IMUTESTSRC = ./tests/IMU_Tests.cpp
TESTINC = -I/home/pi/CPP_PIOneERS/tests -I/home/pi/CPP_PIOneERS/src

all: $(TARGET1) $(TARGET2) $(TESTOUT)
	@echo "Making Everything..."

# build
$(TARGET1): $(PI1OBJS)
	$(CC) $(LFLAGS) $^ -o $@ $(INCLUDES)

$(TARGET2): $(PI2OBJS)
	$(CC) $(LFLAGS) $^ -o $@ $(INCLUDES)

./build/tests.o: $(TESTSSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/raspi1.o: $(RASPI1SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/raspi2.o: $(RASPI2SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)


./build/logger.o : $(LOGSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/packet.o : $(PACKSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/protocol.o : $(PROTOSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/transceiver.o : $(TRANSRC)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)

./build/pipes.o: $(PIPESRC)
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

boom_test: ./src/boom_test.cpp
	$(CC) $(CFLAGS) -o &@ &^ &(TESTINC)

imp_example: ./src/imp_example.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(TESTINC)

# clean
clean:
	@echo "Cleaning..."
	\rm -rf ./*.txt ./build/*.o /Docs ./bin/raspi1 ./bin/raspi2 ./bin/test
