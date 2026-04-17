📊 Word Frequency Analysis using Parallel Computing
📌 Overview

This project analyzes word frequency in large datasets using three approaches: naive (sequential), multiprocessing, and multithreading. It demonstrates how parallel computing improves performance and scalability.

⚙️ Features

Counts word frequency and extracts most frequent words
Implements:

Naive (sequential) approach
Multiprocessing using fork()
Multithreading using pthread

Compares performance using execution time

Evaluates scalability using Amdahl’s Law
🧪 Performance

Parallel approaches significantly reduce execution time compared to the naive method, especially for large datasets

🛠️ Technologies Used

C Programming
pthread (Multithreading)
fork, pipes (Multiprocessing)
Linux System Calls
▶️ How to Run

Compile the program:

gcc -o program file.c -pthread

Run:


./program

🎯 Key Concepts

Parallel computing
Process vs thread management
Synchronization (mutex, semaphores)
Performance analysis
