#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

#include "shared.h"

struct oss_shm* shared_mem = NULL;
static int semaphore_id = -1;
static int oss_msg_queue;
static int proc_msg_queue;

int get_shm(int token) {
	key_t key;
	key = ftok(MAIN_SHM_FILE, token);
	if (key == -1) return -1;
	return shmget(key, sizeof(struct oss_shm), 0644 | IPC_CREAT);
}

int getsemaphores(int token, int sem_num) {
	key_t key;
	key = ftok(MAIN_SHM_FILE, token);
	if (key == -1) return -1;
	if ((semaphore_id = semget(key, sem_num, 0644 | IPC_CREAT)) == -1) return -1;
	return 0;
}

void lock(int num) {
	num -= 1;
	struct sembuf myop[1];
	myop->sem_num = (short)num;
	myop->sem_op = (short)-1;
	myop->sem_flg = (short)0;
	if ((semop(semaphore_id, myop, 1)) == -1) perror("Error: We were not able to lock.");
}

void unlock(int num) {
	num -= 1;
	struct sembuf myop[1];
	myop->sem_num = (short)num;
	myop->sem_op = (short)1;
	myop->sem_flg = (short)0;
	if ((semop(semaphore_id, myop, 1)) == -1) perror("Error: We were not able to unlock.");
} 

void initialize_oss_action(bool create) {
	getsemaphores(OSS_SEM, FINAL_SEMIDS_SIZE); 
    int mem_id = get_shm(OSS_SHM);
    if (mem_id < 0) {
        printf("Error: The shared memory file could not be located.");
    }
	shared_mem = shmat(mem_id, NULL, 0);
    if (shared_mem < 0) {
        printf("Error: The process to attach shared memory could not be executed.");
    }
	key_t oss_msg_key = ftok(MAIN_SHM_FILE, OSS_MSG);
	key_t proc_msg_key = ftok(MAIN_SHM_FILE, PROC_MSG);
	if (oss_msg_key < 0 || proc_msg_key < 0) {
        perror("Error: The process to grab the message queue file could not be executed.");
	}
	if (create) {
		oss_msg_queue = msgget(oss_msg_key, 0644 | IPC_CREAT);
		proc_msg_queue = msgget(proc_msg_key, 0644 | IPC_CREAT);
	} else {
		oss_msg_queue = msgget(oss_msg_key, 0644 | IPC_EXCL);
		proc_msg_queue = msgget(proc_msg_key, 0644 | IPC_EXCL);
	}
	if (oss_msg_queue < 0 || proc_msg_queue < 0) {
        printf("Error: The message queues could not be properly attached.");
	}
	if (!create) return;
	shared_mem->sys_clock.semaphore_id = SYSCLK_SEM;
	shared_mem->sys_clock.seconds = 0;
	shared_mem->sys_clock.nanoseconds = 0;
	for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
		shared_mem->descriptors[i].resource = (rand() % 10) + 1;
		shared_mem->descriptors[i].is_shared = (rand() % 20) > 20 ? false : true; 
	}
	union semun arg;
	arg.val = 1;
	for (int i = 0; i < FINAL_SEMIDS_SIZE; i++) {
		if ((semctl(semaphore_id, i, SETVAL, arg)) == -1) {
			perror("Error: The process has failed to initiliaze a semaphore, please try again.");
		}
	}
}

void destroy_oss_action() {
	if ((semctl(semaphore_id, 0, IPC_RMID)) == -1) {
		perror("Error: The process has failed to remove the semaphore, please try again.");
	}
	int mem_id = get_shm(OSS_SHM);
	if (mem_id < 0) {
        perror("Error: The shared memory file could not be located.");
    }
	if (shmctl(mem_id, IPC_RMID, NULL) < 0) {
        perror("Error: The shared memory could not be removed.");
	}
	shared_mem = NULL;
	if (msgctl(oss_msg_queue, IPC_RMID, NULL) < 0) {
		perror("Error: The shared memory could not be detached from the queue.");
	}
	if (msgctl(proc_msg_queue, IPC_RMID, NULL) < 0) {
		perror("Error: The message could not be detached from the queue.");
	}
}

void increase_time_action(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds) {
	if (Time->semaphore_id > 0) lock(Time->semaphore_id);
	Time->seconds += seconds;
	Time->nanoseconds += nanoseconds;
	if (Time->nanoseconds > 1000000000) {
		Time->seconds += 1;
		Time->nanoseconds -= 1000000000;
	}
	if (Time->semaphore_id > 0) unlock(Time->semaphore_id);
}

bool subtraction_time_choice(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds) {
	if (nanoseconds > Time->nanoseconds) {
		if (Time->seconds < 1) return false;
		Time->nanoseconds += 1000000000;
		Time->seconds -= 1;
	}
	Time->nanoseconds -= nanoseconds;
	if (seconds > Time->seconds) {
		increase_time_action(Time, 0, nanoseconds);
		return false;
	}
	Time->seconds -= seconds;
	return true;
}

void subtract_time_action(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds) {
	if (Time->semaphore_id > 0) lock(Time->semaphore_id);
	if (!subtraction_time_choice(Time, seconds, nanoseconds)) {
		perror("Error: The time could not be properly subtracted.");
	}
	if (Time->semaphore_id > 0) unlock(Time->semaphore_id);
}

void received_message_response(struct message* msg, int msg_queue, bool wait) {
	int msg_queue_id;
	if (msg_queue == OSS_MSG) {
		msg_queue_id = oss_msg_queue;
	} else if (msg_queue == PROC_MSG) {
		msg_queue_id = proc_msg_queue;
	} else {
		printf("Error: The following message %d\n of the queue was unexpected.", msg_queue);
	}
	if (msgrcv(msg_queue_id, msg, sizeof(struct message), msg->msg_type, wait ? 0 : IPC_NOWAIT) < 0) {
		perror("Error: The message could not be received.");
		fprintf(stderr, "Message: %s - Type: %ld - Queue Order: %d - Is wait required: %d\n", msg->msg_text, msg->msg_type, msg_queue_id, wait);
	}
}

void send_message_delivery(struct message* msg, int msg_queue, bool wait) {
	int msg_queue_id;
	if (msg_queue == OSS_MSG) {
		msg_queue_id = oss_msg_queue;
	} else if (msg_queue == PROC_MSG) {
		msg_queue_id = proc_msg_queue;
	} else {
		printf("Error: The following message %d\n of the queue was unexpected.", msg_queue);
	}
	if (msgsnd(msg_queue_id, msg, sizeof(struct message), wait ? 0 : IPC_NOWAIT) < 0) {
		perror("Error: The message could not be sent.");
		fprintf(stderr, "Message: %s - Type: %ld - Queue Order: %d - Is wait required: %d\n", msg->msg_text, msg->msg_type, msg_queue_id, wait);
	}
}