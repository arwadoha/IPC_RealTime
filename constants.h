/*
 * local all constants file   for Supermarket cashier simulation project
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Constants for wait time and behavior update interval
#define WAIT_TIME 5  // Define wait time in seconds
#define BEHAVIOR_UPDATE_INTERVAL 5
// Constants for maximum items, number of customers, number of cashiers, maximum wait time,
// event impatience, impatient customer threshold, and message types
#define MAX_ITEMS 5
#define NUM_CUSTOMERS 10
#define NUM_CASHIERS 3
#define MAX_WAIT_TIME 1
#define EVENT_IMPATIENCE 1
#define IMPATIENT_CUSTOMER_THRESHOLD 5
#define CASHIER_TO_MAIN_TYPE 1
#define CUSTOMER_TO_CASHIER_TYPE 2
#define IMPATIENT_CUSTOMER_TYPE 3
#define INVENTORY_REQUEST_TYPE 4
#define INVENTORY_RESPONSE_TYPE 5
#define BEHAVIOR_SCORE_TYPE 6
#define BEHAVIOR_UPDATE_TYPE 7

// Constant for behavior score
#define BEHAVVIORSCORE 10
// Constant for maximum number of consecutive failures
#define MAX_CONSECUTIVE_FAILURES 3
#endif /* CONSTANTS_H */