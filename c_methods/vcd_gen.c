#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <pthread.h>
#include <limits.h>

#define DEBUG 1
#define TEST 1
#define THREAD_COUNT 2
#define NUM_ENTRIES 2
#define MAX_LINE_LENGTH 256

typedef struct {
    char hex[MAX_LINE_LENGTH];
    char name[MAX_LINE_LENGTH];
} header_entry;
header_entry header_entries[NUM_ENTRIES];

typedef struct {
    uint32_t id;
    uint64_t time_stamp;
} entry;
entry *entries;

pthread_mutex_t i_lock;
pthread_mutex_t time_lock;
pthread_mutex_t e_lock;
pthread_mutex_t vcd_file_lock;
pthread_barrier_t barrier;
pthread_cond_t cond;

FILE *vcd_file;
FILE *bin_file;
FILE *name_file;
int file_size = 0;
int bin_entry_size;
int e_index = 0;

int current_time = 0;

void dump_header_entries() {
    for(int i=0; i<NUM_ENTRIES; i++) {
        printf("\nNode(%s, %s)\n", header_entries[i].hex, header_entries[i].name);
        printf("    hex: %s\n", header_entries[i].hex);
        printf("    name: %s\n", header_entries[i].name);
    }
}

void remove_extra_spaces(char *str) {
    int n = strlen(str);
    int i = 0, j = 0;
    int space_count = 0;
    while (str[i]) {
        if (!isspace(str[i])) {
            if (space_count > 0) {
                str[j++] = ' ';
                space_count = 0;
            }
            str[j++] = str[i];
        } else {
            space_count++;
        }
        i++;
    }
    str[j] = '\0';
    // Remove any leading spaces
    if (j > 0 && str[0] == ' ') {
        memmove(str, str + 1, j);
        str[--j] = '\0';
    }
}

void read_names() {
    name_file = fopen("c_methods/names.txt", "r");
    if (name_file == NULL) {
        perror("Error opening names.txt file");
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;

    while (fgets(line, sizeof(line), name_file) && count < NUM_ENTRIES) {
        line[strcspn(line, "\n")] = '\0'; // Remove the newline character, if present

        char *token = strtok(line, ","); // Split the line by the comma

        if (token != NULL) {
            strncpy(header_entries[count].hex, token, MAX_LINE_LENGTH); // 1st string
            remove_extra_spaces(header_entries[count].hex);
            token = strtok(NULL, ",");

            if (token != NULL) {
                strncpy(header_entries[count].name, token, MAX_LINE_LENGTH); // 2nd string
                remove_extra_spaces(header_entries[count].name);

                // if(DEBUG) printf("hex: %s\nname: %s\n", header_entries[count].hex, header_entries[count].name);

                count++;
            }
        }
    }
}

int create_header() {
    vcd_file = fopen("c_methods/output.vcd", "wb");
    if (vcd_file == NULL) {
        perror("Error opening output.vcd vcd_file");
        return 1;
    }

    time_t current_time;
    time(&current_time);
    struct tm *local_time = localtime(&current_time);
    char time_buffer[20];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_time);

    fprintf(vcd_file, "$date\n     %s\n$end\n", time_buffer);
    fprintf(vcd_file, "$version\n  LG211 VCDv1 vcd_file\n$end\n");
    fprintf(vcd_file, "$timescale\n    1ns\n$end\n");

    read_names();

    // if(DEBUG) dump_header_entries();

    for(int i=0; i<NUM_ENTRIES; i++) {
        fprintf(vcd_file, "$var wire 1 %s %s $end\n", header_entries[i].hex, header_entries[i].name); //header_entry's id-name
        // if(DEBUG) printf("Node(%s, %s)\n", header_entries[i].hex, header_entries[i].name);
    }

    fprintf(vcd_file, "$enddefinitions $end\n");
    fprintf(vcd_file, "#0\n$dumpvars\n");

    for(int i=0; i<NUM_ENTRIES; i++) {
        fprintf(vcd_file, "b0 %s\n", header_entries[i].hex); //header_entry's initial value
    }

    fclose(vcd_file);

    return 0;;
}

long get_file_size() {
    FILE* temp_file = fopen("c_methods/output.bin", "rb");
    if (temp_file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Seek to the end of the file
    fseek(temp_file, 0, SEEK_END);

    // Get the file size
    int size = (int)ftell(temp_file);
    if (size == -1L) {
        perror("Error getting file size");
        fclose(temp_file);
        return 1;
    }
    if(DEBUG) printf("bin size: %d\n\n", size);

    fclose(temp_file);

    return size;
}

void dump_entries() {
    for(int i=0; i<e_index; i++) {
        printf("entries[%d]\n", i);
        printf("    32_bit: 0x%" PRIx32 "\n", entries[i].id);
        printf("    64_bit: %lu\n\n", entries[i].time_stamp);
    };
}

void add_tail(uint32_t id, uint64_t time_stamp) {
    entries[e_index].id = id;
    entries[e_index].time_stamp = time_stamp;
    e_index++;
}

void *read_bin(void* arg) {
    FILE *bin_file = fopen("c_methods/output.bin", "rb");
    if (bin_file == NULL) {
        perror("Error opening bin_file");
        exit;
    }

    uint32_t id;
    uint64_t time_stamp;

    long tid = (long)arg;

    // Read and print 32-bit and 64-bit values

    if(TEST) {
        size_t byte_offset = ((int)(tid) * ((file_size / bin_entry_size) / THREAD_COUNT)) * bin_entry_size; //id * ((120 / 12) / 3)
        // printf("Thread: %lu - offset: %ld\n\n", tid, byte_offset);

        fseek(bin_file, byte_offset, SEEK_SET);

        while(1) {
            int e_position = (int)ftell(bin_file);
            if((tid+1 < THREAD_COUNT && e_position >= ((int)(tid+1) * ((file_size / bin_entry_size) / THREAD_COUNT) * bin_entry_size)) || e_position == file_size) {
                break;
            } else {
                if(fread(&id, 1, sizeof(uint32_t), bin_file) < sizeof(uint32_t)) {
                    break;
                } else {
                    if(fread(&time_stamp, 1, sizeof(uint64_t), bin_file) < sizeof(uint64_t)) {
                        break;
                    } else {
                        pthread_mutex_lock(&i_lock);
                        add_tail(id, time_stamp);
                        pthread_mutex_unlock(&i_lock);
                        // if(DEBUG) printf("THREAD: %lu - position: %d\n", tid, e_position);
                        // if(DEBUG) printf("Read 32-bit value: 0x%" PRIx32 "\n", id);
                        // if(DEBUG) printf("    Read 64-bit value: %lu\n\n", time_stamp);
                    }
                }
            }
        }

    } else {
        while (fread(&id, sizeof(uint32_t), 1, bin_file) == 1) {
            // printf("Thread %ld: 32\n", tid);
            if(DEBUG) printf("Read 32-bit value: 0x%" PRIx32 "\n", id);

            if(fread(&time_stamp, sizeof(uint64_t), 1, bin_file) == 1) {
                // printf("    Thread %ld: 64\n", tid);
                if(DEBUG) printf("    Read 64-bit value: 0x%" PRIx64 "\n", time_stamp);

                if(time_stamp > current_time) {

                    current_time = time_stamp;
                    fprintf(vcd_file, "#%d\n", current_time);

                    if(DEBUG) printf("    Time: %d\n\n", current_time);
                }

                uint8_t e_bit = (id<<24) > 0 ? 1 : 0;
                fprintf(vcd_file, "b%d 0x%" PRIx32 "\n", e_bit, (id>>24));

            } else { // Handle the case where the 64-bit value cannot be read
                perror("Error reading 64-bit value");
                break;
            }
        }
    }
}

int thread_binary() {
    pthread_t threads[THREAD_COUNT];

    vcd_file = fopen("c_methods/output.vcd", "a");
    if (vcd_file == NULL) {
        perror("Error opening vcd_file");
        exit;
    }

    for(long i=0; i<THREAD_COUNT; i++) {
        // printf("\nIn main: creating thread %ld\n", i);
        int rc = pthread_create(&threads[i], NULL, read_bin, (void *)i);

        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            return 1;
        }
    }

    printf("\n");
    for(int i=0; i<THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(vcd_file);

    return 0;
}

int main() {
    int rv = 0;
    bin_entry_size = sizeof(uint64_t) + sizeof(uint32_t);

    file_size = get_file_size(); //10 elements with each being 32+64=96 bits or 4+8=12 bytes so it returns 120 bytes in total for size
    entries = (entry*)malloc(file_size*(sizeof(entry)));

    rv = create_header(); // Works and creates header

    pthread_mutex_init(&i_lock, NULL);
    pthread_mutex_init(&time_lock, NULL);
    pthread_mutex_init(&e_lock, NULL);
    pthread_barrier_init(&barrier, NULL, THREAD_COUNT);
    pthread_cond_init(&cond, NULL);

    // pthread_mutex_init(&vcd_file_lock, NULL);

    rv = thread_binary();

    if(DEBUG) dump_entries();

    pthread_mutex_destroy(&i_lock);
    pthread_mutex_destroy(&time_lock);
    pthread_mutex_destroy(&e_lock);
    pthread_barrier_destroy(&barrier);
    pthread_cond_destroy(&cond);

    // pthread_mutex_destroy(&vcd_file_lock);

    printf("VCD vcd_file created successfully!\n");
    return rv;
}
