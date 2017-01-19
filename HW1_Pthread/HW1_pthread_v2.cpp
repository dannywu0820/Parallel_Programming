#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
#include<sys/time.h>
#include<iostream>
#include<iomanip>
#include<semaphore.h>

using namespace std;

int number_of_threads;
long long int number_in_circle = 0, number_of_toss;
pthread_mutex_t Mutex;
sem_t Semaphore;

struct parameter{
    int serial_number;
    long long int my_start;
    long long int my_end;
};

void* Monte_Carlo(void *data){
    struct parameter get = *(struct parameter *)data;
    double x, y, distance_square;
    double upper = 2.0, lower = 0.0; //upper bound and lower bound
    long long int local_number_in_circle = 0;    

    unsigned seed = (unsigned)time(NULL); //srand(seed); rand()->rand_r()
    for(long long int i = get.my_start; i <= get.my_end; i++){
        x = (double)((upper - lower) * rand_r(&seed)/RAND_MAX - 1.0);
        y = (double)((upper - lower) * rand_r(&seed)/RAND_MAX - 1.0);
        distance_square = x*x + y*y;
        if(distance_square <= 1.0){
            //sem_wait(&Semaphore);
            //number_in_circle++; //synchronization for too many times
            //pthread_mutex_unlock(&Mutex);
            local_number_in_circle++;
        }
    }

    pthread_mutex_lock(&Mutex);
    //pthread_mutex_lock(&Mutex);
    number_in_circle += local_number_in_circle;
    pthread_mutex_unlock(&Mutex);
    //sem_post(&Semaphore);
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage:./FILE_NAME NUM_OF_THREADS NUM_OF_TOSS\n");
        exit(-1);
    }
    number_of_threads = atoi(argv[1]);
    number_of_toss = strtoll(argv[2], NULL, 10);

    pthread_mutex_init(&Mutex, NULL);
    //sem_init(&Semaphore, 0, 1); //2nd is # of sharing, 3rd is initial value
    
    pthread_t *Thread = (pthread_t *)malloc(number_of_threads * sizeof(pthread_t));
    struct parameter *Parame = (struct parameter *)malloc(number_of_threads * sizeof(struct parameter));
    long long int avg_times = number_of_toss/(long long int)number_of_threads;
    Parame[0].serial_number;
    Parame[0].my_start = 1; 
    Parame[0].my_end = avg_times;
    //cout << "Thread0 " << Parame[0].my_start << " " << Parame[0].my_end << endl;
    struct timeval start_time, end_time;
    int sec, usec;
    gettimeofday(&start_time, 0);

    for(int i = 1; i < number_of_threads; i++){
        Parame[i].serial_number = i;
        Parame[i].my_start = Parame[i-1].my_end + 1;
        if(i == number_of_threads - 1){ Parame[i].my_end = number_of_toss;}
        else{ Parame[i].my_end = Parame[i].my_start + avg_times;}
        //cout << "Thread" << i << " " << Parame[i].my_start << " " << Parame[i].my_end << endl;
    }


    for(int i = 0; i < number_of_threads; i++){ //pthread_create
        pthread_create(&Thread[i], NULL, Monte_Carlo, (void *)&Parame[i]);
    } 
    for(int i = 0; i < number_of_threads; i++){ //pthread_join
        pthread_join(Thread[i], NULL);
    }

    gettimeofday(&end_time, 0);
    sec = end_time.tv_sec - start_time.tv_sec;
    usec = end_time.tv_usec - start_time.tv_usec;
    cout << sec + (usec/1000000.0) << " sec" << endl;    

    cout << setprecision(16) << (double)4*number_in_circle/(double)number_of_toss << endl;    

    pthread_mutex_destroy(&Mutex);
    //sem_destroy(&Semaphore);
    free(Thread);
    free(Parame);
    return 0;
}
