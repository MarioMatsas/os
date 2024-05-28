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

    // Order timers
    struct timespec order_start, order_end;
    // Delivery timers
    struct timespec delivery_start, delivery_end;
    // Order timers for max/average stats
    struct timespec overall_start, overall_end;
    // Delivery timers for max/average stats
    struct timespec cold_start, cold_end;

    clock_gettime(CLOCK_REALTIME, &overall_start); // ----------------- OVERALL START

    // The first customer (thread) doesn't wait, however every other one does
    pthread_mutex_lock(&s_lock);
    if (s == 0) {
        s += 1;
        pthread_mutex_lock(&print_lock);
        printf("Id %d skipped!\n", id);
        pthread_mutex_unlock(&print_lock);
    } else {
        wait_time = Torderlow + rand_r(&seed) % (Torderhigh - Torderlow + 1);
        pthread_mutex_lock(&print_lock);	
        printf("Id %d waited.....%d\n", id, wait_time);
        pthread_mutex_unlock(&print_lock);
        sleep(wait_time);
    }
    pthread_mutex_unlock(&s_lock);

    // Get on the phone with the pizzaria
    pthread_mutex_lock(&tel_lock);
    while (N_tels == 0) {
		printf("Id %d waiting...\n", id);
        pthread_cond_wait(&tel_cond, &tel_lock);
    }
    N_tels--;
    pthread_mutex_unlock(&tel_lock);

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
    pthread_mutex_lock(&print_lock);
    printf("Id %d waited...2..%d\n", id, wait_time);
    pthread_mutex_unlock(&print_lock);
    sleep(wait_time);

    // Payment may fail
    choice = 1 + rand_r(&seed) % 100;
    if (choice <= Pfail) {
        pthread_mutex_lock(&success_lock);
        unsuccessful++;
        pthread_mutex_unlock(&success_lock);
        pthread_mutex_lock(&print_lock);
        printf("The transaction of %d has failed...\n", id);
        pthread_mutex_unlock(&print_lock);
		pthread_mutex_lock(&tel_lock);
		N_tels++;
		printf("%d\n", N_tels);
		pthread_cond_broadcast(&tel_cond);
		pthread_mutex_unlock(&tel_lock);
        pthread_exit(NULL);
    } else {
        pthread_mutex_lock(&success_lock);
        successful++;
        pthread_mutex_unlock(&success_lock);
        pthread_mutex_lock(&print_lock);
        printf("The transaction of %d was successful! Please wait for your pizzas...\n", id);
        pthread_mutex_unlock(&print_lock);
    }

    // Add the money to the sum
    pthread_mutex_lock(&revenue_lock);
    revenue += margaritas * Cm + pepperoni * Cp + special * Cs;
    pthread_mutex_unlock(&revenue_lock);

    // Update the total pizzas sold per kind
    pthread_mutex_lock(&total_pizzas_lock);
    total_margaritas_sold += margaritas;
    total_pepperonis_sold += pepperoni;
    total_specials_sold += special;
    pthread_mutex_unlock(&total_pizzas_lock);

    // Your order will be taken care of by the cook, so you disconnect the call
    pthread_mutex_lock(&tel_lock);
    N_tels++;
    pthread_cond_broadcast(&tel_cond);
	pthread_mutex_unlock(&tel_lock);
    

    clock_gettime(CLOCK_REALTIME, &order_start); // ----------------- ORDER START

    // Check for available cooks
    pthread_mutex_lock(&cook_lock);
    while (N_cooks == 0) {
        pthread_cond_wait(&cook_cond, &cook_lock);
    }
    N_cooks--;
    pthread_mutex_unlock(&cook_lock);

    // Prepare pizzas
    wait_time = pizzas_ordered * Tprep;
    sleep(wait_time);

    // Once you get a cook, look for available ovens
    pthread_mutex_lock(&oven_lock);
    while (N_ovens < pizzas_ordered) {
        pthread_cond_wait(&oven_cond, &oven_lock);
    }
    N_ovens -= pizzas_ordered;
    pthread_mutex_unlock(&oven_lock);

    // Release cook
    pthread_mutex_lock(&cook_lock);
    N_cooks++;
    pthread_cond_signal(&cook_cond);
    pthread_mutex_unlock(&cook_lock);

    // Bake pizzas
    wait_time = pizzas_ordered * Tbake;
    sleep(wait_time);

    clock_gettime(CLOCK_REALTIME, &cold_start); // ----------------- COLD START

    // After the pizzas are done we look for a delivery man
    pthread_mutex_lock(&dispatch_lock);
    while (N_deliverers == 0) {
        pthread_cond_wait(&dispatch_cond, &dispatch_lock);
    }
    N_deliverers--;
    pthread_mutex_unlock(&dispatch_lock);

    // Release ovens
    pthread_mutex_lock(&oven_lock);
    N_ovens += pizzas_ordered;
    pthread_cond_broadcast(&oven_cond);
    pthread_mutex_unlock(&oven_lock);

    // Pack the order
    wait_time = pizzas_ordered * Tpack;
    sleep(wait_time);

    clock_gettime(CLOCK_REALTIME, &order_end); // ----------------- ORDER END

    int order_time = order_end.tv_sec - order_start.tv_sec;
    pthread_mutex_lock(&print_lock);
    printf("Order %d was completed in %d minutes. \n", id, order_time);
    pthread_mutex_unlock(&print_lock);

    clock_gettime(CLOCK_REALTIME, &delivery_start); // ----------------- DELIVERY START

    // Delivery time
    wait_time = Tdellow + rand_r(&seed) % (Tdelhigh - Tdellow + 1);
    sleep(wait_time);

    clock_gettime(CLOCK_REALTIME, &delivery_end); // ----------------- DELIVERY END
    clock_gettime(CLOCK_REALTIME, &overall_end); // ----------------- OVERALL END
    clock_gettime(CLOCK_REALTIME, &cold_end); // ----------------- COLD END

    // Update order overall stats
    pthread_mutex_lock(&order_stats_lock);
    int o_time = overall_end.tv_sec - overall_start.tv_sec;
    order_waiting_total += o_time;
    if (o_time > max_order) {
        max_order = o_time;
    }
    pthread_mutex_unlock(&order_stats_lock);

    // Update cold stats
    pthread_mutex_lock(&delivery_stats_lock);
    int c_time = cold_end.tv_sec - cold_start.tv_sec;
    cold_waiting_total += c_time;
    if (c_time > max_cold) {
        max_cold = c_time;
    }
    pthread_mutex_unlock(&delivery_stats_lock);

    int delivery_time = delivery_end.tv_sec - delivery_start.tv_sec;
    pthread_mutex_lock(&print_lock);
    printf("Order %d was delivered in %d minutes. \n", id, delivery_time);
    pthread_mutex_unlock(&print_lock);

    // Go back to the store
    sleep(wait_time);

    // Release delivery man
    pthread_mutex_lock(&dispatch_lock);
    N_deliverers++;
    pthread_cond_signal(&dispatch_cond);
    pthread_mutex_unlock(&dispatch_lock);

    // * Done *
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    int Ncust = atoi(argv[1]);
    seed = atoi(argv[2]);
    int rc;

    int *id = malloc(Ncust * sizeof(int)); // Allocate memory 
    if (id == NULL){ return -1; }
    pthread_t *threads = malloc(Ncust * sizeof(pthread_t));
    if (threads == NULL) { return -1; }

    pthread_mutex_init(&print_lock, NULL);
    pthread_mutex_init(&revenue_lock, NULL);
    pthread_mutex_init(&s_lock, NULL);
    pthread_mutex_init(&tel_lock, NULL);
    pthread_cond_init(&tel_cond, NULL);
    pthread_mutex_init(&cook_lock, NULL);
    pthread_cond_init(&cook_cond, NULL);
    pthread_mutex_init(&oven_lock, NULL);
    pthread_cond_init(&oven_cond, NULL);
    pthread_mutex_init(&dispatch_lock, NULL);
    pthread_cond_init(&dispatch_cond, NULL);
    pthread_mutex_init(&total_pizzas_lock, NULL);
    pthread_mutex_init(&success_lock, NULL);
    pthread_mutex_init(&order_stats_lock, NULL);
    pthread_mutex_init(&delivery_stats_lock, NULL);

    for (int i = 0; i < Ncust; i++) {
        id[i] = i + 1;
        pthread_mutex_lock(&print_lock);
        printf("Main: Thread %d has been created\n", id[i]);
        pthread_mutex_unlock(&print_lock);

        rc = pthread_create(&threads[i], NULL, order, &id[i]);
        if (rc != 0) { return -1; }
    }

    for (int i = 0; i < Ncust; i++) {
        rc = pthread_join(threads[i], NULL);

        if (rc != 0) { return -1; }

        pthread_mutex_lock(&print_lock);
        printf("Main: Thread %d has been destroyed\n", id[i]);
        pthread_mutex_unlock(&print_lock);
    }

    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&revenue_lock);
    pthread_mutex_destroy(&s_lock);
    pthread_mutex_destroy(&tel_lock);
    pthread_cond_destroy(&tel_cond);
    pthread_mutex_destroy(&cook_lock);
    pthread_cond_destroy(&cook_cond);
    pthread_mutex_destroy(&oven_lock);
    pthread_cond_destroy(&oven_cond);
    pthread_mutex_destroy(&dispatch_lock);
    pthread_cond_destroy(&dispatch_cond);
    pthread_mutex_destroy(&total_pizzas_lock);
    pthread_mutex_destroy(&success_lock);
    pthread_mutex_destroy(&order_stats_lock);
    pthread_mutex_destroy(&delivery_stats_lock);

    average_order = (float)order_waiting_total / successful;
    average_cold = (float)cold_waiting_total / successful;
    printf("---------\n");
    printf("Total revenue: %d\n", revenue);
    printf("Margaritas: %d\n", total_margaritas_sold);
    printf("Pepperonis: %d\n", total_pepperonis_sold);
    printf("Specials: %d\n", total_specials_sold);
    printf("Successful orders: %d\n", successful);
    printf("Unsuccessful orders: %d\n", unsuccessful);
    printf("---------\n");
    printf("Average service time: %.2f\n", average_order);
    printf("Longest service time: %d\n", max_order);
    printf("---------\n");
    printf("Average colding time: %.2f\n", average_cold);
    printf("Longest colding time: %d\n", max_cold);

    free(id); // Free allocated memory
    free(threads);

    return 0;
}
