/**
 * REXUS PIOneERS - Pi_1
 * camera.h
 * Purpose: Function declarations for implementation of camera class for video
 *
 * @author David Amison
 * @version 1.1 10/10/2017
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <unistd.h>

class PiCamera {
	pid_t camera_pid = 0;

public:

	/**
	 * Default constructor for the PiCamera class
	 */
	PiCamera() {
	}

	/**
	 * Record video in 5 second chunks saved to Docs/Videos as a child
	 * process.
	 */
	void startVideo();

	/**
	 * Stop the video recording by sending a USR1 signal
	 */
	void stopVideo();
};


#endif /* CAMERA_H */