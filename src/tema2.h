#ifndef TEMA2_H
#define TEMA2_H
#define _GNU_SOURCE


#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100
#define MAX_SEEDS 10
#define ackSize 3
#define ACK "ACK"

typedef struct {
    ssize_t numberOfChunks;
    char filename[MAX_FILENAME];
    char chunks[MAX_CHUNKS + 1][HASH_SIZE + 1];
}File;

typedef struct {
    ssize_t numberOfFilesOwned;
    ssize_t numberOfFilesWanted;
    int rank;
    File filesOwned[MAX_FILES];
    File filesWanted[MAX_FILES];
}Client;

typedef struct {
    File file;
    int numberOfSeeds;
    int seeds[MAX_SEEDS];
}Swarm;

// Function declarations
void *download_thread_func(void *arg);
void *upload_thread_func(void *arg);
void tracker(int numtasks, int rank);
void peer(int numtasks, int rank);
void readInputFile(const char *inputFileName, Client *client);
void sendFileOwnedToTracker(Client *client);
void receiveFilesOwnedFromSeeds(Client *clients, Swarm *swarms, int *numberOfSwarms,  int numtasks, MPI_Status status);
int checkIfSwarmExists(Swarm *swarms, char *filename);
void printSwarms(Swarm *swarms, int numberOfSwarms);
void sendACKToClients(int numtasks);
void receiveACKFromTracker(int rank);
void print(Client *client);


#endif // TEMA2_H