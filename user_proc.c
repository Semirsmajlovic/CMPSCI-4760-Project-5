#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "shared.h"
#include "config.h"

extern struct oss_shm* shared_mem;
static struct message msg;
static struct time_clock endtime;
static char* exe_name;
static int sim_pid;

void help_instructions() {
    printf("Resource Management:\n");
    printf("This process is not intended to be ran alone, it is being utilizes by OSS.\n");
}

void initialize_child() {
    srand((int)time(NULL) + getpid());
    init_oss(false);
    endtime.nanoseconds = (rand() % 100000000) + 1000;
    endtime.seconds = (rand() % 5) + 1;
    add_time(&endtime, shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
}

int main(int argc, char** argv) {
    int option;
    exe_name = argv[0];
    while ((option = getopt(argc, argv, "hp:")) != -1) {
        switch (option) {
        case 'h':
            help_instructions();
            exit(EXIT_SUCCESS);
        case 'p':
            sim_pid = atoi(optarg);
            break;
        case '?':
            exit(EXIT_FAILURE);
        }
    }
    initialize_child();

    bool has_resources = false;
    bool can_terminate = false;

    while (true) {
        strncpy(msg.msg_text, "", MESSAGE_BUFFER_LENGTH);
        msg.msg_type = getpid();
        recieve_msg(&msg, PROC_MSG, true);
        if (shared_mem->sys_clock.seconds > endtime.seconds && shared_mem->sys_clock.nanoseconds > endtime.nanoseconds) {
            can_terminate = true;
        }
        if (can_terminate) {
            strncpy(msg.msg_text, "terminate", MESSAGE_BUFFER_LENGTH);
            msg.msg_type = getpid();
            send_msg(&msg, OSS_MSG, false);
            exit(sim_pid);
        } else if ((rand() % 10 > 5) && has_resources) {
            strncpy(msg.msg_text, "release", MESSAGE_BUFFER_LENGTH);
            msg.msg_type = getpid();
            send_msg(&msg, OSS_MSG, false);
            has_resources = false;
        } else {
            snprintf(msg.msg_text, MESSAGE_BUFFER_LENGTH, "request");
            for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
                char resource_requested[MESSAGE_BUFFER_LENGTH];
                int max = shared_mem->process_table[sim_pid].max_res[i] - shared_mem->process_table[sim_pid].allow_res[i] + 1;
                snprintf(resource_requested, MESSAGE_BUFFER_LENGTH, " %d", rand() % max);
                strncat(msg.msg_text, resource_requested, MESSAGE_BUFFER_LENGTH - strlen(msg.msg_text));
            }
            msg.msg_type = getpid();
            send_msg(&msg, OSS_MSG, false);
            recieve_msg(&msg, PROC_MSG, true);
            if (strncmp(msg.msg_text, "acquired", MESSAGE_BUFFER_LENGTH) == 0) {
                has_resources = true;
            }
        }
    }
    exit(EXIT_SUCCESS);
}