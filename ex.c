#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

pthread_mutex_t print_lock;
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
	
	pthread_mutex_lock(&print_lock);
	printf("Thread %d is ordering\n", id);
	pthread_mutex_unlock(&print_lock);
	
	pthread_exit(NULL);
}

int main(int argc, char *argv[]){
	int Ncust = atoi(argv[1]);
    int seed = atoi(argv[2]);

    int *id = malloc(Ncust * sizeof(int)); // Allocate memory 
    pthread_t *threads = malloc(Ncust * sizeof(pthread_t));

    pthread_mutex_init(&print_lock, NULL);

    for (int i = 0; i < Ncust; i++) {
        id[i] = i+1;
        printf("Main: Thread %d has been created\n", id[i]);
        pthread_create(&threads[i], NULL, order, &id[i]);
    }

    for (int i = 0; i < Ncust; i++) {
        printf("Main: Thread %d has been destroyed\n", id[i]);
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&print_lock);

    free(id); // Free allocated memory
    free(threads); 

    return 0;
}
