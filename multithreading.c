#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#define MAX_NUMBER_OF_WORDS 18000000 
#define MAX_UNIQUE_WORDS 300000
#define MAX_WORD_LENGTH 100
#define THREADS_NUMBER 8

typedef struct {
    char word[MAX_WORD_LENGTH];
    int frequency;
} WordFrequency;

WordFrequency *tempArr;  // Temporary array to hold words and their initial frequencies
WordFrequency *arrWord;  // Array to store unique words and their final frequencies
int wordCount = 0;       // Total number of words read from the file
int total_unique_words = 0;  // Total number of unique words identified
pthread_mutex_t mutex;  // Mutex lock for synchronizing access to shared resources

// Function prototypes
void tolowercase(char *);  // Converts a string to lowercase
int find_word(char *);  // Finds a word in arrWord, returns its index or -1 if not found
void readDataFile();  // Reads the data file and stores words in tempArr
void *count_words(void *);  // Counts the frequency of words (runs in parallel)
int compare(const void *, const void *);  // Comparison function for sorting words by frequency
void printTop10Words();  // Prints the top 10 most frequent words

int main() {
      struct timespec start_time, end_time, section_start, section_end;
      double total_time = 0.0, read_time = 0.0, count_words_time = 0.0, sort_print_time = 0.0, serial_time = 0.0;

      clock_gettime(CLOCK_MONOTONIC, &start_time);  // Record the start time of the program
      
      // Allocate memory for tempArr and arrWord dynamically
      tempArr = malloc(MAX_NUMBER_OF_WORDS * sizeof(WordFrequency));
      arrWord = malloc(MAX_UNIQUE_WORDS * sizeof(WordFrequency));
      
      // Measure the time for reading the file
      clock_gettime(CLOCK_MONOTONIC, &section_start);
      readDataFile();  // Read data from the file
      clock_gettime(CLOCK_MONOTONIC, &section_end);
      read_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

      pthread_mutex_init(&mutex, NULL);  // Initialize mutex lock 

      pthread_t threads[THREADS_NUMBER];  // Array to hold thread IDs
      int thread_ids[THREADS_NUMBER];  // Array to store thread indices
      clock_gettime(CLOCK_MONOTONIC, &section_start);
      
      // Create threads for counting words
      for (int i = 0; i < THREADS_NUMBER; i++) {
          thread_ids[i] = i;
          pthread_create(&threads[i], NULL, count_words, &thread_ids[i]);  // Create thread for each portion of words
      }

      // Wait for all threads to complete
      for (int i = 0; i < THREADS_NUMBER; i++) {
          pthread_join(threads[i], NULL);  // Join threads after completion
      }
      clock_gettime(CLOCK_MONOTONIC, &section_end);
      count_words_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

      // Measure the time for sorting and printing the top 10 words
      clock_gettime(CLOCK_MONOTONIC, &section_start);
      printTop10Words();  // Print the top 10 most frequent words
      clock_gettime(CLOCK_MONOTONIC, &section_end);
      sort_print_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

      clock_gettime(CLOCK_MONOTONIC, &end_time);  // Record the end time of the program
      total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

      // Print the total time taken for the entire program
      printf("Total time: %f seconds\n", total_time);
      serial_time = read_time + sort_print_time;  // Calculate time taken by serial operations
      printf("Serial time(Reading + Sorting + Printing): %f seconds\n", serial_time);
      printf("Parallel time(Counting words): %f seconds\n", count_words_time);

      free(tempArr);  // Free dynamically allocated memory
      free(arrWord);
      
      pthread_mutex_destroy(&mutex);  // Destroy mutex lock

      return 0;
}

void tolowercase(char *word) {
    int i = 0;
    while (word[i]) {
        word[i] = tolower(word[i]);  // Convert each character to lowercase
        i++;
    }
}

int find_word(char *word) {
    for (int i = 0; i < total_unique_words; i++) {
        if (strcmp(arrWord[i].word, word) == 0) {
            return i;  // Return index if word is found
        }
    }
    return -1;  // Return -1 if word is not found
}

void readDataFile() {
    FILE *data = fopen("text8.txt", "r");  // Open the data file for reading
    if (data == NULL) {
        printf("Error opening 'text8.txt' file.\n");
        exit(1);
    }
    
    printf("File 'text8.txt' opened.\n");
    
    char word[MAX_WORD_LENGTH];
    while (fscanf(data, "%s", word) != EOF) {  // Read words from the file
        // Ensure bounds
        if (wordCount >= MAX_NUMBER_OF_WORDS) {
            printf("Reached maximum number of words (%d).\n", MAX_NUMBER_OF_WORDS);
            break;
        }

        // Convert to lowercase and store in tempArr
        tolowercase(word);
        strcpy(tempArr[wordCount].word, word);
        tempArr[wordCount].frequency = 1;  // Initialize frequency to 1
        wordCount++;
    }
    fclose(data);  // Close the file after reading
}

void *count_words(void *param) {
    int thread_num = *(int *)param;  // Retrieve the thread number

    // Calculate the number of words each thread will process
    int words_per_thread = (wordCount + THREADS_NUMBER - 1) / THREADS_NUMBER; // Round up to cover all words

    // Calculate the starting and ending indices for this thread
    int starting_index = thread_num * words_per_thread;
    int ending_index = starting_index + words_per_thread;

    // Ensure the ending index does not exceed the total number of words
    if (ending_index > wordCount) {
        ending_index = wordCount;
    }

    // Process the assigned words for this thread
    for (int i = starting_index; i < ending_index; i++) {
        char word[MAX_WORD_LENGTH];
        strcpy(word, tempArr[i].word);
        tolowercase(word);  // Convert word to lowercase
        int word_idx = find_word(word);  // Find the word in arrWord
        if (word_idx == -1) {  // If word not found
            pthread_mutex_lock(&mutex);  // Lock the mutex to protect shared data
            if (total_unique_words < MAX_UNIQUE_WORDS) {
                strcpy(arrWord[total_unique_words].word, word);  // Add new word
                arrWord[total_unique_words].frequency = 1;
                total_unique_words++;
            }
            pthread_mutex_unlock(&mutex);  // Unlock the mutex after modifying shared data
        } else {  // If word found
            pthread_mutex_lock(&mutex);  // Lock mutex to update word frequency safely
            arrWord[word_idx].frequency++;
            pthread_mutex_unlock(&mutex);  // Unlock mutex after updating frequency
        }
    }
    return NULL;
}

// Comparison function for qsort to sort by frequency in descending order
int compare(const void *a, const void *b) {
    WordFrequency *wordA = (WordFrequency *)a;
    WordFrequency *wordB = (WordFrequency *)b;
    return wordB->frequency - wordA->frequency;  // Compare based on frequency
}

void printTop10Words() {
    // Sort the arrWord array by frequency using qsort
    qsort(arrWord, total_unique_words, sizeof(WordFrequency), compare);

    // Print the top 10 most frequent words
    printf("Top 10 Most Frequent Words:\n");
    for (int i = 0; i < 10 && i < total_unique_words; i++) {
        printf("%d. %s - %d\n", i + 1, arrWord[i].word, arrWord[i].frequency);  // Print word and its frequency
    }
}

