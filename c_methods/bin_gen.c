#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <inttypes.h>

#define DEBUG 1
#define THREAD_COUNT 5
#define NUM_NODES 10
#define MIN_NANO 500000
#define MAX_NANO 5000000

typedef struct {
    uint32_t id;
    uint64_t time_stamp;
} Node;
FILE *bin_file;

pthread_mutex_t v_lock;
pthread_cond_t cond;
int id = 1;
int current_thread = 0;

int dump_bin(){
    FILE *file = fopen("c_methods/output.bin", "rb");
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

void *create_values(void* arg){
    uint32_t temp;
    long tid = (long)arg;

    while(1){
        pthread_mutex_lock(&v_lock);

        while (current_thread != tid) { 
            pthread_cond_wait(&cond, &v_lock);
        }

        if (id > NUM_NODES) {
            pthread_mutex_unlock(&v_lock);
            current_thread = (current_thread + 1) % THREAD_COUNT;
            pthread_cond_broadcast(&cond);
            break;
        }

        if(id%4 == 0) temp = 0x9a222200;
        if(id%4 == 1) temp = 0xfe222200;
        if(id%4 == 2) temp = 0x9a2222FF;
        if(id%4 == 3) temp = 0xfe2222FF;

        //wait 1 mili = 1,000,000 nano and 0.5mili -> 5 mili so 500,000nano -> 5,000,000nano range: 4,500,000
        struct timespec req, rem;
        req.tv_sec = 0;
        srand(time(0)); //ensures different results each run with current time = seed
        req.tv_nsec = MIN_NANO + rand() % (MAX_NANO - MIN_NANO + 1);
        nanosleep(&req, &rem);

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t time_stamp = (uint64_t)ts.tv_sec * 1000000000L + (uint64_t)ts.tv_nsec;

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

        if(DEBUG) printf("T%ld: i-%d, time-%lu, id-%u\n\n", tid, id, time_stamp, temp);
        id++;

        current_thread = (current_thread + 1) % THREAD_COUNT;
        pthread_cond_broadcast(&cond);

        pthread_mutex_unlock(&v_lock);
        sched_yield();
    }
}

int main(){
    int rv;
    pthread_t threads[THREAD_COUNT];

    bin_file = fopen("c_methods/output.bin", "wb");
    if (bin_file == NULL) {
        perror("Error opening bin file");
        return 1;
    }

    pthread_mutex_init(&v_lock, NULL);
    pthread_cond_init(&cond, NULL);

    for(long i=0; i<THREAD_COUNT; i++) {
        int rc = pthread_create(&threads[i], NULL, create_values, (void *)i);

        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            return 1;
        }
    }

    for(int i=0; i<THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    fclose(bin_file);

    pthread_mutex_destroy(&v_lock);
    pthread_cond_destroy(&cond);

    if(DEBUG) rv = dump_bin();

    printf("Binary file created successfully!\n");
    return rv;
}