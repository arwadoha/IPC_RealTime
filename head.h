/*
 * local header file for Supermarket cashier simulation project
 */

#ifndef __LOCAL_H_
#define __LOCAL_H_
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>
#include <float.h>
#include <sys/sem.h>
#include <ctype.h>
#include <sys/shm.h>
#include <unistd.h>  // Include for sleep function
#include <limits.h>  // Include for INT_MAX
#include <float.h>   // Include for DBL_MAX


int shmId;  // Global variable for shared memory ID

// Global variable to keep track of the number of customers who have paid and left
volatile sig_atomic_t customersPaidAndLeft = 0;
volatile sig_atomic_t customerPaidAndLeftFlag = 0;


// DS For (which customer is assigned to which cashier).
typedef struct {
    int customerId;
    int cashierId;
    int numItems;
    time_t maxWaitTime;
} CustomerAssignment;

// Message Queue
struct Message {
    long mtype;
    int customerId;
    int eventType; // Define event types as needed (e.g., customer left due to impatience)
    time_t timestamp;
    int itemIndex;
    int quantityRequested;
    int quantityRemaining;
    double behaviorScore;
    double cashierScore;
};

typedef struct {
    char name[50];
    double price;
    int quantity;
} Item;

// Configuration file data 
typedef struct {
    double minArrivalRate;
    double maxArrivalRate;
    int minShoppingTime;
    int maxShoppingTime;
    double cashierBehaviorDecayRate;
    int maxCashierItemsPerScan;
    double minCashierScanTime;
    double maxCashierScanTime;
} Configuration;

//cashier struct 
typedef struct {
    int cashierId;
    int queueSize;
    double scanRate; // Items per second
    double behaviorScore; // Represents kindness, smile, etc.
    int behaviorDecay; // Rate at which behavior score decreases
    int isActive; // Flag to mark if the cashier is active or leaving
    double income;
} Cashier;

/* Union used for setting values in the `semctl` function.
  Provides different member types for different operations on semaphores.
*/
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};


#endif
