#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <getopt.h>

#define MIN_NANO 500000
#define MAX_NANO 5000000
// Determines if the binary generator is used for unit testing or genuine test files
#define TEST 1

typedef struct {
    uint32_t id;
    uint64_t time_stamp;
} Node;
char *bin_path;
FILE *bin_file;

pthread_mutex_t v_lock;
pthread_cond_t cond;
// Determines the number of threads
int thread_count;
// Determines the max number of entries generated
int num_nodes;
// Determines the node # of the node being written
int id = 1;
// Determines which thread is chosen to be active
int current_thread = 0;
// The computer's 0 time
uint64_t base_ts;
// The holer for the previous node's time_stamp for unit testing
uint64_t recent_ts;

int dump_bin(char *output_bin){
    FILE *file = fopen(output_bin, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    uint32_t id;
    uint64_t time_stamp;
    int i=0;

    // Read and print 32-bit and 64-bit values
    while (fread(&id, sizeof(uint32_t), 1, file) == 1) {
        printf("Read 32-bit value: 0x%x\n", (unsigned int)id);
        if (fread(&time_stamp, sizeof(uint64_t), 1, file) == 1) {
            printf("    Read 64-bit value: %lu\n\n", time_stamp);
        } else {
            perror("Error reading 64-bit value");
            break;
        }
        i++;
    }

    fclose(file);
    return 0;
}

uint64_t gen_time_stamp() {
    uint64_t rv;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    rv = (uint64_t)ts.tv_sec * 1000000000L + (uint64_t)ts.tv_nsec;

    return rv;
}

void *create_values(void* arg){
    uint32_t temp;
    long tid = (long)arg;

    while(1){
        pthread_mutex_lock(&v_lock);

        while (current_thread != tid) { 
            pthread_cond_wait(&cond, &v_lock);
        }

        if (id > num_nodes) {
            pthread_mutex_unlock(&v_lock);
            current_thread = (current_thread + 1) % thread_count;
            pthread_cond_broadcast(&cond);
            break;
        }

        if(id%4 == 0) temp = 0x9a222200;
        if(id%4 == 1) temp = 0xfe222200;
        if(id%4 == 2) temp = 0x9a2222FF;
        if(id%4 == 3) temp = 0xfe2222FF;

        uint64_t time_stamp;

        if(TEST) {
            srand(time(0)); //ensures different results each run with current time = seed
            recent_ts = recent_ts + (MIN_NANO + rand() % (MAX_NANO - MIN_NANO + 1));

            time_stamp = recent_ts - base_ts;
        } else {
            //wait 1 mili = 1,000,000 nano and 0.5mili -> 5 mili so 500,000nano -> 5,000,000nano range: 4,500,000
            struct timespec req, rem;
            req.tv_sec = 0;
            srand(time(0)); //ensures different results each run with current time = seed
            req.tv_nsec = MIN_NANO + rand() % (MAX_NANO - MIN_NANO + 1);
            nanosleep(&req, &rem);

            time_stamp = gen_time_stamp() - base_ts;
        }

        size_t bytes_written = fwrite(&temp, sizeof(uint32_t), 1, bin_file);
        if (bytes_written != 1) {
            perror("Error writing uint32_t to bin file");
            fclose(bin_file);
        }

        bytes_written = fwrite(&time_stamp, sizeof(uint64_t), 1, bin_file);
        if (bytes_written != 1) {
            perror("Error writing uint64_t to bin file");
            fclose(bin_file);
        }

        // if(debug) printf("T%ld: i-%d, time-%lu, id-%u\n\n", tid, id, time_stamp, temp);
        if(id % 1000000 == 0) {
            printf("%d\n", id);;
        }
        id++;

        current_thread = (current_thread + 1) % thread_count;
        pthread_cond_broadcast(&cond);

        pthread_mutex_unlock(&v_lock);
        sched_yield();
    }
}

int main(int argc, char *argv[]){
    int opt;
    int option_index = 0;
    int debug = 0;

    // Define long options
    static struct option long_options[] = {
        {"debug", no_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "o:n:t:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'o':
                printf("%s\n", optarg);
                bin_path = optarg;
                break;
            case 'n':
                num_nodes = atoi(optarg);
                break;
            case 't':
                thread_count = atoi(optarg);
                break;
            case 0:
                if(strcmp(long_options[option_index].name, "debug") == 0){
                    debug = 1;
                }
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    int rv;
    struct timespec start, end;
    double cpu_time_used;
    pthread_t threads[thread_count];

    bin_file = fopen(bin_path, "wb");
    if (bin_file == NULL) {
        perror("Error opening bin file");
        return 1;
    }

    pthread_mutex_init(&v_lock, NULL);
    pthread_cond_init(&cond, NULL);

    base_ts = gen_time_stamp();
    recent_ts = base_ts;

    for(long i=0; i<thread_count; i++) {
        int rc = pthread_create(&threads[i], NULL, create_values, (void *)i);

        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            return 1;
        }
    }

    for(int i=0; i<thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(bin_file);

    pthread_mutex_destroy(&v_lock);
    pthread_cond_destroy(&cond);

    if(debug) rv = dump_bin(bin_path);

    printf("Binary file created successfully!\n");
    return rv;
}