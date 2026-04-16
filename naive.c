#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Define constants 
#define MAX_NUMBER_OF_WORDS 18000000 
#define MAX_UNIQUE_WORDS 300000
#define MAX_WORD_LENGTH 100

// Define a structure to hold word and its frequency
typedef struct {
    char word[MAX_WORD_LENGTH];  // Word string
    int frequency;               // Frequency of the word
} WordFrequency;

// Global variables
WordFrequency *tempArr;           // Array to store words and their frequencies temporarily
WordFrequency *arrWord;           // Array to store unique words and their frequencies
int wordCount = 0;                // Total count of words processed
int total_unique_words = 0;       // Total count of unique words found

// Function declarations
void tolowercase(char *);          // Function to convert a word to lowercase
int find_word(char *);             // Function to find a word in the unique words array
void readDataFile();               // Function to read data from a file
void count_words();                // Function to count word frequencies
int compare(const void *, const void *);  // Function to compare two WordFrequency items (for sorting)
void printTop10Words();            // Function to print the top 10 most frequent words

int main() {
    struct timespec start_time, end_time, section_start, section_end;
    double total_time = 0.0, read_time = 0.0, count_words_time = 0.0, sort_print_time = 0.0, serial_time = 0.0;

    clock_gettime(CLOCK_MONOTONIC, &start_time);  // Record the start time of the program

    // Allocate memory for the arrays
    tempArr = malloc(MAX_NUMBER_OF_WORDS * sizeof(WordFrequency));
    arrWord = malloc(MAX_UNIQUE_WORDS * sizeof(WordFrequency));

    // Measure the time taken to read the data file
    clock_gettime(CLOCK_MONOTONIC, &section_start);
    readDataFile();
    clock_gettime(CLOCK_MONOTONIC, &section_end);
    read_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

    // Measure the time taken to count the word frequencies
    clock_gettime(CLOCK_MONOTONIC, &section_start);
    count_words();
    clock_gettime(CLOCK_MONOTONIC, &section_end);
    count_words_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

    // Measure the time taken to sort and print the top 10 words
    clock_gettime(CLOCK_MONOTONIC, &section_start);
    printTop10Words();
    clock_gettime(CLOCK_MONOTONIC, &section_end);
    sort_print_time = (section_end.tv_sec - section_start.tv_sec) + (section_end.tv_nsec - section_start.tv_nsec) / 1000000000.0;

    // Record the total time taken
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    
    // Calculate serial execution time
    serial_time = read_time + count_words_time + sort_print_time;

    // Output the time statistics
    printf("Total time: %f seconds\n", total_time);
    printf("Serial time(Reading + Counting words + Sorting + Printing): %f seconds\n", serial_time);

    // Free allocated memory for arrays
    free(tempArr);
    free(arrWord);

    return 0;
}

// Function to convert a word to lowercase
void tolowercase(char *word) {
    int i = 0;
    while (word[i]) {  // Iterate over each character of the word
        word[i] = tolower(word[i]);  // Convert to lowercase
        i++;
    }
}

// Function to find a word in the array of unique words
int find_word(char *word) {
    for (int i = 0; i < total_unique_words; i++) {
        if (strcmp(arrWord[i].word, word) == 0) {  // Compare word with each unique word
            return i;  // Return index if word is found
        }
    }
    return -1;  // Return -1 if word is not found
}

// Function to read the data file and store words
void readDataFile() {
    FILE *data = fopen("text8.txt", "r");  // Open the file for reading
    if (data == NULL) {  // Check for file opening error
        printf("Error opening 'text8.txt' file.\n");
        exit(1);  // Exit if file cannot be opened
    }
    
    printf("File 'text8.txt' opened.\n");
    
    char word[MAX_WORD_LENGTH];
    while (fscanf(data, "%s", word) != EOF) {  // Read words from the file
        if (wordCount >= MAX_NUMBER_OF_WORDS) {  // Check for maximum word count
            printf("Reached maximum number of words (%d).\n", MAX_NUMBER_OF_WORDS);
            break;
        }

        tolowercase(word);  // Convert the word to lowercase
        strcpy(tempArr[wordCount].word, word);  // Store the word in the temporary array
        tempArr[wordCount].frequency = 1;  // Initialize frequency to 1
        wordCount++;  // Increment the word count
    }
    fclose(data);  // Close the file
}

// Function to count the frequency of each word
void count_words() {
    for (int i = 0; i < wordCount; i++) {
        char word[MAX_WORD_LENGTH];
        strcpy(word, tempArr[i].word);  // Copy the word to avoid modifying tempArr

        // Check if the word already exists in arrWord
        int word_idx = find_word(word);
        if (word_idx == -1) {  // If word is not found, add it as a new unique word
            if (total_unique_words < MAX_UNIQUE_WORDS) {
                strcpy(arrWord[total_unique_words].word, word);  // Store the word
                arrWord[total_unique_words].frequency = 1;  // Set its frequency to 1
                total_unique_words++;  // Increment the unique word count
            } else {
                printf("Error: Maximum unique word limit reached.\n");
                exit(1);  // Exit if the unique word limit is reached
            }
        } else {  // If word is found, increment its frequency
            arrWord[word_idx].frequency++;
        }
    }
}

// Comparison function for qsort to sort words by frequency in descending order
int compare(const void *a, const void *b) {
    WordFrequency *wordA = (WordFrequency *)a;
    WordFrequency *wordB = (WordFrequency *)b;
    return wordB->frequency - wordA->frequency;  // Compare frequencies
}

// Function to print the top 10 most frequent words
void printTop10Words() {
    qsort(arrWord, total_unique_words, sizeof(WordFrequency), compare);  // Sort words by frequency

    printf("Top 10 Most Frequent Words:\n");
    for (int i = 0; i < 10 && i < total_unique_words; i++) {  // Print the top 10 words
        printf("%d. %s - %d\n", i + 1, arrWord[i].word, arrWord[i].frequency);
    }
}

