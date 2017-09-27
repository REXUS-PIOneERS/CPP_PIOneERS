/*
 * Basic tests for the RPi_IMU class to test writing to and reading from
 * the required registers as well as the ability to create a separate process
 * that reads the data, saving it to a file and also passing it back to the
 * main process as a pipe.
 */

#include "catch.h"

#include "RPi_IMU/RPi_IMU.h"
#include <stdio.h>  // For getc()
#include <stdint.h>
#include <fcntl.h> // For open()
#include <unistd.h>  // For sleep()

SCENARIO("IMU is operational", "[IMU]") {

	GIVEN("An IMU class") {
		RPi_IMU IMU = RPi_IMU();
		//REQUIRE(IMU.i2c_file >= 0);

		WHEN("Setting up IMU Registers with default values") {
			bool acc = IMU.setupAcc();
			bool gyr = IMU.setupGyr();
			bool mag = IMU.setupMag();

			THEN("The registers are written to") {
				REQUIRE(acc);
				REQUIRE(gyr);
				REQUIRE(mag);
			}

			AND_WHEN("Data is read from each accelerometer axis") {
				uint16_t x = IMU.readAccAxis(1);
				uint16_t y = IMU.readAccAxis(2);
				uint16_t z = IMU.readAccAxis(3);

				THEN("Data from accelerometer received") {
					REQUIRE(x != 0xFFFF);
					REQUIRE(y != 0xFFFF);
					REQUIRE(z != 0xFFFF);
				}
			}

			AND_WHEN("Data is read from each gyro axis") {
				uint16_t x = IMU.readGyrAxis(1);
				uint16_t y = IMU.readGyrAxis(2);
				uint16_t z = IMU.readGyrAxis(3);

				THEN("Data from gyro received") {
					REQUIRE(x != 0xFFFF);
					REQUIRE(y != 0xFFFF);
					REQUIRE(z != 0xFFFF);
				}
			}

			AND_WHEN("Data is read from each magnetometer axis") {
				uint16_t x = IMU.readMagAxis(1);
				uint16_t y = IMU.readMagAxis(2);
				uint16_t z = IMU.readMagAxis(3);

				THEN("Data from magnetometer received") {
					REQUIRE(x != 0xFFFF);
					REQUIRE(y != 0xFFFF);
					REQUIRE(z != 0xFFFF);
				}
			}

			AND_WHEN("Reading data as a background process") {
				int pipe = IMU.startDataCollection("test_data");
				sleep(1);

				THEN("Pipe is received and is receiving data") {
					REQUIRE(pipe >= 0);
					FILE* fd = fdopen(pipe,"r");
					int c = getc(fd);
					REQUIRE(c);
					REQUIRE(!feof(fd));
				}

				AND_THEN("Data is written to a file") {
					int file = open("test_data0000.txt", O_RDONLY);
					REQUIRE(file >= 0);
					FILE* fd = fdopen(file,"r");
					int c = getc(fd);
					REQUIRE(c);
					REQUIRE(!feof(fd));
					close(file);
				}

				AND_WHEN("Background process ends") {
					int rtn = IMU.stopDataCollection();

					THEN("Process ends cleanly") {
						REQUIRE(rtn == 0);
						//REQUIRE(!IMU.i2c_file.is_open());
					}
					// Get rid of all the test files
					sleep(1);
					system("sudo rm -rf ./*.txt");
				}
			}
		}
	}
}
