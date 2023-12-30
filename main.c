/* Supermarket cashier simulation 
 *  15/12/2023 
 * Raghad Afaghani - 1192423   //   Yousra sheikh qasem -1192131 // Arwa Doha 1190324
 */
#include "head.h"
#include "constants.h"


CustomerAssignment assignments[NUM_CUSTOMERS];
Cashier *sharedCashiers;

// Function prototypes
void parseConfigFile(const char *filename, Configuration *config, Item **items, int *itemCount);
void sendMessageToQueue(int msgQueueId, long messageType, struct Message *msg);
void receiveMessageFromQueue(int msgQueueId, long messageType, struct Message *msg);
int chooseCashier(Cashier *cashiers, CustomerAssignment *assignments, int customerId, int customerItems);
void customerProcess(int customerId, Configuration *config, Item *items, int itemCount, int msgQueueId, int sem_id, CustomerAssignment *assignments);
void handleInventoryRequests(int msgQueueId, Item *items, int itemCount);
void handleCashierDeparture(Cashier *cashiers, int *numCashiers, int msgQueueId);
void removeCashier(Cashier *cashiers, int index, int *numCashiers);
void updateBehaviorScore(Cashier *cashiers, int numCashiers, double decayRate);
void simulateCashiersBehavior(Cashier *cashiers, int numCashiers, double decayRate, CustomerAssignment *assignments, int msgQueueId);
void handleCustomerExit(int signum);
void cashierProcess(int cashierId, int (*customerToCashierPipes)[2], int (*cashierToMainPipes)[2], int msgQueueId);
void decayBehavior(Cashier *cashiers, int numCashiers);
int checkSimulationTermination(Cashier *cashiers, int numCashiers, int impatientCustomerThreshold, double cashierIncomeThreshold, int cashierBehaviorThreshold);
void initializeCashiers(Cashier *cashiers, int numCashiers);
void updateCashierBehavior(Cashier *cashiers, int numCashiers);
void behaviorUpdateProcess();
void reassignCustomers(int cashierId);

//...........................................................................................................................

//Parse a configuration file and populate the configuration and item data structures.
void parseConfigFile(const char *filename, Configuration *config, Item **items, int *itemCount) {
    FILE *file = fopen(filename, "r"); // Open the file in read mode
    if (!file) { // Check if file opening was successful
        perror("Error opening config file");
        exit(1); // Exit the program if file opening fails
    }

    // Initialize itemCount to zero
    *itemCount = 0;

    // Reading configuration values from the file
    fscanf(file, "minArrivalRate: %lf\n", &config->minArrivalRate); // Read minimum arrival rate
    fscanf(file, "maxArrivalRate: %lf\n", &config->maxArrivalRate); // Read maximum arrival rate
    fscanf(file, "minShoppingTime: %d\n", &config->minShoppingTime); // Read minimum shopping time
    fscanf(file, "maxShoppingTime: %d\n", &config->maxShoppingTime); // Read maximum shopping time
    fscanf(file, "cashierBehaviorDecayRate: %lf\n", &config->cashierBehaviorDecayRate); // Read cashier behavior decay rate
    fscanf(file, "maxCashierItemsPerScan: %d\n", &config->maxCashierItemsPerScan); // Read maximum items per scan for cashier
    fscanf(file, "minCashierScanTime: %lf\n", &config->minCashierScanTime); // Read minimum scan time for cashier
    fscanf(file, "maxCashierScanTime: %lf\n", &config->maxCashierScanTime); // Read maximum scan time for cashier

    // Skip to the items section in the file
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Items:", 6) == 0) {
            break; // Stop when "Items:" section is found
        }
    }

    // Allocate memory for items assuming a maximum of 10 items
    *items = malloc(sizeof(Item) * 10);

    char itemName[50];
    double price;
    int quantity;

    // Read items from the file
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, " \t"); // Tokenize by space and tab
        if (token) {
            strcpy(itemName, token);
            token = strtok(NULL, " \t");
            if (token) price = atof(token); // Convert token to double
            token = strtok(NULL, " \t");
            if (token) quantity = atoi(token); // Convert token to integer

            // Trim trailing whitespace from itemName, if needed
            int len = strlen(itemName);
            while (len > 0 && isspace(itemName[len - 1])) {
                itemName[--len] = '\0';
            }

            strcpy((*items)[*itemCount].name, itemName); // Set item name
            (*items)[*itemCount].price = price; // Set item price
            (*items)[*itemCount].quantity = quantity; // Set item quantity
            (*itemCount)++; // Increment itemCount for each item
            if (*itemCount >= 10) {
                break; // Stop reading if maximum item count (10) is reached
            }
        }
    }
    fclose(file); // Close the file after reading
}

// Function to send a message to the message queue
void sendMessageToQueue(int msgQueueId, long messageType, struct Message *msg) {
    if (msgsnd(msgQueueId, msg, sizeof(struct Message) - sizeof(long), 0) == -1) {
        perror("Error sending message to queue");
        exit(1);
    }
}

// Function to receive a message from the message queue
void receiveMessageFromQueue(int msgQueueId, long messageType, struct Message *msg) {
    if (msgrcv(msgQueueId, msg, sizeof(struct Message) - sizeof(long), messageType, 0) == -1) {
        if (errno != ENOMSG) {
            perror("Error receiving message from queue");
            exit(1);
        }
    }
}

//Reassigns customers from a given cashier to other available cashier
void reassignCustomers(int cashierId) {
    // Attach to shared memory if not already attached
    if (sharedCashiers == NULL) {
        sharedCashiers = (Cashier *)shmat(shmId, NULL, 0);
        if (sharedCashiers == (Cashier *)-1) {
            perror("shmat");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        // Check if the current customer is assigned to the given cashier
        if (assignments[i].cashierId == cashierId) {
            // Reassign this customer to another cashier
            int newCashierId = chooseCashier(sharedCashiers, assignments, assignments[i].customerId, assignments[i].numItems);
            // Check if a valid new cashier is found
            if (newCashierId != -1) {
                // Update the customer's assignment to the new cashier
                assignments[i].cashierId = newCashierId;
                printf("Customer %d reassigned from Cashier %d to Cashier %d\n", assignments[i].customerId, cashierId, newCashierId);
            } else {
                printf("No available cashier for Customer %d\n", assignments[i].customerId);
            }
        }
    }
    // Detach from shared memory if attached within this function
    if (shmdt(sharedCashiers) == -1) {
        perror("shmdt");
        exit(1);
    }
    //it's no longer attached to shared memory
    sharedCashiers = NULL;
}

// Updates the behavior scores of the cashiers
void updateCashierBehavior(Cashier *cashiers, int numCashiers) {
    for (int i = 0; i < numCashiers; i++) {  // Loop through all cashiers
        if (cashiers[i].isActive) { // Check if cashier is active
            cashiers[i].behaviorScore -= cashiers[i].behaviorDecay;  // Decrease behavior score by decay
            printf("Cashier %d: Updated Behavior Score = %.2f\n", i, cashiers[i].behaviorScore);
            // Check if the behavior score has fallen below or equal to zero
            if (cashiers[i].behaviorScore <= 0) {
                cashiers[i].isActive = 0;   // Deactivate cashier --> inactive
                printf("Cashier %d is now inactive due to low behavior score.\n", i);
                // Call the [reassignCustomers] function to reassign this cashier's customers to other cashiers
                reassignCustomers(i); // Reassign customers of inactive cashier
            }
        }
    }
}

//Process for updating the behavior scores of cashiers at regular intervals
void behaviorUpdateProcess() {
    // Attach to shared memory segment containing cashier data
    sharedCashiers = (Cashier *)shmat(shmId, NULL, 0);
    if (sharedCashiers == (Cashier *)-1) {
        perror("shmat in behaviorUpdateProcess: Failed to attach to shared memory.");
        exit(1);
    }

    // Run update loop indefinitely
    while (1) {
        sleep(BEHAVIOR_UPDATE_INTERVAL); // Sleep for the specified update interval between behavior updates
        printf("Updating behavior scores...\n"); // Added for debugging
        updateCashierBehavior(sharedCashiers, NUM_CASHIERS); // Update the behavior of all active cashiers
    }

    // Detach from the shared memory segment
    if (shmdt(sharedCashiers) == -1) {
        perror("shmdt in behaviorUpdateProcess: Failed to detach from shared memory.");
        exit(1);
    }
}

//Initializes the cashiers with default values and random characteristics.
void initializeCashiers(Cashier *cashiers, int numCashiers) {
    // Seed the random number generator with the current time for randomness
    srand(time(NULL));
    for (int i = 0; i < numCashiers; i++) {
        // Set basic cashier information
        cashiers[i].cashierId = i;  // Set the cashier's ID to the current value of 'i'
        cashiers[i].queueSize = 0; // Initialize queue size to 0
        cashiers[i].isActive = 1; // Initially, all cashiers are active

        // Set random within a specific range
        cashiers[i].scanRate = 1.5 + ((double)rand() / RAND_MAX) * 0.5; // Random scan rate between 1.5 and 2.0
        cashiers[i].behaviorScore = 8.0 + ((double)rand() / RAND_MAX) * 2.0; // Random behavior score between 8.0 and 10.0
      
        cashiers[i].behaviorDecay = 0.1; // Set a fixed example decay rate for behavior score
        cashiers[i].income = 0.0;  // Start with zero income for each cashier
        printf("Cashier %d initialized: Scan Rate = %.2f, Behavior Score = %.2f\n", i, cashiers[i].scanRate, cashiers[i].behaviorScore);
    }
}

//Choose the most suitable cashier for assigning a customer based on various factors.
int chooseCashier(Cashier *cashiers, CustomerAssignment *assignments, int customerId, int customerItems) {
    // 1. Attach to shared memory and handle potential errors
    Cashier *sharedCashiers = (Cashier *)shmat(shmId, NULL, 0);
    if (sharedCashiers == (Cashier *)-1) {
        perror("shmat");
        // Handle the error appropriately
        return -1;
    }
    // 2. Calculate total queue length for each cashier
    int totalQueueItems[NUM_CASHIERS] = {0};
    for (int j = 0; j < NUM_CUSTOMERS; j++) {
        if (assignments[j].customerId != -1 && assignments[j].cashierId >= 0 && assignments[j].cashierId < NUM_CASHIERS) {
            // Add this customer's items to the assigned cashier's queue
            totalQueueItems[assignments[j].cashierId] += assignments[j].numItems;
        }
    }
     // 3. Calculate and track scores for all active cashiers
    double scores[NUM_CASHIERS];
    int activeCashiers = 0;
    for (int i = 0; i < NUM_CASHIERS; i++) {
        if (sharedCashiers[i].isActive) {
            activeCashiers++;
             // Calculate adjusted queue size for this cashier with the new customer
            int adjustedQueueSize = sharedCashiers[i].queueSize + customerItems;
             // Calculate score based on:
             //1- Scan rate (higher is better) 2- Behavior score (higher is better) 3- Estimated queue size (lower is better)
            double score = (1.0 / sharedCashiers[i].scanRate) * 50.0 + (10.0 - sharedCashiers[i].behaviorScore) * 10.0 + adjustedQueueSize * 10.0;
            scores[i] = score;
            // Print debug information for each active cashier
            printf("Cashier %d: Score = %.3f, Queue Size = %d, Scan Rate = %.2f, Behavior Score = %.2f\n",
                   i, score, adjustedQueueSize, sharedCashiers[i].scanRate, sharedCashiers[i].behaviorScore);
        } else {
            // Set score to a high value to prioritize active cashiers
            scores[i] = INT_MAX;
        }
    }
        // 4. Check if any cashiers are available and handle accordingly
    if (activeCashiers == 0) { 
        printf("No active cashiers available.\n");
        shmdt(sharedCashiers); // Detach from shared memory
        return -1;
    }
    // 5. Find the cashier with the lowest score (highest priority)
    int chosenCashierId = -1;
    double bestScore = DBL_MAX;
    for (int i = 0; i < NUM_CASHIERS; i++) {
        if (scores[i] < bestScore) {
            bestScore = scores[i];
            chosenCashierId = i;
        }
    }
    // 6. Assign customer to chosen cashier and update information
    if (chosenCashierId != -1) {
        printf("Customer %d chose Cashier %d\n", customerId, chosenCashierId);
        assignments[customerId].customerId = customerId;
        assignments[customerId].cashierId = chosenCashierId;
        assignments[customerId].numItems = customerItems;

        // Update cashier's queue size
        sharedCashiers[chosenCashierId].queueSize += customerItems;
    } else {// If no suitable cashier found, wait and retry
        printf("Customer %d could not find a suitable cashier, trying again...\n", customerId);
        sleep(WAIT_TIME);
        chosenCashierId = chooseCashier(cashiers, assignments, customerId, customerItems);
    }
    // 7. Detach from shared memory and handle potential errors
    if (shmdt(sharedCashiers) == -1) {
        perror("shmdt"); // Handle the error appropriately
    }
    return chosenCashierId;
}

// indicating a customer has finished paying and left the store.
void customerPaidAndLeftSignalHandler(int sig) { // `SIGUSR1` sent by the main process
    customersPaidAndLeft++;  // Increment the total number of customers who have completed and left.
    // Print informative message confirming receiving the signal and the updated number of completed transactions.
    printf("Received SIGUSR1. Customer has paid and left. Total customers paid and left: %d\n", customersPaidAndLeft);
    customerPaidAndLeftFlag = 1; // Set a flag to true, indicating a customer has finished processing
}

// In main process or a dedicated inventory manager process
void handleInventoryRequests(int msgQueueId, Item *items, int itemCount){
    struct Message msg;
    while (1) {
        // Receive inventory request message from the message queue
        receiveMessageFromQueue(msgQueueId, INVENTORY_REQUEST_TYPE, &msg);

        // Check if the itemIndex is within valid bounds
        if (msg.itemIndex >= 0 && msg.itemIndex < itemCount)
        {
            // Check if item is in stock (quantity > 0)
            if (items[msg.itemIndex].quantity > 0)
            {
                if (items[msg.itemIndex].quantity >= msg.quantityRequested) // Enough stock available
                    {
                    items[msg.itemIndex].quantity -= msg.quantityRequested;// Update remaining quantity
                    msg.quantityRemaining = items[msg.itemIndex].quantity; // Send remaining after taking requested amount
                    }
                else // Not enough stock to fulfill the request
                   {
                    msg.quantityRemaining = items[msg.itemIndex].quantity; // Send remaining quantity (whatever's available)
                    items[msg.itemIndex].quantity = 0;     // Take all available items (empty the shelf)

                   }
            }
            else
                 { // No stock available
                msg.quantityRemaining = -1; // Indicate out of stock
                 }
        }
        else
        {  // Invalid item index
            msg.quantityRemaining = -1; // Invalid item index, indicate out of stock
        }

        // Send response back with updated quantity remaining
        msg.mtype = INVENTORY_RESPONSE_TYPE; // Change message type to response
        sendMessageToQueue(msgQueueId, INVENTORY_RESPONSE_TYPE, &msg); // Send response to queue
    }
}

//Simulate the shopping process for a customer, including item selection, checkout, and payment.
void customerProcess(int customerId, Configuration *config, Item *items, int itemCount, int msgQueueId, int sem_id, CustomerAssignment *assignments) {
    // Simulate the arrival of the customer
    double arrivalTime = config->minArrivalRate + (rand() / (double) RAND_MAX) * (config->maxArrivalRate - config->minArrivalRate);
    printf("Customer %d started shopping (Arrival Time: %.2f seconds)\n", customerId, arrivalTime);
    sleep(arrivalTime); // Simulate time taken to arrive at the store

    double totalBill = 0.0;
    int consecutiveFailures = 0;
    int totalCustomerItems = 0;
    time_t startTime = time(NULL);

    while (totalCustomerItems < MAX_ITEMS && difftime(time(NULL), startTime) < config->maxShoppingTime && consecutiveFailures < MAX_CONSECUTIVE_FAILURES) {
        // Select a random product from the available products in the supermarket
        int itemIndex = rand() % itemCount;

        // Check if the item is available in stock
        if (items[itemIndex].quantity <= 0) {
            printf("Item %s is out of stock.\n", items[itemIndex].name);
            consecutiveFailures++;
            continue;
        }

        // Calculate the quantity that the customer wants to buy
        int quantityRequested = rand() % (items[itemIndex].quantity) + 1;

        // Prepare and send an inventory request message
        struct Message reqMsg, respMsg;
        reqMsg.mtype = INVENTORY_REQUEST_TYPE;
        reqMsg.customerId = customerId;
        reqMsg.itemIndex = itemIndex;
        reqMsg.quantityRequested = quantityRequested;

        sendMessageToQueue(msgQueueId, INVENTORY_REQUEST_TYPE, &reqMsg);

        // Wait for a response from the inventory manager
        receiveMessageFromQueue(msgQueueId, INVENTORY_RESPONSE_TYPE, &respMsg);

        if (respMsg.quantityRemaining >= 0) {     // Item is available in stock
            // Calculate the cost of the items and update cart information
            totalBill += items[itemIndex].price * quantityRequested;
            totalCustomerItems += quantityRequested;
            printf("Customer %d added %d units of item %s to cart. Remaining stock: %d\n", customerId, quantityRequested, items[itemIndex].name, respMsg.quantityRemaining);
            consecutiveFailures = 0;
        } else {
            // Item is out of stock
            printf("Item %s is out of stock.\n", items[itemIndex].name);
            consecutiveFailures++;
        }

        sleep(1); // Simulate shopping time for each item
    }

    // Use the cashier selection semaphore to prevent multiple customers from choosing the same cashier
    struct sembuf acquire = {0, -1, SEM_UNDO};
    if (semop(sem_id, &acquire, 1) == -1) {
        perror("semop -- acquire");
        exit(EXIT_FAILURE);
    }

    // Attach to shared memory before accessing sharedCashiers
    Cashier *sharedCashiers = (Cashier *)shmat(shmId, NULL, 0);
    if (sharedCashiers == (Cashier *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Choose a cashier for the customer based on the assigned index
    int chosenCashierId = chooseCashier(sharedCashiers, assignments, customerId, totalCustomerItems);
    printf("Customer %d finished shopping and chose Cashier %d (Total Bill: %.2f)\n", customerId, chosenCashierId, totalBill);

    // Update cashier's queue size in shared memory
    sharedCashiers[chosenCashierId].queueSize += totalCustomerItems;

    // Detach from shared memory
    if (shmdt(sharedCashiers) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Release the semaphore to allow other customers to choose their cashiers
    struct sembuf release = {0, 1, SEM_UNDO};
    if (semop(sem_id, &release, 1) == -1) {
        perror("semop -- release");
        exit(EXIT_FAILURE);
    }

    // Send a message to the chosen cashier indicating customer's arrival
    struct Message msg;
    msg.mtype = CUSTOMER_TO_CASHIER_TYPE;
    msg.customerId = customerId;
    msg.eventType = 0; // Use 0 to indicate customer arrival
    msg.timestamp = time(NULL);
    sendMessageToQueue(msgQueueId, CUSTOMER_TO_CASHIER_TYPE, &msg);

    // Set up the signal handler for SIGUSR1
    if (signal(SIGUSR1, customerPaidAndLeftSignalHandler) == SIG_ERR) {
        perror("Signal handler setup failed");
        exit(EXIT_FAILURE);
    }

    // Notify the main process that this customer is ready to pay
    kill(getppid(), SIGUSR1);

    // Loop and wait for a signal indicating payment is complete
    while (!customerPaidAndLeftFlag) {
        pause(); // Wait for a signal
    }
}

//This function manages the cashier process in a supermarket simulation.
void cashierProcess(int cashierId, int (*customerToCashierPipes)[2], int (*cashierToMainPipes)[2], int msgQueueId) {
    // 1. Connect to shared memory containing cashier data
    key_t shmKey = ftok(".", 'a'); // Unique key based on file and identifier
    if (shmKey == -1) {
        perror("ftok");
        exit(EXIT_FAILURE); // Handle error gracefully
    }
    
    int shmId = shmget(shmKey, sizeof(Cashier) * NUM_CASHIERS, 0666); // Allocate shared memory for cashiers
    if (shmId == -1) {
        perror("shmget");
        exit(EXIT_FAILURE); // Handle error gracefully
    }

    Cashier *sharedCashiers = (Cashier *)shmat(shmId, NULL, 0); // Attach shared memory to process address space
    if (sharedCashiers == (Cashier *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE); // Handle error gracefully
    }

    // 2. Message queue communication loop
    struct Message msg;
    char readBuffer[256];
    while (1) {
        // Receive behavior update message from the main process
        receiveMessageFromQueue(msgQueueId, BEHAVIOR_UPDATE_TYPE, &msg);
        
        // 3. Read messages from all customers (non-blocking)
        for (int i = 0; i < NUM_CUSTOMERS; i++) {
            ssize_t bytesRead = read(customerToCashierPipes[i][0], readBuffer, sizeof(readBuffer));
            if (bytesRead > 0) {
                printf("Cashier %d received message: %s\n", cashierId, readBuffer);
            }
        }
        // 4. Prepare and send behavior score message to main process
        msg.mtype = BEHAVIOR_SCORE_TYPE;
        // Update message content with relevant cashier information (details not shown)
        sendMessageToQueue(msgQueueId, BEHAVIOR_SCORE_TYPE, &msg);
    }
    // 5. Cleanup and exit
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        close(customerToCashierPipes[i][0]); // Close customer-to-cashier read ends
    }
    close(cashierToMainPipes[cashierId][1]); // Close cashier-to-main write end
}

//Generate a random arrival time based on the configuration settings.
double getRandomArrivalTime(Configuration *config) {
  // Calculate and return the random arrival time based on the range and minimum arrival rate
    return config->minArrivalRate +
           ((double) rand() / RAND_MAX) * (config->maxArrivalRate - config->minArrivalRate);
}

//Decay the behavior scores of active cashiers based on their individual decay rates.
void decayBehavior(Cashier *cashiers, int numCashiers) {
    for (int i = 0; i < numCashiers; i++) { // Iterate through all active cashiers
        if (cashiers[i].isActive) {
            // Decrease the cashier's behavior score based on their individual decay rate
            cashiers[i].behaviorScore -= cashiers[i].behaviorDecay;
             //Mark cashiers as inactive if their behavior score reaches zero.
            if (cashiers[i].behaviorScore <= 0) { // Check if the score has reached zero
                printf("Cashier %d's behavior dropped to 0. Leaving the simulation.\n", cashiers[i].cashierId);
                cashiers[i].isActive = 0; // Mark the cashier as inactive
            }
        }
    }
}

// Function to check simulation termination conditions
int checkSimulationTermination(Cashier *cashiers, int numCashiers, int impatientCustomerThreshold, double cashierIncomeThreshold, int cashierBehaviorThreshold) {
    // Check if any termination condition is met
    int numCashiersLeft = NUM_CASHIERS;
    int numImpatientCustomers = 0;
    double totalIncome = 0.0;
    //int cashierIncomeThreshold = 20; // Set your defined threshold for cashier income
    //int cashierBehaviorThreshold = 0; // Set your defined threshold for cashier behavior
    int cashierIncomeThresholdValue = 20; // Set your defined threshold for cashier income
    int cashierBehaviorThresholdValue = 0;
    // 2 Check if any cashiers have behavior dropped to 0 and left
    int numCashiersWithZeroBehavior = 0;
    for (int i = 0; i < numCashiers; i++) {
        if (cashiers[i].behaviorScore <= 0.0) {
            numCashiersWithZeroBehavior++;
        }
    }
    if (numCashiersWithZeroBehavior >= cashierBehaviorThreshold) {
        printf("Number of cashiers with behavior dropped to 0 exceeded the threshold. Exiting simulation.\n");
        return 1; // Terminate simulation
    }
    if (numCashiersWithZeroBehavior >= cashierBehaviorThreshold) {
        printf("Number of cashiers with behavior dropped to 0 exceeded the threshold. Exiting simulation.\n");
        return 1; // Exit the simulation
    }

    // Check if the number of impatient customers exceeds the threshold
    if (numImpatientCustomers >= IMPATIENT_CUSTOMER_THRESHOLD) {
        printf("Number of impatient customers exceeded the threshold. Exiting simulation.\n");
        return 1; // Exit the simulation
    }

    // Check if any cashier made income more than the threshold
    for (int i = 0; i < NUM_CASHIERS; i++) {
        // Assuming each cashier's income contributes equally to the total income
        totalIncome += cashiers[i].income;
        if (totalIncome >= cashierIncomeThreshold) {
            printf("Income threshold exceeded. Exiting simulation.\n");
            return 1; // Exit the simulation
        }
    }
    return 0; // Continue simulation
}


//******** main method *************
int main() {
    srand(time(NULL));
    CustomerAssignment assignments[NUM_CUSTOMERS] = {{-1, -1, 0}};
    Configuration config;
    Item *items = NULL;
    int itemCount = 0;

    // Fork a process for behavior updates
    pid_t behaviorUpdatePid = fork();
    if (behaviorUpdatePid == 0) {
        behaviorUpdateProcess();
        exit(0);
    }

    // Semaphore initialization
    int sem_id;
    union semun arg;

    key_t sem_key = ftok("supermarket", 'B');
    sem_id = semget(sem_key, 1, IPC_CREAT | 0660);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    // read & print item information
    parseConfigFile("config.txt", &config, &items, &itemCount);
    printf("Items loaded from config:\n");
    for (int i = 0; i < itemCount; i++) {
        printf("Item %d: Name: %s, Price: %.2f, Quantity: %d\n", i, items[i].name, items[i].price, items[i].quantity);
    }
    // Set up signal handler for customer completion
    struct sigaction sa;
    sa.sa_handler = &customerPaidAndLeftSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
    // Create message queue and shared memory
    key_t msgQueueKey = ftok(".", 'a');
    int msgQueueId = msgget(msgQueueKey, IPC_CREAT | 0666);
    if (msgQueueId == -1) {
        perror("Error creating message queue");
        exit(1);
    }

// Creating shared memory
    key_t shmKey = ftok(".", 'a');
    if (shmKey == -1) {
        perror("ftok error");
        exit(EXIT_FAILURE);
    }
    shmId = shmget(shmKey, sizeof(Cashier) * NUM_CASHIERS, IPC_CREAT | 0666);
    if (shmId == -1) {
        perror("shmget error");
        exit(EXIT_FAILURE);
    }

// Attaching shared memory
    Cashier *sharedCashiers = (Cashier *)shmat(shmId, NULL, 0);
    if (sharedCashiers == (Cashier *)-1) {
        perror("shmat error");
        exit(EXIT_FAILURE);
    }

    // Initialize cashiers in shared memory
    initializeCashiers(sharedCashiers, NUM_CASHIERS);

    // Detach from shared memory after initialization
    if (shmdt(sharedCashiers) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Start inventory manager process by fork
    pid_t inventoryManagerPid = fork();
    if (inventoryManagerPid == 0) {
        handleInventoryRequests(msgQueueId, items, itemCount);
        exit(0);
    }

    // Fork customer processes
    for (int customerId = 1; customerId <= NUM_CUSTOMERS; customerId++) {
        double arrivalTime = getRandomArrivalTime(&config);
        sleep(arrivalTime);
        pid_t pid = fork();
        if (pid == 0) {
            customerProcess(customerId, &config, items, itemCount, msgQueueId, sem_id, assignments);
            exit(0);
        } else if (pid < 0) {
            perror("fork failed");
            exit(1);
        }
    }
    // Create pipes for customer-cashier and cashier-main communication
    int customerToCashierPipes[NUM_CUSTOMERS][2];
    int cashierToMainPipes[NUM_CASHIERS][2];

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if (pipe(customerToCashierPipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_CASHIERS; i++) {
        if (pipe(cashierToMainPipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t cashierPid = fork();

        if (cashierPid == 0) {
            cashierProcess(i, customerToCashierPipes, cashierToMainPipes, msgQueueId);
            exit(0);
        } else if (cashierPid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all child processes to finish
    int status;
    pid_t childPid;

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        close(customerToCashierPipes[i][0]);
    }

    for (int i = 0; i < NUM_CASHIERS; i++) {
        close(cashierToMainPipes[i][1]);
    }

    kill(inventoryManagerPid, SIGINT);
    waitpid(inventoryManagerPid, &status, 0);
    // Wait for all child processes to complete
    while ((childPid = waitpid(-1, &status, 0)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child process %d terminated with status %d\n", childPid, WEXITSTATUS(status));
        } else {
            printf("Child process %d terminated abnormally\n", childPid);
        }
    }

    kill(behaviorUpdatePid, SIGINT);
    waitpid(behaviorUpdatePid, &status, 0);

    // Cleanup
    free(items);
    if (msgctl(msgQueueId, IPC_RMID, NULL) == -1) {
        perror("Error removing message queue");
    }

    // Semaphore cleanup
    if (semctl(sem_id, 0, IPC_RMID, 0) == -1) {
        perror("semctl -- remove semaphore");
        exit(EXIT_FAILURE);
    }

    // Shared Memory cleanup
    if (shmdt(sharedCashiers) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    if (shmctl(shmId, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    printf("Supermarket simulation completed.\n");
    return 0;

}
