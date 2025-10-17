# OS Lab 1 – Inter-Process Communication (IPC)

This project is part of the Operating System course (Lab 1) and focuses on implementing **Inter-Process Communication (IPC)** using message queues, shared memory, and semaphores in Linux.

## 🧠 Overview
The goal of this lab is to design and implement two programs — `sender` and `receiver` — that communicate through IPC mechanisms.  
Each program uses wrapper functions to simplify the use of system calls for message exchange and synchronization.

### Features
- Implementation of **two separate processes**: `sender` and `receiver`
- Use of **System V IPC** (message queues / shared memory / semaphores)
- Robust **error handling** and **wrapper functions** for system calls
- Demonstration of **synchronized data transfer** between processes
- Tested in a **Linux (Ubuntu)** virtual machine environment

---

## 📁 File Structure
├── sender.c # Sends messages to the receiver process
├── receiver.c # Receives messages from the sender process
├── receiver.h
├── sender.h 
├── Makefile # For compiling sender and receiver
└── README.md # Project documentation

### Step 1: Compile
make
Step 2: Run two terminals
Terminal 1 (Receiver):

./receiver
Terminal 2 (Sender):

./sender
🧩 Example Output
Receiver Terminal:

[Receiver] Waiting for message...
[Receiver] Message received: "Hello from sender!"
Sender Terminal:

[Sender] Sending message: "Hello from sender!"
[Sender] Message successfully sent.
🧰 Development Environment
OS: Ubuntu 22.04 LTS (VirtualBox)

Language: C

Compiler: GCC (GNU Compiler Collection)

Build Tool: Make

🧑‍💻 Author
Eason Peng (彭以呈)
Department of Computer Science, National Cheng Kung University (NCKU)
GitHub: @PYCHS

📚 Notes
This lab serves as an introduction to IPC concepts in Linux, helping students understand:

Process creation and communication

Synchronization using semaphores

Safe resource sharing via shared memory

Future labs will build upon this foundation to explore process scheduling, multithreading, and virtual memory.
