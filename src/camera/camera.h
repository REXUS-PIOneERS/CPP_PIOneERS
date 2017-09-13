#ifndef CAMERA_H
#define CAMERA_H

#include <unistd.h>

class PiCamera {
	pid_t camera_pid = 0;

public:

	PiCamera() {
	}

	void startVideo();
	void stopVideo();
};


#endif /* CAMERA_H */