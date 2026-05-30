# Distributed Tumor Treatment Simulator

A distributed healthcare simulation platform developed as an Operating Systems mini-project.

The system models interactions between administrators, doctors, and patients while simulating tumor growth and treatment progression. The project was designed to demonstrate practical operating systems concepts through a realistic healthcare scenario.

---

## Project Overview

The simulator manages:

* Doctor registration and assignment
* Patient management
* Tumor growth simulation
* Chemotherapy and radiation treatment workflows
* Patient treatment preferences and tolerance levels
* Real-time status reporting

The system supports three user roles:

### Administrator

* Add doctors
* Add patients
* Assign patients to doctors
* View system-wide information
* Execute simulation rounds

### Doctor

* View assigned patients
* Apply treatments
* Monitor patient health and tumor progression

### Patient

* View personal treatment status
* Configure treatment tolerance
* Request reduced treatment intensity

---

## Operating Systems Concepts Demonstrated

### Concurrency Control

* POSIX mutexes (`pthread_mutex`)
* Reader-writer locks (`pthread_rwlock`)
* Protected access to shared data structures

### Inter-Process Communication (IPC)

* Named FIFOs
* Parent-child communication
* Dedicated logging process

### Synchronization

* POSIX semaphores
* Race condition prevention
* Safe concurrent updates

### Socket Programming

* TCP server implementation
* Remote status reporting
* Client-server architecture

### Multithreading

* Dedicated socket server thread
* Concurrent request handling

### Process Management

* Process creation using `fork()`
* Child logger process
* Process coordination and cleanup

### Data Consistency

* Protected read-modify-write operations
* Shared-state synchronization
* Consistent patient record management

---

## Architecture

```text
Admin / Doctor / Patient
            |
            v
      Simulation Engine
            |
    -------------------
    |                 |
    v                 v
 FIFO Logger     TCP Server
 (IPC)           (Sockets)
```

---

## Technologies Used

* C
* POSIX Threads
* POSIX Semaphores
* TCP/IP Sockets
* Linux System Calls
* Makefile Build System

---

## Build

```bash
make
```

## Run

```bash
./tumor_simulator
```

---

## Learning Outcomes

This project demonstrates practical applications of:

* Synchronization mechanisms
* Inter-process communication
* Network programming
* Concurrent system design
* Resource protection
* Operating systems abstractions

---

## Author

Suhasini Madala
