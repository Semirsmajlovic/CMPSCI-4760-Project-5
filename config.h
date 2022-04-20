// Call our config header file
#ifndef __CONFIG_H
#define __CONFIG_H

// Include our STGBOOL
#include <stdbool.h>

// Define our constants
#define MAXIMUM_LOGFILE_OUTPUT 100000
#define DEFAULT_FILE "logfile.log"
#define VERBOSE_MODE true
#define MAXIMUM_PROCESSES 18
#define MAIN_SHM_FILE "oss_shm.shm"
#define MESSAGE_BUFFER_LENGTH 2048
#define MAXIMUM_RES_INSTANCES 20
#define MAXIMUM_RUNNING_TIME 300
#define MAXIMUM_RUNNING_PROCS 40

// Define our variables
#define maximum_time_between_new_procs_in_seconds 0
#define minimum_time_between_new_procs_in_seconds 0
#define minimum_time_between_new_procs_in_ms 1000000
#define maximum_time_between_new_procs_in_ms 500000000

#endif