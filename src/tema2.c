#include "tema2.h"

void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;

    return NULL;
}

void *upload_thread_func(void *arg)
{
    int rank = *(int*) arg;

    return NULL;
}

/*
    Functie care verifica daca un fisier exista deja in lista de swarms.
    Daca exista, returneaza indexul swarm-ului, altfel returneaza -1.
*/
int checkIfSwarmExists(Swarm *swarms, char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(swarms[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}

/*
    Aceasta functie primeste de la fiecare peer numarul de fisiere detinute
    si informatiile despre acestea.
*/
void receiveFilesOwnedFromSeeds(Client *clients, Swarm *swarms, int *numberOfSwarms,  int numtasks, MPI_Status status) {
    for (int i = 1; i < numtasks; i++) {
        MPI_Recv(&clients[i - 1].numberOfFilesOwned, 1, MPI_LONG, i, 0, MPI_COMM_WORLD, &status);
        clients[i - 1].rank = i;
        for (int j = 0; j < clients[i - 1].numberOfFilesOwned; j++) {
            MPI_Recv(&clients[i - 1].filesOwned[j].filename, MAX_FILENAME, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&clients[i - 1].filesOwned[j].numberOfChunks, 1, MPI_LONG, i, 0, MPI_COMM_WORLD, &status);
            int swarmIndex = checkIfSwarmExists(swarms, clients[i - 1].filesOwned[j].filename);
            if(swarmIndex == -1) {
                strcpy(swarms[*numberOfSwarms].filename, clients[i - 1].filesOwned[j].filename);
                swarms[*numberOfSwarms].numberOfSeeds = 1;
                swarms[*numberOfSwarms].seeds[0] = i;
                (*numberOfSwarms)++;
            } else {
                swarms[swarmIndex].seeds[swarms[swarmIndex].numberOfSeeds] = i;
                swarms[swarmIndex].numberOfSeeds++;
            }
            for (int k = 0; k < clients[i - 1].filesOwned[j].numberOfChunks; k++) {
                MPI_Recv(&clients[i - 1].filesOwned[j].chunks[k], HASH_SIZE, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
            }
        }
    }
}

/*
    Functie care afiseaza informatiile despre swarms.
*/
void printSwarms(Swarm *swarms, int numberOfSwarms) {
    for (int i = 0; i < numberOfSwarms; i++) {
        printf("Swarm %d: %s\n", i, swarms[i].filename);
        printf("Seeds: ");
        for (int j = 0; j < swarms[i].numberOfSeeds; j++) {
            printf("%d ", swarms[i].seeds[j]);
        }
        printf("\n");
    }
}

/*
    Functie care trimite mesajul de ACK catre toti
    clientii, anuntandu-i ca a primit toate informatiile
    necesare si ca poate incepe comunicarea.
*/
void sendACKToClients(int numtasks) {
    for (int i = 1; i < numtasks; i++) {
        MPI_Send(ACK, ackSize, MPI_CHAR, i, 0, MPI_COMM_WORLD);
    }
}

/*
    Functie care trimite informatiile despre fisierele detinute
    catre tracker.
*/
void sendFileOwnedToTracker(Client *client) {
    MPI_Send(&client->numberOfFilesOwned, 1, MPI_LONG, TRACKER_RANK, 0, MPI_COMM_WORLD);
    for (int i = 0; i < client->numberOfFilesOwned; i++) {
        MPI_Send(&client->filesOwned[i].filename, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        MPI_Send(&client->filesOwned[i].numberOfChunks, 1, MPI_LONG, TRACKER_RANK, 0, MPI_COMM_WORLD);
        for (int j = 0; j < client->filesOwned[i].numberOfChunks; j++) {
            MPI_Send(&client->filesOwned[i].chunks[j], HASH_SIZE, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        }
    }
}

/*
    Aceasta functie citeste datele din fisierul de input
    si le salveaza in structura Client.
*/
void readInputFile(const char *inputFileName, Client *client) {
    FILE *inputFile = fopen(inputFileName, "r");
    if (inputFile == NULL) {
        printf("Eroare la deschiderea fisierului de input\n");
        exit(-1);
    }

    fscanf(inputFile, "%ld", &client->numberOfFilesOwned);
    for (int i = 0; i < client->numberOfFilesOwned; i++) {
        File file;
        fscanf(inputFile, "%s %ld", file.filename, &file.numberOfChunks);
        for (int j = 0; j < file.numberOfChunks; j++) {
            fscanf(inputFile, "%s", file.chunks[j]);
        }
        client->filesOwned[i] = file;
    }

    fscanf(inputFile, "%ld", &client->numberOfFilesWanted);
    for (int i = 0; i < client->numberOfFilesWanted; i++) {
        File file;
        fscanf(inputFile, "%s", file.filename);
        file.numberOfChunks = 0;
        client->filesWanted[i] = file;
    }

    fclose(inputFile);
}

void print(Client *client){
    printf("Client %d\n", client->rank);
    printf("Files owned: %ld\n", client->numberOfFilesOwned);
    for (int i = 0; i < client->numberOfFilesOwned; i++) {
        printf("File %d: %s\n", i, client->filesOwned[i].filename);
        printf("Chunks: ");
        for (int j = 0; j < client->filesOwned[i].numberOfChunks; j++) {
            printf("%s %ld\n", client->filesOwned[i].chunks[j], strlen(client->filesOwned[i].chunks[j]));
        }
        printf("\n");
    }

    printf("Files wanted: %ld\n", client->numberOfFilesWanted);
    for (int i = 0; i < client->numberOfFilesWanted; i++) {
        printf("File %d: %s\n", i, client->filesWanted[i].filename);
        printf("\n");
    }
}

/*
    Functie care primeste mesajul de ACK de la tracker.
*/
void receiveACKFromTracker(int rank) {
    char ack[4];
    MPI_Status status;
    MPI_Recv(ack, ackSize, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);
    printf("Received ACK from tracker\n");
}

void tracker(int numtasks, int rank) {
    MPI_Status status;
    Client clients[numtasks - 1];
    Swarm swarms[MAX_FILES];
    int *numberOfSwarms = malloc(sizeof(int));
    *numberOfSwarms = 0;
    receiveFilesOwnedFromSeeds(clients, swarms, numberOfSwarms,numtasks, status);
    printSwarms(swarms, *numberOfSwarms);
    sendACKToClients(numtasks);
    free(numberOfSwarms);
}

void peer(int numtasks, int rank) {
    char *inputFileName = malloc(100 * sizeof(char));
    char rankStr[10];
    sprintf(rankStr, "%d", rank);
    strcpy(inputFileName, "./test1/in");
    strcat(inputFileName, rankStr);
    strcat(inputFileName, ".txt");

    Client client;
    readInputFile(inputFileName, &client);
    sendFileOwnedToTracker(&client);
    receiveACKFromTracker(rank);

    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
    free(inputFileName);
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
