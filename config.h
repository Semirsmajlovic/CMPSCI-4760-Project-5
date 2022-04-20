// Call our config header file
#ifndef __CONFIG_H
#define __CONFIG_H

// Include our STGBOOL
#include <stdbool.h>

// Define our constants
#define MAX_LOGFILE 100000
#define LOGFILE "logfile.log"
#define VERBOSE_MODE true
#define MAX_PROCESSES 18
#define SHM_FILE "shmOSS.shm"
#define MSG_BUFFER_LEN 2048
#define MAX_RES_INSTANCES 20
#define MAX_RUNTIME 300 // 5m
#define MAX_RUN_PROCS 40 // Max number of processes to run

// Define our variables
#define maximum_time_between_new_procs_in_seconds 0
#define minimum_time_between_new_procs_in_seconds 0
#define minimum_time_between_new_procs_in_ms 1000000
#define maximum_time_between_new_procs_in_ms 500000000

#endif