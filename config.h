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
#define maxTimeBetweenNewProcsSecs 0
#define minTimeBetweenNewProcsSecs 0
#define minTimeBetweenNewProcsNS 1000000 // 1 ms
#define maxTimeBetweenNewProcsNS 500000000 // 500 ms

#endif