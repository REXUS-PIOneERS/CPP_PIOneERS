# REXUS PIOneERS-C++ Code

Code for both pis in the REXUS PIOneERS project, controlling data collection and storage of various sensors as well as sharing with the RXSM.

## Getting Started

Basic instructions for how to install and setup a copy of the project

### Requriements

This project is designed to run on a raspberry pi running Raspbian and makes use of various libraries:
* git
* wiringPi
* i2c-tools

To install them you can run the following commands:
```
// To install git tools
sudo apt-get install git

// To install wiringPi
cd
git clone git://git.drogon.net/wiringPi
cd ~/wiringPi
./build

// To install i2c tools
sudo apt-get install i2c-tools
```
It is also necessary to activate the I2C, Camera and Serial (not for terminal) interfaces using the raspi-config command and to rename Pi 1 to raspi1 and Pi 2 to raspi2.
In the file /boot/config.txt it is necessary to add this line:
```
dtoverlay=pi3-disable-bt
```
This disables the Bluetooth on the Pi and changes the mini-UART (used by TX and RX pins) to standard UART.

### Installing

To install the code for Pi_1 you will need to clone the repository from GitHub into the '/home/pi' directory.
```
cd ~/home/pi
git clone https://github.com/REXUS-PIOneERS/CPP_PIOneERS.git
```
The package comes with a makefile for ease of creating the executables.
```
make clean

make all

// Alternatively make each file separately
make ./bin/raspi1
make ./bin/raspi2
```
This can also be used to compile tests to check everything is properly setup.
```
make ./bin/test

// To run tests
./bin/test
```
Tests are controlled via [Catch](https://github.com/philsquared/Catch) and when run will report any problems.
Note: If everything is not set up (i.e. camera, IMU, motor etc) tests will fail.

### Pinout Instructions

Connect the various components to the Pi 1 as shown in the pinout diagram.

![Alt text](/img/Pinout.JPG)

### Usage Instructions
