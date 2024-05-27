#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "p3220120-p3220150-p3220227-pizza.h"

pthread_mutex_t tel_lock;
pthread_cond_t tel_cond;


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

//jim's addition---------------
pthread_mutex_t T_serving_lock;
float T_serving_max;
int T_serving_N;
float T_serving_sum;
//=----------------------------

int N_cooks = Ncook;  // TODO: modify rest similarly
int N_oven = Noven;
int N_dispatch = Ndeliverer;

//jim addition-------------------
int N_tels = Ntel;
int s = 0; //μεταβλητή, χρήσιμη για να μας πεί αν είμαστε στην χρονική στιγμή 0 (0) ή κάποια άλλη (>0)
//-------------------------------

void *order(void *x){

//jim's addition---------------------------
	int wait_time;
    struct timespec start_s, finish_s;//to calculate the serving time
    struct timespec start_r, finish_r;//to calculate the ready time
//-----------------------------------------	
    int id = *(int *)x;
    
    //---------------------------------------
    clock_gettime(CLOCK_REALTIME, &start_s);//for serving time
    //---------------------------------------
	
	//tel part (jim part)------------------
	
	pthread_mutex_lock(&print_lock);
	printf("Thread %d is ordering\n", id);
	pthread_mutex_unlock(&print_lock);
	
	// Wait for ordering, only after the zero seconds
	
	pthread_mutex_lock(&tel_lock);
	if(!s){
		s += 1;
	}
	else{
		wait_time = Torderlow + rand_r(&seed)%(Torderhigh - Torderlow - 1);
		sleep(wait_time);
	}
	pthread_mutex_unlock(&tel_lock);
	
	
	// Check for available telephones, else wait
	pthread_mutex_lock(&tel_lock);
	while (N_tels == 0){
		pthread_cond_wait(&tel_cond, &tel_lock);
	}
	N_tels--;
	pthread_mutex_unlock(&tel_lock);
	
	//-------------------------------------
	
	int pizzas_ordered =
        Norderlow + rand_r(&seed) % (Norderhigh - Norderlow + 1);

    int margaritas = 0;
    int pepperoni = 0;
    int special = 0;
    int choice;
    //int wait_time;

    for (int i = 0; i < pizzas_ordered; i++) {
        choice = rand_r(&seed) % 100 + 1;
        if (choice <= 35) {
            margaritas++;
        } else if (choice <= 60) {
            pepperoni++;
        } else {
            special++;
        }
    }

    wait_time = Tpaymentlow + rand_r(&seed) % (Tpaymenthigh - Tpaymentlow + 1);
    sleep(wait_time);

    choice = rand_r(&seed) % 100 + 1;
    if (choice <= 5) {
        pthread_mutex_lock(&success_lock);
        unsuccessful++;
        pthread_mutex_unlock(&success_lock);
        pthread_mutex_lock(&print_lock);
        printf("The transaction of %d has failed...\n", id);
        pthread_mutex_unlock(&print_lock);
        pthread_exit(NULL);
    } else {
        pthread_mutex_lock(&success_lock);
        successful++;
        pthread_mutex_unlock(&success_lock);
        pthread_mutex_lock(&print_lock);
        printf(
            "The transaction of %d was successful! Please wait for your "
            "pizzas...\n",
            id);
        pthread_mutex_unlock(&print_lock);
    }

    pthread_mutex_lock(&revenue_lock);
    revenue += margaritas * Cm + pepperoni * Cp + special * Cs;
    pthread_mutex_unlock(&revenue_lock);

    pthread_mutex_lock(&total_pizzas_lock);
    total_margaritas_sold += margaritas;
    total_pepperonis_sold += pepperoni;
    total_specials_sold += special;
    pthread_mutex_unlock(&total_pizzas_lock);
    
    //end of telephone----------------
    pthread_mutex_lock(&tel_lock);
    N_tels++;
    pthread_cond_signal(&tel_cond);
    pthread_mutex_unlock(&tel_lock);
    //--------------------------------

	//begin order---------------------
	clock_gettime(CLOCK_REALTIME, &start_r);
	//--------------------------------

    pthread_mutex_lock(&cook_lock);
    while (N_cooks == 0) {
        pthread_cond_wait(&cook_cond, &cook_lock);
    }
    N_cooks--;
    pthread_mutex_unlock(&cook_lock);

    // Prepare pizzas
    sleep(pizzas_ordered * Tprep);

    // Once you get a cook, look for available ovens
    pthread_mutex_lock(&oven_lock);
    while (N_oven < pizzas_ordered) {
        pthread_cond_wait(&oven_cond, &oven_lock);
    }
    N_oven -= pizzas_ordered;
    pthread_mutex_unlock(&oven_lock);

    pthread_mutex_lock(&cook_lock);
    N_cooks++;
    pthread_cond_signal(&cook_cond);
    pthread_mutex_unlock(&cook_lock);
    pthread_mutex_unlock(&oven_lock);

    // Bake pizzas
    sleep(pizzas_ordered * Tbake);
    
    struct timespec start, finish;  // needed for printing end stats, added by
                                    // ppdms, don't remove if refactoring
    clock_gettime(CLOCK_REALTIME, &start);  // same as above

    // BEGIN DISPATCH
    // Check for available dispatchers
    pthread_mutex_lock(&dispatch_lock);
    while (N_dispatch == 0) {
        pthread_cond_wait(&dispatch_cond, &dispatch_lock);
    }
    N_dispatch--;
    pthread_mutex_unlock(&dispatch_lock);

    pthread_mutex_lock(&oven_lock);
    N_oven += pizzas_ordered;
    pthread_cond_signal(&oven_cond);
    pthread_mutex_unlock(&oven_lock);
    
    //order is ready-----------------------------------
    clock_gettime(CLOCK_REALTIME, &finish_r);
    //-------------------------------------------------

	//calculating the ready_time--------------------
	int ready_time = finish_r.tv_sec - start_r.tv_sec;
	
	pthread_mutex_lock(&print_lock);
    printf("Η παραγγελία με αριθμό %d ετοιμάστηκε σε %d λεπτά. \n", id,
           ready_time);
    pthread_mutex_unlock(&print_lock);
	//-----------------------------------------------------

    int del_time = Tdellow + ((rand_r(&seed) * (Tdelhigh - Tdellow)) / RAND_MAX);
    int sleep_time = pizzas_ordered * Tpack + 2*del_time;
    sleep(sleep_time);
    clock_gettime(CLOCK_REALTIME, &finish);
    // ignoring nanoseconds (considering we would potentially need to normalize
    // them and assuming no need for such accuracy)
    float T_cooling_time_sec = finish.tv_sec - start.tv_sec;

    pthread_mutex_lock(&print_lock);
    printf("Η παραγγελία με αριθμό %d παραδόθηκε σε %d λεπτά. \n", id,
           del_time);
    pthread_mutex_unlock(&print_lock);
    
    //-----------------------------------------------------
    clock_gettime(CLOCK_REALTIME, &finish_s);//for serving time
    
    float T_serving_time_sec = finish_s.tv_sec - start_s.tv_sec;
    //-----------------------------------------------------

    pthread_mutex_lock(&T_cooling_lock);
    T_cooling_sum += T_cooling_time_sec;
    T_cooling_N++;
    if (del_time > T_cooling_max) T_cooling_max = del_time;
    pthread_mutex_unlock(&T_cooling_lock);
    
    pthread_mutex_lock(&T_serving_lock);//to calculate the serving time, sum , max and N
    T_serving_sum += T_serving_time_sec;
    T_serving_N++;
    if (T_serving_time_sec > T_serving_max) T_serving_max = T_serving_time_sec;
    pthread_mutex_unlock(&T_serving_lock);

    pthread_mutex_lock(&dispatch_lock);
    N_dispatch++;
    pthread_cond_signal(&dispatch_cond);
    pthread_mutex_unlock(&dispatch_lock);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Wrong number of arguments\n");
        return -1;
    }
    int Ncust = atoi(argv[1]);
    seed = atoi(argv[2]);
    int rc;  // return code

    int *id = malloc(Ncust * sizeof(int));  // Allocate memory
    if (id == NULL) {
        return -1;
    }
    pthread_t *threads = malloc(Ncust * sizeof(pthread_t));
    if (threads == NULL) {
        return -1;
    }

    pthread_mutex_init(&print_lock, NULL);
    pthread_mutex_init(&revenue_lock, NULL);
    pthread_mutex_init(&cook_lock, NULL);
    pthread_cond_init(&cook_cond, NULL);
    pthread_mutex_init(&oven_lock, NULL);
    pthread_cond_init(&oven_cond, NULL);
    pthread_mutex_init(&dispatch_lock, NULL);
    pthread_cond_init(&dispatch_cond, NULL);
    pthread_mutex_init(&total_pizzas_lock, NULL);
    pthread_mutex_init(&success_lock, NULL);
    pthread_mutex_init(&T_cooling_lock, NULL);


    for (int i = 0; i < Ncust; i++) {
        id[i] = i + 1;
        pthread_mutex_lock(&print_lock);
        printf("Main: Thread %d has been created\n", id[i]);
        pthread_mutex_unlock(&print_lock);

        rc = pthread_create(&threads[i], NULL, order, &id[i]);
        if (rc != 0) {
            return -1;
        }
    }

    for (int i = 0; i < Ncust; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            return -1;
        }

        pthread_mutex_lock(&print_lock);
        printf("Main: Thread %d has been destroyed\n", id[i]);
        pthread_mutex_unlock(&print_lock);
    }

    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&revenue_lock);
    pthread_mutex_destroy(&cook_lock);
    pthread_cond_destroy(&cook_cond);
    pthread_mutex_destroy(&oven_lock);
    pthread_cond_destroy(&oven_cond);
    pthread_mutex_destroy(&dispatch_lock);
    pthread_cond_destroy(&dispatch_cond);
    pthread_mutex_destroy(&total_pizzas_lock);
    pthread_mutex_destroy(&success_lock);
    pthread_mutex_destroy(&T_cooling_lock);

    free(id);  // Free allocated memory
    free(threads);

    printf("Total revenue: %d\n", revenue);
    printf("Margaritas: %d\n", total_margaritas_sold);
    printf("Pepperonis: %d\n", total_pepperonis_sold);
    printf("Specials: %d\n", total_specials_sold);
    printf("Successful orders: %d\n", successful);
    printf("Unsuccessful orders: %d\n", unsuccessful);
    //jim's addition---------------------------------------------------
    printf("Μέσος χρόνος εξυπηρέτησης πελατών: %f λεπτά\n",
           T_serving_sum / T_serving_N);
    printf("Μέγιστος χρόνος εξυπηρέτησης πελατών: %f λεπτά\n",
           T_serving_max);
           //----------------------------------------------------------
    printf("Μέσος χρόνος κρυώματος των παραγγελιών: %f λεπτά\n",
           T_cooling_sum / T_cooling_N);
    printf("Μέγιστος χρόνος κρυώματος των παραγγελιών: %f λεπτά\n",
           T_cooling_max);

    return 0;
}
