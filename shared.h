#ifndef __SHARED_H
#define __SHARED_H

#include <stdbool.h>
#include "config.h"

enum Shared_Mem_Tokens {OSS_SHM, OSS_SEM, OSS_MSG, PROC_MSG};
enum Semaphore_Ids {BEGIN_SEMIDS, SYSCLK_SEM, FINAL_SEMIDS_SIZE};

union semun {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
};

struct time_clock {
    unsigned long nanoseconds;
    unsigned long seconds; 
    int semaphore_id;
};

struct message {
    long int msg_type;
    char msg_text[MESSAGE_BUFFER_LENGTH];
};

struct res_descr {
    int resource;
    bool is_shared;
};

struct process_ctrl_block {
    unsigned int sim_pid;
    pid_t actual_pid;
    int max_res[MAXIMUM_RES_INSTANCES];
    int allow_res[MAXIMUM_RES_INSTANCES];
};

struct oss_shm {
    struct time_clock sys_clock;
    struct process_ctrl_block process_table[MAXIMUM_PROCESSES];
    struct res_descr descriptors[MAXIMUM_RES_INSTANCES];
};

void destroy_oss_action();
void initialize_oss_action(bool create);
void increase_time_action(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds);
void subtract_time_action(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds);
void received_message_response(struct message* msg, int msg_queue, bool wait);
void send_message_delivery(struct message* msg, int msg_queue, bool wait);


#endif
