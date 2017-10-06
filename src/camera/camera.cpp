#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "camera.h"
#include <sys/wait.h>

void PiCamera::startVideo() {
	/*
	 * Sets the camera recording video split into 5 second segments saved as
	 * rexus_video0001.h264, rexus_video0002.h264 etc.
	 * Video is stopped by sending the USR1 signal to the child process.
	 */
	if ((camera_pid = fork()) == 0) {
		//Create the command structure
		fprintf(stderr, "Camera Starting...\n");
		char* cmd[] = {
			"raspivid",
			"-n",
			"-t", "10",
			"-s",
			"-o", "Docs/Video/rexus_video%04d.h264",
			"-sg", "5000",
			"-g", "30",
			"-w", "1920",
			"-h", "1080",
			"-fps", "30",
			NULL
		};
		execv("/usr/bin/raspivid", cmd);
		perror("PiCamera: ");
	}
}

// raspivid -n -t 10 -s -o rexus_video%04d.h264 -sg 5000 -g 25 -w 1920 -h 1080 -fps 25

void PiCamera::stopVideo() {
	if (camera_pid) {
		//TODO May need to send signal multiple times?
		bool died = false;
		fprintf(stdout, "Stopping Camera... ID:%d\n", camera_pid);
		for (int i = 0; !died && i < 5; i++) {
			int status;
			kill(camera_pid, SIGUSR1);
			sleep(1);
			if (waitpid(camera_pid, &status, WNOHANG) == camera_pid) died = true;
		}
		if (died) {
			fprintf(stdout, "Camera stopped via USR1\n");
		} else {
			fprintf(stdout, "USR1 signal failed, sending SIGKILL\n");
			kill(camera_pid, SIGKILL);
		}
	}
}
