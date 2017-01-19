//2 reasons that slower the performance
//using rand() is not thread safe -> using rand_r() instead
//using array to store local sum causes Cache Coherence Problem -> declaring local sum in Monte_Carlo and using mutex to add local sum to number_in_circle
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<iomanip> //setprecision()
#include<time.h> //time()
#include<sys/time.h> //gettimeofday()
#include<pthread.h>
#include<string> //atoi() strtoll()
#include<math.h> //round()
#include<sys/sysinfo.h> //get_nprocs()

using namespace std;

//#define NUM_OF_THREADS 16
int NUM_OF_THREADS;
pthread_mutex_t Mutex;

long long int number_in_circle = 0, number_of_toss; //-(2^63)~2^63-1

struct parameters{
    int thread_number;
    long long int times; //average times that each thread needs to do
    long long int *my_in_circle; //local sum of dots in circle of a thread
};

void* Monte_Carlo(void *data){
    struct parameters p_get = *(struct parameters*)data;
    long long int start = p_get.times * p_get.thread_number + 1;
    long long int end = p_get.times * (p_get.thread_number + 1);
    double x, y, distance_square;
    double low = 0.0, up = 2.0; //lower bound is 0, upper bound is 2 
    struct timeval start_time, end_time;
    int sec, usec;
    long long int local_in_circle = 0; //local sum of dots in circle of a thread

    gettimeofday(&start_time, 0);

    //randomly generate a number between [-1,1] by left shifting [0,2]
    unsigned seed = (unsigned)time(NULL); srand(seed);
    for(long long int i = start; i <= start + p_get.times; i++){
        x = (double)((up - low) * ((double)rand_r(&seed)/(double)RAND_MAX) - 1.0);
        y = (double)( (up - low) * rand_r(&seed)/RAND_MAX - 1.0);
        //x = (double)((up - low) * ((double)rand()/(double)RAND_MAX) - 1.0);
        //y = (double)( (up - low) * rand()/RAND_MAX - 1.0);
        distance_square = x*x + y*y; 
        //cout << "x:" << x << " y:" << y << " distance^2:"<< distance_square << endl; 
        if(distance_square <= 1.0){
            local_in_circle++;
            //(*p_get.my_in_circle)++; //Cache Coherence Problem
        }
    }

    if(p_get.thread_number == (NUM_OF_THREADS-1)){ //last thread
        //cout << end+1 << " " << number_of_toss << endl;
        for(long long int i = end + 1; i <= number_of_toss; i++){
            x = (double)( (up - low) * rand_r(&seed)/RAND_MAX - 1.0);
            y = (double)( (up - low) * rand_r(&seed)/RAND_MAX - 1.0);
            //x = (double)( (up - low) * rand()/RAND_MAX - 1.0);
            //y = (double)( (up - low) * rand()/RAND_MAX - 1.0);
            distance_square = x*x + y*y; 
            if(distance_square <= 1.0){ 
                local_in_circle++;
                //(*p_get.my_in_circle)++; //Cache Coherence Problem
            }
        }
    }

    gettimeofday(&end_time, 0);
    sec = end_time.tv_sec - start_time.tv_sec;
    usec = end_time.tv_usec - start_time.tv_usec;

    pthread_mutex_lock(&Mutex);
    number_in_circle += local_in_circle;
    cout << "Thread" << p_get.thread_number << " runs " << sec + (usec/10000000.0) << " sec" << endl;
    pthread_mutex_unlock(&Mutex);
} 

int main(int argc, char *argv[]){
    pthread_t *thread;
    struct parameters *para;
    long long int *local_in_circle;
    double pi_estimate = 0.0;
    /*struct timeval start_time, end_time;
    int sec, usec;

    gettimeofday(&start_time, 0);*/
     
    //NUM_OF_THREADS = atoi(argv[1]);
    NUM_OF_THREADS = get_nprocs();
    number_of_toss = strtoll(argv[1], NULL, 10); //string to long long int
    //printf("%lld\n", number_of_toss);

    //dynamically allocate memory space
    thread = (pthread_t *)malloc(NUM_OF_THREADS * sizeof(pthread_t));
    para = (struct parameters *)malloc(NUM_OF_THREADS * sizeof(struct parameters));
    local_in_circle = (long long int *)malloc(NUM_OF_THREADS * sizeof(long long int));
    for(int i = 0; i < NUM_OF_THREADS; i++) local_in_circle[i] = 0;
    pthread_mutex_init(&Mutex, NULL);
    
    for(int i = 0; i < NUM_OF_THREADS; i++){
        para[i].thread_number = i;
        para[i].times = (long long int)(number_of_toss/NUM_OF_THREADS);
        //para[i].my_in_circle = &local_in_circle[i]; //call by reference
        pthread_create(&thread[i], NULL, Monte_Carlo, (void *)&para[i]);
    }
    
    for(int i = 0; i < NUM_OF_THREADS; i++){
        pthread_join(thread[i], NULL);
    }

    /*for(int i = 0; i < NUM_OF_THREADS; i++){
        //cout << local_num_in_circle[i] << endl;
        number_in_circle += local_in_circle[i];
    }*/
    pi_estimate = (double)4.0*number_in_circle/(double)number_of_toss;
    
    /*gettimeofday(&end_time, 0);
    sec = end_time.tv_sec - start_time.tv_sec;
    usec = end_time.tv_usec - start_time.tv_usec;*/

    //cout << "time:" << sec + (usec/10000000.0) << " sec" << endl;
    //printf("time:%f sec \r\n", sec+(usec/1000000.0));
    
    //cout << "number_in_circle:" << number_in_circle << endl;
    //cout << "pi_estimate:" 
    //cout << setprecision(16) << pi_estimate << endl;
    printf("%.15lf\n", pi_estimate);

    pthread_mutex_destroy(&Mutex);
    free(thread);
    free(para);
    free(local_in_circle);
    return 0;
}
