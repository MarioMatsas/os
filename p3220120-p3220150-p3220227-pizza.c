#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "p3220120-p3220150-p3220227-pizza.h"

pthread_mutex_t print_lock;
pthread_mutex_t revenue_lock;
pthread_mutex_t s_lock;
pthread_mutex_t tel_lock;
pthread_cond_t tel_cond;
pthread_mutex_t cook_lock;
pthread_cond_t cook_cond;
pthread_mutex_t oven_lock;
pthread_cond_t oven_cond;
pthread_mutex_t dispatch_lock;
pthread_cond_t dispatch_cond;
pthread_mutex_t total_pizzas_lock;
pthread_mutex_t success_lock;
pthread_mutex_t order_stats_lock;
pthread_mutex_t delivery_stats_lock;

unsigned int seed;
int revenue;
int total_margaritas_sold;
int total_pepperonis_sold;
int total_specials_sold;
int successful;
int unsuccessful;

// For orders
int max_order = 0;
float average_order = 0.0;
int order_waiting_total = 0;
// For deliveries
int max_cold = 0;
float average_cold = 0.0;
int cold_waiting_total = 0;

int s = 0;

int N_tels = Ntel;
int N_cooks = Ncook;
int N_ovens = Noven;
int N_deliverers = Ndeliverer;

void *order(void *x) {
    int id = *(int *)x;
    int wait_time;
    int rc;

    // Order timers
    struct timespec order_start, order_end;
    // Delivery timers
    struct timespec delivery_start, delivery_end;
    // Order timers for max/average stats
    struct timespec overall_start, overall_end;
    // Delivery timers for max/average stats
    struct timespec cold_start, cold_end;

    rc = clock_gettime(CLOCK_REALTIME, &overall_start); // ----------------- OVERALL START
    if (rc != 0) { exit(-1); }


    // The first customer (thread) doesn't wait, however every other one does
    rc = pthread_mutex_lock(&s_lock);
    if (rc != 0) { exit(-1); }
    if (s == 0) {
        s += 1;
        //rc = pthread_mutex_lock(&print_lock);
        //printf("Id %d skipped!\n", id);
        //rc = pthread_mutex_unlock(&print_lock);
    } else {
        wait_time = Torderlow + rand_r(&seed) % (Torderhigh - Torderlow + 1);
        //pthread_mutex_lock(&print_lock);	
        //printf("Id %d waited.....%d\n", id, wait_time);
        //pthread_mutex_unlock(&print_lock);
        sleep(wait_time);
    }
    rc = pthread_mutex_unlock(&s_lock);
    if (rc != 0) { exit(-1); }

    // Get on the phone with the pizzaria
    rc = pthread_mutex_lock(&tel_lock);
    if (rc != 0) { exit(-1); }
    while (N_tels == 0) {
		//printf("Id %d waiting...\n", id);
        rc = pthread_cond_wait(&tel_cond, &tel_lock);
        if (rc != 0) { exit(-1); }
    }
    N_tels--;
    rc = pthread_mutex_unlock(&tel_lock);
    if (rc != 0) { exit(-1); }

    // Select a random number of pizzas to order
    int pizzas_ordered = Norderlow + rand_r(&seed) % (Norderhigh - Norderlow + 1);

    int margaritas = 0;
    int pepperoni = 0;
    int special = 0;
    int choice;

    for (int i = 0; i < pizzas_ordered; i++) {
        // Select a random number for the percentage and depending on that, order one 
        // of the 3 options
        choice = 1 + rand_r(&seed) % 100;
        if (choice <= Pm) {
            margaritas++;
        } else if (choice <= Pm + Pp) {
            pepperoni++;
        } else {
            special++;
        }
    }

    // Wait for payment
    wait_time = Tpaymentlow + rand_r(&seed) % (Tpaymenthigh - Tpaymentlow + 1);
    //pthread_mutex_lock(&print_lock);
    //printf("Id %d waited...2..%d\n", id, wait_time);
    //pthread_mutex_unlock(&print_lock);
    sleep(wait_time);

    // Payment may fail
    choice = 1 + rand_r(&seed) % 100;
    if (choice <= Pfail) {
        rc = pthread_mutex_lock(&success_lock);
        if (rc != 0) { exit(-1); }
        unsuccessful++;
        rc = pthread_mutex_unlock(&success_lock);
        if (rc != 0) { exit(-1); }
        rc = pthread_mutex_lock(&print_lock);
        if (rc != 0) { exit(-1); }
        printf("The transaction of %d has failed...\n", id);
        rc = pthread_mutex_unlock(&print_lock);
        if (rc != 0) { exit(-1); }
        
		rc = pthread_mutex_lock(&tel_lock);
		if (rc != 0) { exit(-1); }
		N_tels++;
		//printf("%d\n", N_tels);
		rc = pthread_cond_broadcast(&tel_cond);
		if (rc != 0) { exit(-1); }
		rc = pthread_mutex_unlock(&tel_lock);
		if (rc != 0) { exit(-1); }
		
        pthread_exit(NULL);
    } else {
        rc = pthread_mutex_lock(&success_lock);
        if (rc != 0) { exit(-1); }
        successful++;
        rc = pthread_mutex_unlock(&success_lock);
        if (rc != 0) { exit(-1); }
        rc = pthread_mutex_lock(&print_lock);
        if (rc != 0) { exit(-1); }
        printf("The transaction of %d was successful! Please wait for your pizzas...\n", id);
        rc = pthread_mutex_unlock(&print_lock);
        if (rc != 0) { exit(-1); }
    }

    // Add the money to the sum
    rc = pthread_mutex_lock(&revenue_lock);
    if (rc != 0) { exit(-1); }
    revenue += margaritas * Cm + pepperoni * Cp + special * Cs;
    rc = pthread_mutex_unlock(&revenue_lock);
    if (rc != 0) { exit(-1); }

    // Update the total pizzas sold per kind
    rc = pthread_mutex_lock(&total_pizzas_lock);
    if (rc != 0) { exit(-1); }
    total_margaritas_sold += margaritas;
    total_pepperonis_sold += pepperoni;
    total_specials_sold += special;
    rc = pthread_mutex_unlock(&total_pizzas_lock);
    if (rc != 0) { exit(-1); }

    // Your order will be taken care of by the cook, so you disconnect the call
    rc = pthread_mutex_lock(&tel_lock);
    if (rc != 0) { exit(-1); }
    N_tels++;
    rc = pthread_cond_broadcast(&tel_cond);
    if (rc != 0) { exit(-1); }
	rc = pthread_mutex_unlock(&tel_lock);
	if (rc != 0) { exit(-1); }
    

    rc = clock_gettime(CLOCK_REALTIME, &order_start); // ----------------- ORDER START
    if (rc != 0) { exit(-1); }

    // Check for available cooks
    rc = pthread_mutex_lock(&cook_lock);
    if (rc != 0) { exit(-1); }
    while (N_cooks == 0) {
        rc = pthread_cond_wait(&cook_cond, &cook_lock);
        if (rc != 0) { exit(-1); }
    }
    N_cooks--;
    rc = pthread_mutex_unlock(&cook_lock);
    if (rc != 0) { exit(-1); }

    // Prepare pizzas
    wait_time = pizzas_ordered * Tprep;
    sleep(wait_time);

    // Once you get a cook, look for available ovens
    rc = pthread_mutex_lock(&oven_lock);
    if (rc != 0) { exit(-1); }
    while (N_ovens < pizzas_ordered) {
        rc = pthread_cond_wait(&oven_cond, &oven_lock);
        if (rc != 0) { exit(-1); }
    }
    N_ovens -= pizzas_ordered;
    rc = pthread_mutex_unlock(&oven_lock);
    if (rc != 0) { exit(-1); }

    // Release cook
    rc = pthread_mutex_lock(&cook_lock);
    if (rc != 0) { exit(-1); }
    N_cooks++;
    rc = pthread_cond_signal(&cook_cond);
    if (rc != 0) { exit(-1); }
    rc = pthread_mutex_unlock(&cook_lock);
    if (rc != 0) { exit(-1); }

    // Bake pizzas
    wait_time = pizzas_ordered * Tbake;
    sleep(wait_time);

    rc = clock_gettime(CLOCK_REALTIME, &cold_start); // ----------------- COLD START
    if (rc != 0) { exit(-1); }

    // After the pizzas are done we look for a delivery man
    rc = pthread_mutex_lock(&dispatch_lock);
    if (rc != 0) { exit(-1); }
    while (N_deliverers == 0) {
        rc = pthread_cond_wait(&dispatch_cond, &dispatch_lock);
        if (rc != 0) { exit(-1); }
    }
    N_deliverers--;
    rc = pthread_mutex_unlock(&dispatch_lock);
    if (rc != 0) { exit(-1); }

    // Release ovens
    rc = pthread_mutex_lock(&oven_lock);
    if (rc != 0) { exit(-1); }
    N_ovens += pizzas_ordered;
    rc = pthread_cond_broadcast(&oven_cond);
    if (rc != 0) { exit(-1); }
    rc = pthread_mutex_unlock(&oven_lock);
    if (rc != 0) { exit(-1); }

    // Pack the order
    wait_time = pizzas_ordered * Tpack;
    sleep(wait_time);

    rc = clock_gettime(CLOCK_REALTIME, &order_end); // ----------------- ORDER END
    if (rc != 0) { exit(-1); }

    int order_time = order_end.tv_sec - order_start.tv_sec;
    rc = pthread_mutex_lock(&print_lock);
    if (rc != 0) { exit(-1); }
    printf("Order %d was completed in %d minutes. \n", id, order_time);
    rc = pthread_mutex_unlock(&print_lock);
    if (rc != 0) { exit(-1); }

    rc = clock_gettime(CLOCK_REALTIME, &delivery_start); // ----------------- DELIVERY START
    if (rc != 0) { exit(-1); }

    // Delivery time
    wait_time = Tdellow + rand_r(&seed) % (Tdelhigh - Tdellow + 1);
    sleep(wait_time);

    rc = clock_gettime(CLOCK_REALTIME, &delivery_end); // ----------------- DELIVERY END
    if (rc != 0) { exit(-1); }
    rc = clock_gettime(CLOCK_REALTIME, &overall_end); // ----------------- OVERALL END
    if (rc != 0) { exit(-1); }
    rc = clock_gettime(CLOCK_REALTIME, &cold_end); // ----------------- COLD END
    if (rc != 0) { exit(-1); }

    // Update order overall stats
    rc = pthread_mutex_lock(&order_stats_lock);
    if (rc != 0) { exit(-1); }
    int o_time = overall_end.tv_sec - overall_start.tv_sec;
    order_waiting_total += o_time;
    if (o_time > max_order) {
        max_order = o_time;
    }
    rc =pthread_mutex_unlock(&order_stats_lock);
    if (rc != 0) { exit(-1); }

    // Update cold stats
    rc = pthread_mutex_lock(&delivery_stats_lock);
    if (rc != 0) { exit(-1); }
    int c_time = cold_end.tv_sec - cold_start.tv_sec;
    cold_waiting_total += c_time;
    if (c_time > max_cold) {
        max_cold = c_time;
    }
    rc = pthread_mutex_unlock(&delivery_stats_lock);
    if (rc != 0) { exit(-1); }

    int delivery_time = delivery_end.tv_sec - delivery_start.tv_sec;
    rc = pthread_mutex_lock(&print_lock);
    if (rc != 0) { exit(-1); }
    printf("Order %d was delivered in %d minutes. \n", id, delivery_time);
    rc = pthread_mutex_unlock(&print_lock);
    if (rc != 0) { exit(-1); }

    // Go back to the store
    sleep(wait_time);

    // Release delivery man
    rc = pthread_mutex_lock(&dispatch_lock);
    if (rc != 0) { exit(-1); }
    N_deliverers++;
    rc = pthread_cond_signal(&dispatch_cond);
    if (rc != 0) { exit(-1); }
    rc = pthread_mutex_unlock(&dispatch_lock);
    if (rc != 0) { exit(-1); }

    // * Done *
    pthread_exit(NULL);
}
int main(int argc, char *argv[]){
	// Make sure we have the right number of arguments
	if (argc != 3){
		printf("Wrong number of arguments\n");
		return -1;
	}
    int Ncust = atoi(argv[1]);
    seed = atoi(argv[2]);
    int rc;

    int *id = malloc(Ncust * sizeof(int)); // Allocate memory 
    if (id == NULL){ return -1; }
    pthread_t *threads = malloc(Ncust * sizeof(pthread_t));
    if (threads == NULL) { return -1; }

    rc = pthread_mutex_init(&print_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&revenue_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&s_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&tel_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_cond_init(&tel_cond, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&cook_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_cond_init(&cook_cond, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&oven_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_cond_init(&oven_cond, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&dispatch_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_cond_init(&dispatch_cond, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&total_pizzas_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&success_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&order_stats_lock, NULL);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_init(&delivery_stats_lock, NULL);
    if (rc != 0) { return -1; }

    for (int i = 0; i < Ncust; i++) {
        id[i] = i + 1;
        rc = pthread_mutex_lock(&print_lock);
        if (rc != 0) { return -1; }
        printf("Main: Thread %d has been created\n", id[i]);
        rc = pthread_mutex_unlock(&print_lock);
        if (rc != 0) { return -1; }

        rc = pthread_create(&threads[i], NULL, order, &id[i]);
        if (rc != 0) { return -1; }
    }

    for (int i = 0; i < Ncust; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) { return -1; }

        rc = pthread_mutex_lock(&print_lock);
        if (rc != 0) { return -1; }
        printf("Main: Thread %d has been destroyed\n", id[i]);
        rc = pthread_mutex_unlock(&print_lock);
        if (rc != 0) { return -1; }
    }

    rc = pthread_mutex_destroy(&print_lock);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&revenue_lock);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&s_lock);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&tel_lock);
    if (rc != 0) { return -1; }
    rc = pthread_cond_destroy(&tel_cond);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&cook_lock);
    if (rc != 0) { return -1; }
    rc = pthread_cond_destroy(&cook_cond);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&oven_lock);
    if (rc != 0) { return -1; }
    rc = pthread_cond_destroy(&oven_cond);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&dispatch_lock);
    if (rc != 0) { return -1; }
    rc = pthread_cond_destroy(&dispatch_cond);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&total_pizzas_lock);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&success_lock);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&order_stats_lock);
    if (rc != 0) { return -1; }
    rc = pthread_mutex_destroy(&delivery_stats_lock);
    if (rc != 0) { return -1; }

    average_order = (float)order_waiting_total / successful;
    average_cold = (float)cold_waiting_total / successful;
    printf("---------\n");
    printf("Total revenue: %d Euros\n", revenue);
    printf("Margaritas: %d\n", total_margaritas_sold);
    printf("Pepperonis: %d\n", total_pepperonis_sold);
    printf("Specials: %d\n", total_specials_sold);
    printf("Successful orders: %d\n", successful);
    printf("Unsuccessful orders: %d\n", unsuccessful);
    printf("---------\n");
    printf("Average service time: %.2f minutes\n", average_order);
    printf("Longest service time: %d minutes\n", max_order);
    printf("---------\n");
    printf("Average colding time: %.2f minutes\n", average_cold);
    printf("Longest colding time: %d minutes\n", max_cold);

    free(id); // Free allocated memory
    free(threads);

    return 0;
}
