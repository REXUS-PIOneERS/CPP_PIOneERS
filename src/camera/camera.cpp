/**
 * REXUS PIOneERS - Pi_1
 * camera.cpp
 * Purpose: Implementation of functions in the PiCamera class
 *
 * @author David Amison
 * @version 2.0 10/10/2017
 */

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "camera.h"
#include <cstring>
#include <sys/wait.h>
#include <string>
#include "timing/timer.h"
#include <error.h>

void PiCamera::startVideo(std::string filename) {
	if ((camera_pid = fork()) == 0) {
		//Create the command structure
		log("INFO") << "Starting camera recording video";
		// raspivid -n -t 10 -s -o rexus_video%04d.h264 -sg 5000 -g 25 -w 1920 -h 1080 -fps 25
		char unique_file[50];
		sprintf(unique_file, "%s_%s%%04d.h264", filename.c_str(), Timer::str_datetime().c_str());
		char* cmd[] = {
			"raspivid",
			"-n",
			"-t", "10",
			"-s",
			"-o", unique_file,
			"-sg", "5000",
			"-g", "30",
			"-w", "1920",
			"-h", "1080",
			"-fps", "30",
			NULL
		};
		execv("/usr/bin/raspivid", cmd);
		log("ERROR") << "Failed to start camera\n\t\"" << std::strerror(errno)
				<< "\"";
		exit(-1);
	}
}

void PiCamera::stopVideo() {
	if (camera_pid) {
		log("INFO") << "Stopping camera process (ID:" << camera_pid << ")";
		//TODO May need to send signal multiple times?
		bool died = false;
		for (int i = 0; !died && i < 5; i++) {
			int status;
			kill(camera_pid, SIGUSR1);
			sleep(1);
			if (waitpid(camera_pid, &status, WNOHANG) == camera_pid) died = true;
		}
		if (died) {
			log("INFO") << "Camera process terminated by sending USR1 signal";
		} else {
			log("ERROR") << "USR1 signal failed, sending SIGKILL";
			kill(camera_pid, SIGKILL);
		}
		camera_pid = 0;
	}
}

bool PiCamera::is_running() {
	log("INFO") << "Camera process being polled...";
	if (camera_pid) {
		if (kill(camera_pid, 0) == 0) {
			log("INFO") << "Camera process running";
			return true;
		} else {
			log("INFO") << "Camera process dead";
			camera_pid = 0;
			return false;
		}
	}
	log("INFO") << "Process has not been started";
	return false;
}
