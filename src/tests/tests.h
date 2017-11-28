/**
 * REXUS PIOneERS - Pi_1
 * tests.h
 * Purpose: Functions  definietions for testing the various modules of the
 * PIOneERS code. Each function returns 0 on success or an error code on
 * failure.
 *
 * @author David Amison
 * @version 1.0 28/10/2017
 */

#ifndef TESTS_H
#define TESTS_H

namespace tests {
	int all_tests();

	/**
	 * Run a series of basic tests on the IMU.
	 * 1) Try to connect with the IMU via i2c
	 * 2) Try to write values to a register
	 * 3) Try to read values from a register
	 * 4) Test multiprocess data collection (5 seconds), check data pipes properly
	 *		and that data is saved to files
	 * @return
	 * 0: Success
	 * 0b0000XXXX For each X that is 1 the corresponding test failed i.e.
	 * 0b00001000 means test 4 failed but others succeeded.
	 */
	int IMU_test();

	/**
	 * Run a basic test on the Camera. Run for 5 seconds.
	 * @return
	 * 0: Success
	 * 1: Failure
	 */
	int camera_test();

}
#endif /* TESTS_H */

