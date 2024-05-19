#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

pthread_mutex_t print_lock;
pthread_mutex_t revenue_lock;
pthread_mutex_t cook_lock;
pthread_cond_t cook_cond;
pthread_mutex_t oven_lock;
pthread_cond_t oven_cond;
pthread_mutex_t total_pizzas_lock;
unsigned int seed;
int revenue;
int total_margaritas_sold;
int total_pepperonis_sold;
int total_specials_sold;
//pthread_cond_t  cond;

int Ntel=2; // Recources
int Ncook=2;
int Noven=10;
int Ndeliverer=10;
int Torderlow=1; 
int Torderhigh=5;
int Norderlow=1; // Number of pizzas
int Norderhigh=5; 
int Pm=35; // Chances of pizza being a margarita, pepperoni or special
int Pp=25;
int Ps=40;
int Tpaymentlow=1; // Payment time
int Tpaymenthigh=3;
int Pfail=5; // Chance of order to fail
int Cm=10; // Pizza cost according to type of pizza
int Cp=11;
int Cs=12;
int Tprep=1; // Prep, bake and pack time
int Tbake=10;
int Tpack=1;
int Tdellow=5; // Delivery times
int Tdelhigh=15;

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
		pthread_mutex_lock(&print_lock);
		printf("The transaction of %d has failed...\n", id);
		pthread_mutex_unlock(&print_lock);
		pthread_exit(NULL);
	}
	else{
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
	while (Ncook == 0){
		pthread_cond_wait(&cook_cond, &cook_lock);
	}
	Ncook--;
	pthread_mutex_unlock(&cook_lock);
	
	// Prepare pizzas
	sleep(pizzas_ordered*Tprep);
	
	// ----- THIS PART IS ONLY FOR TESTING AND WILL BE DELETED! 
	pthread_mutex_lock(&cook_lock);
	Ncook++;
	pthread_cond_signal(&cook_cond);
	pthread_mutex_unlock(&cook_lock);
	
	// ------ THIS PART HERE REQUIRES MORE TO BE DONE IN ORDER TO WORK
	/*
	// Once you get a cook, look for available ovens
	pthread_mutex_lock(&oven_lock);
	while (Noven < pizzas_ordered){
		pthread_cond_wait(&oven_cond, &oven_lock);
	}
	Noven--;
	// Release cook
	pthread_mutex_lock(&cook_lock);
	Ncook++;
	pthread_cond_signal(&cook_cond);
	pthread_mutex_unlock(&cook_lock);
	pthread_mutex_unlock(&oven_lock); 
	*/
	// Bake pizzas
	//sleep(pizzas_ordered*Tbake);
	
	// Pretty sure pws prepei na apodesmeutoun oi fournoi afou bre8ei deliveras
	
	
	
	pthread_exit(NULL);
}

int main(int argc, char *argv[]){
	int Ncust = atoi(argv[1]);
    seed = atoi(argv[2]);
    int rc;

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

    free(id); // Free allocated memory
    free(threads); 

    return 0;
}
