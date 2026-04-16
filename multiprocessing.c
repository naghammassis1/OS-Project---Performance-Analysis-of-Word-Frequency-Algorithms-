#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

// Define constants 
#define MAX_NUMBER_OF_WORDS 18000000 
#define MAX_UNIQUE_WORDS 300000
#define MAX_WORD_LENGTH 100
#define PROCESSES_NUMBER 8

// Structure to store each word and its frequency
typedef struct {
    char word[MAX_WORD_LENGTH];
    int frequency;
} WordFrequency;

WordFrequency *tempArr; // Array to hold all words temporarily
WordFrequency *arrWord; // Array to store unique words and their frequencies
int wordCount = 0; // Total word count read from the file
int *total_unique_words; // Total number of unique words found
sem_t *semaphore; // Semaphore for synchronizing access to shared memory

// Function declarations
void tolowercase(char *); // Function to convert word to lowercase
int find_word(char *); // Function to find if a word already exists
void readDataFile(); // Function to read words from the data file
void count_words(int process_num); // Function for each process to count word frequencies
int compare(const void *, const void *); // Comparison function for sorting words by frequency
void printTop10Words(); // Function to print the top 10 most frequent words

int main() {
    struct timespec start_time, end_time, section_start, section_end;
    double total_time = 0.0, read_time = 0.0, count_words_time = 0.0, sort_print_time = 0.0, serial_time = 0.0;

    clock_gettime(CLOCK_MONOTONIC, &start_time);  // Record the start time of the program

    // Allocate shared memory for tempArr, arrWord, and total_unique_words using mmap
    tempArr = mmap(NULL, MAX_NUMBER_OF_WORDS * sizeof(WordFrequency), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    arrWord = mmap(NULL, MAX_UNIQUE_WORDS * sizeof(WordFrequency), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    total_unique_words = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    if (semaphore == MAP_FAILED) {
        perror("mmap failed"); // Error checking for mmap
        exit(1);
    }

    // Initialize the semaphore for process synchronization (value 1 for mutual exclusion)
    if (sem_init(semaphore, 1, 1) == -1) {
        perror("sem_init failed"); // Error initializing the semaphore
        exit(1);
    }

    // Check if mmap failed for other memory regions
    if (tempArr == MAP_FAILED || arrWord == MAP_FAILED || total_unique_words == MAP_FAILED) {
        printf("mmap failed");
        exit(1);
    }

    *total_unique_words = 0;  // Initialize total_unique_words to 0

    // Measure the time for reading the file
    clock_gettime(CLOCK_MONOTONIC, &section_start);
    readDataFile();
    clock_gettime(CLOCK_MONOTONIC, &section_end);
    read_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

    // Create child processes to count word frequencies in parallel
    int pids[PROCESSES_NUMBER];
    clock_gettime(CLOCK_MONOTONIC, &section_start);
    for (int i = 0; i < PROCESSES_NUMBER; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            // Child process: call count_words function to process the word frequency counting
            count_words(i);
            exit(0); // Terminate child process after task is complete
        }
    }

    // Parent process waits for all child processes to finish
    for (int i = 0; i < PROCESSES_NUMBER; i++) {
        wait(NULL); // Wait for each child process
    }
    clock_gettime(CLOCK_MONOTONIC, &section_end);
    count_words_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

    // Measure the time for sorting and printing the top 10 words
    clock_gettime(CLOCK_MONOTONIC, &section_start);
    printTop10Words();
    clock_gettime(CLOCK_MONOTONIC, &section_end);
    sort_print_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

    // Record the total time and print results
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    
    // Output total and serial times
    printf("Total time: %f seconds\n", total_time);
    serial_time = read_time + sort_print_time;
    printf("Serial time(Reading + Sorting + Printing): %f seconds\n", serial_time);
    printf("Parallel time(Counting words): %f seconds\n", count_words_time);

    // Clean up allocated memory and destroy the semaphore
    munmap(tempArr, MAX_NUMBER_OF_WORDS * sizeof(WordFrequency));
    munmap(arrWord, MAX_UNIQUE_WORDS * sizeof(WordFrequency));
    munmap(total_unique_words, sizeof(int));

    if (sem_destroy(semaphore) == -1) {
        perror("sem_destroy failed"); // Error destroying the semaphore
        exit(1);
    }

    // Unmap the semaphore from memory
    if (munmap(semaphore, sizeof(sem_t)) == -1) {
        perror("munmap failed"); // Error unmapping the semaphore
        exit(1);
    }

    return 0;
}

// Function to convert a word to lowercase
void tolowercase(char *word) {
    int i = 0;
    while (word[i]) {
        word[i] = tolower(word[i]);
        i++;
    }
}

// Function to find a word in the arrWord array. Returns the index or -1 if not found.
int find_word(char *word) {
    for (int i = 0; i < (*total_unique_words); i++) {
        if (strcmp(arrWord[i].word, word) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to read words from the file and store them in tempArr
void readDataFile() {
    FILE *data = fopen("text8.txt", "r");
    if (data == NULL) {
        printf("Error opening 'text8.txt' file.\n");
        exit(1);
    }
    printf("File 'text8.txt' opened.\n");
    
    // Read words from file
    char word[MAX_WORD_LENGTH];
    while (fscanf(data, "%s", word) != EOF) {
        if (wordCount >= MAX_NUMBER_OF_WORDS) {
            printf("Reached maximum number of words (%d).\n", MAX_NUMBER_OF_WORDS);
            break;
        }
        
        // Convert to lowercase and store in tempArr
        tolowercase(word);
        strcpy(tempArr[wordCount].word, word);
        tempArr[wordCount].frequency = 1;
        wordCount++;
    }
    fclose(data);
}

// Function to count word frequencies in parallel (each process handles a portion of words)
void count_words(int process_num) {
    int words_per_process = (wordCount + PROCESSES_NUMBER - 1) / PROCESSES_NUMBER; // Ceiling division
    int start_index = process_num * words_per_process;
    int end_index = start_index + words_per_process;

    // Ensure the end_index does not exceed the total word count
    if (end_index > wordCount) {
        end_index = wordCount;
    }

    // Each process counts word frequencies within its range
    for (int i = start_index; i < end_index; i++) {
        char word[MAX_WORD_LENGTH];
        strcpy(word, tempArr[i].word);
        tolowercase(word);

        // Synchronize access to shared data using semaphore
        int word_idx = find_word(word);
        if (word_idx == -1) {
            sem_wait(semaphore); 
            if ((*total_unique_words) < MAX_UNIQUE_WORDS) {
                strcpy(arrWord[(*total_unique_words)].word, word);
                arrWord[(*total_unique_words)].frequency = 1;
                (*total_unique_words)++;
            }
            sem_post(semaphore);
        } else {
            sem_wait(semaphore); 
            arrWord[word_idx].frequency++;
            sem_post(semaphore);
        }
    }
}

// Comparison function for qsort to sort words by frequency in descending order
int compare(const void *a, const void *b) {
    WordFrequency *wordA = (WordFrequency *)a;
    WordFrequency *wordB = (WordFrequency *)b;
    return wordB->frequency - wordA->frequency; // Sort in descending order
}

// Function to print the top 10 most frequent words
void printTop10Words() {
    qsort(arrWord, (*total_unique_words), sizeof(WordFrequency), compare);  // Sort arrWord by frequency

    // Print the top 10 most frequent words
    printf("Top 10 Most Frequent Words:\n");
    for (int i = 0; i < 10 && i < (*total_unique_words); i++) {
        printf("%d. %s - %d\n", i + 1, arrWord[i].word, arrWord[i].frequency);
    }
}

