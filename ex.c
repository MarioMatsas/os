#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "param.h"

pthread_mutex_t print_lock;
pthread_mutex_t revenue_lock;
pthread_mutex_t cook_lock;
pthread_cond_t cook_cond;
pthread_mutex_t oven_lock;
pthread_cond_t oven_cond;
pthread_mutex_t dispatch_lock;
pthread_cond_t dispatch_cond;
pthread_mutex_t total_pizzas_lock;
pthread_mutex_t success_lock;
unsigned int seed;
int revenue;
int total_margaritas_sold;
int total_pepperonis_sold;
int total_specials_sold;
int successful;
int unsuccessful;

pthread_mutex_t T_cooling_lock;
float T_cooling_max;
int T_cooling_N;
float T_cooling_sum;
struct timespec start, finish;
//pthread_cond_t  cond;

int N_cooks = Ncook; // TODO: modify rest similarly
int N_oven = Noven;
int N_dispatch = Ndeliverer;

void *order(void *x){
	
    int id = *(int *)x;
	/*
	pthread_mutex_lock(&print_lock);
	printf("Thread %d is ordering\n", id);
	pthread_mutex_unlock(&print_lock);*/ 
	// Select a random number of pizzas to order
	int pizzas_ordered = Norderlow + rand_r(&seed)%(Norderhigh - Norderlow - 1);
	
	int margaritas = 0;
	int pepperoni = 0;
	int special = 0;
	int choice;
	int wait_time;
	
	for (int i=0; i<pizzas_ordered; i++){
		// Select a random number for the precentage and depending on that, order one 
		// of the 3 options
		choice = 1 + rand_r(&seed)%100;
		if (choice <= 35){
			margaritas++;
		}
		else if (choice <= 60){
			pepperoni++;
		}
		else{
			special++;
		}
	}
	
	// Wait for payment
	wait_time = Tpaymentlow + rand_r(&seed)%(Tpaymenthigh - Tpaymentlow - 1);
	sleep(wait_time);
	
	// Payment may fail
	choice = 1 + rand_r(&seed)%100;
	if (choice <= 5){
		pthread_mutex_lock(&success_lock);
		unsuccessful++;
		pthread_mutex_unlock(&success_lock);
		pthread_mutex_lock(&print_lock);
		printf("The transaction of %d has failed...\n", id);
		pthread_mutex_unlock(&print_lock);
		pthread_exit(NULL);
	}
	else{
		pthread_mutex_lock(&success_lock);
		successful++;
		pthread_mutex_unlock(&success_lock);
		pthread_mutex_lock(&print_lock);
		printf("The transaction of %d was successful! PLease wait for your pizzas...\n", id);
		pthread_mutex_unlock(&print_lock);
	}
	
	// Add the money to the sum
	pthread_mutex_lock(&revenue_lock);
	revenue += margaritas*Cm + pepperoni*Cp + special*Cs;
	pthread_mutex_unlock(&revenue_lock);
	
	// Update the total pizzas sold per kind
	pthread_mutex_lock(&total_pizzas_lock);
	total_margaritas_sold += margaritas;
	total_pepperonis_sold += pepperoni;
	total_specials_sold += special;
	pthread_mutex_unlock(&total_pizzas_lock);
	
	// Check for available cooks
	pthread_mutex_lock(&cook_lock);
	while (N_cooks == 0){
		pthread_cond_wait(&cook_cond, &cook_lock);
	}
	N_cooks--;
	pthread_mutex_unlock(&cook_lock);
	
	// Prepare pizzas
	sleep(pizzas_ordered*Tprep);
	
	// ----- THIS PART IS ONLY FOR TESTING AND WILL BE DELETED! 
	//pthread_mutex_lock(&cook_lock);
	//N_cooks++;
	//pthread_cond_signal(&cook_cond);
	//pthread_mutex_unlock(&cook_lock);
	
	// ------ THIS PART HERE REQUIRES MORE TO BE DONE IN ORDER TO WORK
	
	// Once you get a cook, look for available ovens
	pthread_mutex_lock(&oven_lock);
	while (N_oven < pizzas_ordered){
		pthread_cond_wait(&oven_cond, &oven_lock);
	}
	N_oven -= pizzas_ordered;
	// Release cook
	pthread_mutex_lock(&cook_lock);
	N_cooks++;
	pthread_cond_signal(&cook_cond);
	pthread_mutex_unlock(&cook_lock);
	pthread_mutex_unlock(&oven_lock); 
	
	// Bake pizzas
	//sleep(pizzas_ordered*Tbake);
	// struct timespec start, finish; // needed for printing end stats, added by ppdms, don't remove if refactoring
	// clock_gettime(CLOCK_REALTIME, &start); // same as above
	
	// BEGIN DISPATCH
	// Check for available dispatchers
	pthread_mutex_lock(&dispatch_lock);
		while (N_dispatch == 0){
			pthread_cond_wait(&dispatch_cond, &dispatch_lock);
		}
		pthread_mutex_lock(&oven_lock);
			N_oven += pizzas_ordered;
		pthread_mutex_unlock(&oven_lock);
		N_dispatch--;
	pthread_mutex_unlock(&dispatch_lock);
	pthread_cond_signal(&dispatch_cond);
	int del_time = pizzas_ordered*Tpack + ( Tdellow + ( ( 2 * rand_r(&seed) * (Tdelhigh-Tdellow) ) / RAND_MAX ) );
	sleep(del_time);
	clock_gettime(CLOCK_REALTIME, &finish);
	// ignoring nanoseconds (considering we would potentially need to normalize them and assuming no need for such accuracy)
	int T_cooling_time_sec = finish.tv_sec - start.tv_sec; 
	pthread_mutex_lock(&print_lock);
		printf("Η παραγγελία με αριθμό %d παραδόθηκε σε %d λεπτά. \n", id, del_time);
	pthread_mutex_unlock(&print_lock);
	pthread_mutex_lock(&T_cooling_lock);
		T_cooling_sum += T_cooling_time_sec;
		T_cooling_N += 1;
		if (del_time > T_cooling_max) T_cooling_max = del_time;
	pthread_mutex_unlock(&T_cooling_lock);	
	
	pthread_exit(NULL);
}

int main(int argc, char *argv[]){
	int Ncust = atoi(argv[1]);
    seed = atoi(argv[2]);
    int rc; // return code

    int *id = malloc(Ncust * sizeof(int)); // Allocate memory 
    if (id == NULL){ return -1; }
    pthread_t *threads = malloc(Ncust * sizeof(pthread_t));
    if (threads == NULL){ return -1; }

    rc = pthread_mutex_init(&print_lock, NULL);
    if (rc != 0){ return -1; }

    for (int i = 0; i < Ncust; i++) {
        id[i] = i+1;
        pthread_mutex_lock(&print_lock);
		printf("Main: Thread %d has been created\n", id[i]);
		pthread_mutex_unlock(&print_lock);
        
        rc = pthread_create(&threads[i], NULL, order, &id[i]);
        if (rc != 0){ return -1; }
    }

    for (int i = 0; i < Ncust; i++) {    
        rc = pthread_join(threads[i], NULL);
        
        if (rc != 0){ return -1; }
        
        pthread_mutex_lock(&print_lock);
		printf("Main: Thread %d has been destroyed\n", id[i]);
		pthread_mutex_unlock(&print_lock);
    }
	
    rc = pthread_mutex_destroy(&print_lock);
    if (rc != 0){ return -1; }
    rc = pthread_mutex_destroy(&revenue_lock);
    if (rc != 0){ return -1; }
    rc = pthread_mutex_destroy(&cook_lock);
    if (rc != 0){ return -1; }
    rc = pthread_mutex_destroy(&oven_lock);
    if (rc != 0){ return -1; }
    rc = pthread_mutex_destroy(&total_pizzas_lock);
    if (rc != 0){ return -1; }
	rc = pthread_mutex_destroy(&success_lock);
    if (rc != 0){ return -1; }
	rc = pthread_mutex_destroy(&T_cooling_lock);
    if (rc != 0){ return -1; }

    free(id); // Free allocated memory
    free(threads); 

	// print stats here: single-threaded, no worrying about mutexes
	printf("Total revenue: %d\n", revenue);
    printf("Margaritas: %d\n", total_margaritas_sold);
    printf("Pepperonis: %d\n", total_pepperonis_sold);
    printf("Specials: %d\n", total_specials_sold);
    printf("Successful orders: %d\n", successful);
    printf("Unsuccessful orders: %d\n", unsuccessful);
	printf("Mέσος χρόνος κρυώματος των παραγγελιών: %f λεπτά\n", T_cooling_sum / T_cooling_N);
	printf("Μέγιστος χρόνος κρυώματος των παραγγελιών: %f λεπτά\n", T_cooling_max);
    return 0;
}
