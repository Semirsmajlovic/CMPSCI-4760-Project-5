// Define our includes
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <string.h>

// Define our imports
#include "shared.h"
#include "config.h"
#include "queue.h"

// Set our child pid
static pid_t children[MAXIMUM_PROCESSES];
static size_t num_children = 0;

// Define our structs
extern struct oss_shm* shared_mem;
static struct Queue proc_queue;
static struct Queue copy_queue;
static struct message msg;

// Define our static variables
static char* exe_name;
static int log_line = 0;
static int total_procs = 0;
static struct time_clock last_run;

// Define statistics
struct Statistics {
    unsigned int granted_requests;
    unsigned int denied_requests;
    unsigned int terminations;
    unsigned int releases;
};
static struct Statistics stats;

// Include our voids
void help();
void signal_handler(int signum);
void initialize();
void try_spawn_child();

// Define integer and bool conditional
int launch_child();
bool is_safe(int sim_pid, int resources[MAXIMUM_RES_INSTANCES]);

// Include more voids
void handle_processes();
void remove_child(pid_t pid);
void matrix_to_string(char* buffer, size_t buffer_size, int* matrix, int rows, int cols);
void output_stats();
void save_to_log(char* text);

// Main function
int main(int argc, char** argv) {
    int option;
    exe_name = argv[0];

    // Loop through our arguments
    while ((option = getopt(argc, argv, "h")) != -1) {
        switch (option) {
            case 'h':
                help();
                exit(EXIT_SUCCESS);
            case '?':
                exit(EXIT_FAILURE);
        }
    }

    // Remove and erase our logfile.
    FILE* file_ptr = fopen(DEFAULT_FILE, "w");
    fclose(file_ptr);

    // Call our initialization
    initialize();

    // Keep track of our processes
    last_run.nanoseconds = 0;
    last_run.seconds = 0;

    // Our main while loop in our function
    while (true) {
        add_time(&(shared_mem->sys_clock), 1, rand() % 1000);
        try_spawn_child();
        handle_processes();
        pid_t pid = waitpid(-1, NULL, WNOHANG);
		if (pid > 0) {
            remove_child(pid);
		}
        if (total_procs > MAXIMUM_RUNNING_PROCS && queue_is_empty(&proc_queue)) {
            break;
        } 
    }
    output_stats();
    dest_oss();
    exit(EXIT_SUCCESS);
}

// Define our help method
void help() {
    printf("\nResource Management Usage:\n");
	printf("Run './oss' and the execution will begin.\n");
	printf("The output will be written to 'logfile.log'.\n\n");
}

// Define our signal handler
void signal_handler(int signum) {
	if (signum == SIGINT) {
		fprintf(
            stderr, 
            "\nError: Interruption detected using SIGINT, we are now terminating the children process..\n"
        );
	} else if (signum == SIGALRM) {
		fprintf(
            stderr, 
            "\nError: The process execution has timed out, the process finished in %d seconds.\n", 
            MAXIMUM_RUNNING_TIME
        );
	}
    for (int i = 0; i < MAXIMUM_PROCESSES; i++) {
        if (children[i] > 0) {
            kill(children[i], SIGKILL);
            children[i] = 0;
            num_children--;
        }
    }
    output_stats();
    dest_oss();

    // Trigger our alarms with exits
    if (signum == SIGINT) {
        exit(EXIT_SUCCESS);
    }
	if (signum == SIGALRM) {
        exit(EXIT_SUCCESS);
    }
}

// Define our initialization
void initialize() {
    srand((int)time(NULL) + getpid());
    init_oss(true);
    queue_init(&proc_queue);
    for (int i = 0; i < MAXIMUM_PROCESSES; i++) {
        children[i] = 0;
    }
    stats.granted_requests = 0;
    stats.denied_requests = 0;
    stats.releases = 0;
    stats.terminations = 0;
	signal(SIGINT, signal_handler);
	signal(SIGALRM, signal_handler);	
	alarm(MAXIMUM_RUNNING_TIME);
}

// Define our launch child variable
int launch_child() {
    char* program = "./user_proc";
    return execl(program, program, NULL);
}


// Define our remove_child method
void remove_child(pid_t pid) {
    for (int i = 0; i < num_children; i++) {
		if (children[i] == pid) {
			children[i] = 0;
            num_children--;
            break;
		}
	}
}

// Define our spawn child method
void try_spawn_child() {
    if (total_procs > MAXIMUM_RUNNING_PROCS) {
        return;
    }
    int seconds = (rand() % (maximum_time_between_new_procs_in_seconds + 1)) + minimum_time_between_new_procs_in_seconds;
    int nansecs = (rand() % (maximum_time_between_new_procs_in_ms + 1)) + minimum_time_between_new_procs_in_ms;
    if ((shared_mem->sys_clock.seconds - last_run.seconds > seconds) && 
    (shared_mem->sys_clock.nanoseconds - last_run.nanoseconds > nansecs)) {
        if (num_children < MAXIMUM_PROCESSES) {
            int sim_pid;
            for (sim_pid = 0; sim_pid < MAXIMUM_PROCESSES; sim_pid++) {
                if (children[sim_pid] == 0) break;
            }
            shared_mem->process_table[sim_pid].sim_pid = sim_pid;
            for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
                shared_mem->process_table[sim_pid].max_res[i] = rand() % (shared_mem->descriptors[i].resource + 1);
                shared_mem->process_table[sim_pid].allow_res[i] = 0;
            }
            pid_t pid = fork();
            if (pid == 0) {
                if (launch_child() < 0) {
                    printf("Failed to launch process.\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                children[sim_pid] = pid;
                num_children++;
                queue_insert(&proc_queue, sim_pid);
                shared_mem->process_table[sim_pid].actual_pid = pid;
                total_procs++;
            }
            add_time(&shared_mem->sys_clock, 0, rand() % 100000);
            add_time(&last_run, shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
        }
    }
}

// Define our children process handler
void handle_processes() {
    char log_buf[100];
    int sim_pid = queue_peek(&proc_queue);
    if (sim_pid < 0) return;
    strncpy(msg.msg_text, "run", MESSAGE_BUFFER_LENGTH);
    msg.msg_type = shared_mem->process_table[sim_pid].actual_pid;
    send_msg(&msg, PROC_MSG, false);
    snprintf(log_buf, 100, "OSS sent run message to P%d at %ld:%ld", sim_pid, shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
    save_to_log(log_buf);
    add_time(&shared_mem->sys_clock, 0, rand() % 10000);
    strncpy(msg.msg_text, "", MESSAGE_BUFFER_LENGTH);
    msg.msg_type = shared_mem->process_table[sim_pid].actual_pid;
    recieve_msg(&msg, OSS_MSG, true);
    add_time(&shared_mem->sys_clock, 0, rand() % 10000);
    char* cmd = strtok(msg.msg_text, " ");
    if (strncmp(cmd, "request", MESSAGE_BUFFER_LENGTH) == 0) {
        snprintf(log_buf, 100, "OSS recieved request from P%d for some resources at %ld:%ld", sim_pid, shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
        save_to_log(log_buf);
        int resources[MAXIMUM_RES_INSTANCES];
        for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
            cmd = strtok(NULL, " ");
            resources[i] = atoi(cmd);
        }
        for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
            shared_mem->process_table->allow_res[i] = resources[i];
        }
        add_time(&shared_mem->sys_clock, 0, rand() % 10000);
        if (is_safe(sim_pid, resources)) {
            snprintf(log_buf, 100, "\tSafe state, granting request");
            save_to_log(log_buf);
            strncpy(msg.msg_text, "acquired", MESSAGE_BUFFER_LENGTH);
            msg.msg_type = shared_mem->process_table[sim_pid].actual_pid;
            send_msg(&msg, PROC_MSG, false);
            stats.granted_requests++;
        } else {
            snprintf(log_buf, 100, "\tUnsafe state, denying request");
            save_to_log(log_buf);
            strncpy(msg.msg_text, "denied", MESSAGE_BUFFER_LENGTH);
            msg.msg_type = shared_mem->process_table[sim_pid].actual_pid;
            send_msg(&msg, PROC_MSG, false);
            stats.denied_requests++;
        }
    } else if (strncmp(cmd, "release", MESSAGE_BUFFER_LENGTH) == 0) {
        snprintf(log_buf, 100, "OSS releasing resources for P%d at %ld:%ld", sim_pid, shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
        save_to_log(log_buf);
        int num_res = 0;
        for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
            if (shared_mem->process_table[sim_pid].allow_res[i] > 0) {
                snprintf(log_buf, 100, "\tReleasing resource %d with %d instances", i, shared_mem->process_table[sim_pid].allow_res[i]);
                save_to_log(log_buf);
                shared_mem->process_table[sim_pid].allow_res[i] = 0;
                num_res++;
                add_time(&shared_mem->sys_clock, 0, rand() % 100);
            }
        }
        stats.releases++;
        if (num_res <= 0) {
            save_to_log("\tNo resources to release");
        }
    } else if (strncmp(cmd, "terminate", MESSAGE_BUFFER_LENGTH) == 0) {
        int num_res = 0;
        for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
            if (shared_mem->process_table[sim_pid].allow_res[i] > 0) {
                snprintf(log_buf, 100, "\tReleasing resource %d with %d instances", i, shared_mem->process_table[sim_pid].allow_res[i]);
                save_to_log(log_buf);
                shared_mem->process_table[sim_pid].max_res[i] = 0;
                shared_mem->process_table[sim_pid].allow_res[i] = 0;
                num_res++;
                add_time(&shared_mem->sys_clock, 0, rand() % 100);
            }
        }
        stats.terminations++;
        if (num_res <= 0) {
            save_to_log("\tNo resources to release");
        }
        add_time(&shared_mem->sys_clock, 0, rand() % 100000);
        sim_pid = queue_pop(&proc_queue);
        remove_child(shared_mem->process_table[sim_pid].actual_pid);
        return;
    }
    queue_pop(&proc_queue);
    queue_insert(&proc_queue, sim_pid);
    add_time(&shared_mem->sys_clock, 0, rand() % 100000);
}
// Define our is safe boolean
bool is_safe(int sim_pid, int requests[MAXIMUM_RES_INSTANCES]) {
    char log_buf[100];
    snprintf(log_buf, 100, "OSS running deadlock detection at %ld:%ld", shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
    add_time(&shared_mem->sys_clock, 0, rand() % 1000000);
    save_to_log(log_buf);
    memcpy(&copy_queue, &proc_queue, sizeof(struct Queue));
    int size = copy_queue.size;
    int curr_elm = queue_pop(&copy_queue);
    int maximum[size][MAXIMUM_RES_INSTANCES];
    int allocated[size][MAXIMUM_RES_INSTANCES];
    int need[size][MAXIMUM_RES_INSTANCES];
    int available[MAXIMUM_RES_INSTANCES];
    int num_avail = 0;
    for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
        available[i] = shared_mem->descriptors[i].resource;
        num_avail++;
    }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < MAXIMUM_RES_INSTANCES; j++) {
            maximum[i][j] = shared_mem->process_table[curr_elm].max_res[j];
            allocated[i][j] = shared_mem->process_table[curr_elm].allow_res[j];
        }
        curr_elm = queue_pop(&copy_queue);
    }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < MAXIMUM_RES_INSTANCES; j++) {
            need[i][j] = maximum[i][j] - allocated[i][j]; 
        }
    }
    if (VERBOSE_MODE && ((stats.granted_requests % 20) == 0)) {
        int buf_size = size * MAXIMUM_RES_INSTANCES * 8;
        char buf[buf_size];
        save_to_log("Need Matrix:");
        matrix_to_string(buf, buf_size, &need[0][0], size, MAXIMUM_RES_INSTANCES);
        save_to_log(buf);
        save_to_log("Maximum Matrix:");
        matrix_to_string(buf, buf_size, &maximum[0][0], size, MAXIMUM_RES_INSTANCES);
        save_to_log(buf);
        save_to_log("Allocated Matrix:");
        matrix_to_string(buf, buf_size, &allocated[0][0], size, MAXIMUM_RES_INSTANCES);
        save_to_log(buf);
        save_to_log("Available Array:");
        matrix_to_string(buf, buf_size, available, 1, MAXIMUM_RES_INSTANCES);
        save_to_log(buf);
        save_to_log("Request Array:");
        matrix_to_string(buf, buf_size, requests, 1, MAXIMUM_RES_INSTANCES);
        save_to_log(buf);
    }
    int index = 0;
    memcpy(&copy_queue, &proc_queue, sizeof(struct Queue));
    curr_elm = queue_pop(&copy_queue);
    while (!queue_is_empty(&copy_queue)) {
        if (curr_elm == sim_pid) break;
        index++;
        curr_elm = queue_pop(&copy_queue);
    }
    for (int i = 0; i < MAXIMUM_RES_INSTANCES; i++) {
        if (need[index][i] < requests[i]) {
            return false;
        }
        if (requests[i] <= available[i]) {
            available[i] -= requests[i];
            allocated[index][i] += requests[i];
            need[index][i] -= requests[i];
        } else {
            return false;
        }
    }
    return true;
}

// Define our matrix to string conversion
void matrix_to_string(char* dest, size_t buffer_size, int* matrix, int rows, int cols) {
    strncpy(dest, "", buffer_size);
    char buffer[buffer_size];
    strncat(dest, "    ", buffer_size);
    for (int i = 1; i <= MAXIMUM_RES_INSTANCES; i++) {
        snprintf(buffer, buffer_size, "R%-2d ", i);
        strncat(dest, buffer, buffer_size);
    }
    strncat(dest, "\n", buffer_size);
    for (int i = 0; i < rows; i++) {
        snprintf(buffer, buffer_size, "P%-3d", i);
        strncat(dest, buffer, buffer_size);
        for (int j = 0; j < cols; j++) {
            snprintf(buffer, buffer_size, "%-3d ", matrix[i * cols + j]);
            strncat(dest, buffer, buffer_size);
        }
        if (i != rows - 1) strncat(dest, "\n", buffer_size);
    }
}

// Define our statistics output
void output_stats() {
    printf("\n");
    printf("Our Statistics:\n\n");
    printf("Request Data: \n");
    printf("Denied Requests: ", stats.denied_requests ," | Granted Requests: ", stats.granted_requests ," | Accumulative: ", stats.granted_requests + stats.denied_requests);
    printf("--TERMINATIONS\n");
    printf("\t%-12s %d\n", "TOTAL:", stats.terminations);
    printf("--RELEASES\n");
    printf("\t%-12s %d\n", "TOTAL:", stats.releases);
    printf("--SIMULATED TIME\n");
    printf("\t%-12s %ld\n", "SECONDS:", shared_mem->sys_clock.seconds);
    printf("\t%-12s %ld\n", "NANOSECONDS:", shared_mem->sys_clock.nanoseconds);
    printf("\n");
}

// Define our method to save to logfile.
void save_to_log(char* text) {
	FILE* file_log = fopen(DEFAULT_FILE, "a+");
    log_line++;
    if (log_line > MAXIMUM_LOGFILE_OUTPUT) {
        errno = EINVAL;
        perror("Error: The logfile.log has exceeded the maximum length for output.");
    }
	if (file_log == NULL) {
		perror("Error: The logfile.log could not be opened due to an issue.");
        return;
	}
    fprintf(file_log, "%s\n", text);
    fclose(file_log);
}