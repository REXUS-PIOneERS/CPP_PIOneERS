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
#include <string>

class PiCamera {
	pid_t camera_pid = 0;

public:

	/**
	 * Default constructor for the PiCamera class
	 */
	PiCamera() {
	}

	/**
	 * Sets the camera recording video split into 5 second segments saved as
	 * rexus_video0001.h264, rexus_video0002.h264 etc.
	 * Video is stopped by sending the USR1 signal to the child process.
	 */
	void startVideo(std::string filename);

	/**
	 * Stop the video recording by sending a USR1 signal
	 */
	void stopVideo();
};


#endif /* CAMERA_H */