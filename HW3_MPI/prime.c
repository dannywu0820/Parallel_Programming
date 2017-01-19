#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include"mpi.h"
#include<sys/time.h>

int isprime(int n){
    int i, squareroot;
    if(n > 10){
        squareroot = (int)sqrt(n);
        for(i = 3; i <= squareroot; i = i + 2){
            if((n % i) == 0) return 0;
        }
        return 1;
    }
    else return 0;
}

int main(int argc, char *argv[]){
    long long int pc, foundone, total = 0, largest = 0; //prime counter & most recent prime found
    long long int n, limit, lower, upper, avg;
    int rank, size;
    struct timeval start, finish;
    int sec, usec;
    MPI_Status status;

    sscanf(argv[1], "%llu", &limit);

    gettimeofday(&start, 0);    

    pc = 0; //assume input is much larger than 10, need to +4 at last
    //for(int i = 0; i < 4; i++) local_large[i] = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    long long int local_large[size];
    for(int i = 0; i < size; i++) local_large[i] = 0;
    if(rank == 0){
        printf("Starting. Numbers to be scanned= %lld\n", limit);
    }
    
    avg = (limit-10)/size;
    lower = avg * rank + 11;
    if(rank == size-1) upper = limit;
    else upper = lower + avg - 1;
    if(avg % 2 != 0 && rank % 2 != 0) lower++;

	for(n = 11 + 2 * rank; n <= limit; n+=2*size){ //faster
    //for(n = lower; n <= upper; n = n + 2){
        if(isprime(n)){
            pc++;
            foundone = n;
        }
    }
    
    //MPI_Barrier(MPI_COMM_WORLD); 
    MPI_Reduce(&pc, &total, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    total += 4;
    MPI_Gather(&foundone, 1, MPI_LONG, &local_large, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    
    gettimeofday(&finish, 0);
    sec = finish.tv_sec - start.tv_sec;
    usec = finish.tv_usec - start.tv_usec;
        
    if(rank == 0){
        /*printf("collector process %d of %d ", rank, size);
        printf("deal with %lld~%lld ", lower, upper);
        printf("pc: %lld, foundone: %lld, total: %lld\n", pc, foundone, total);*/
        
		for(int i = size - 1; i >= 0; i--){ 
            if(local_large[i] > largest){
                largest = local_large[i];
            }
        }
        printf("Done. Largest prime is %lld Total primes %lld\n", largest, total);
        /*total+=pc;
        largest = foundone;
        for(int i = 1; i < size; i++){
            MPI_Recv(&pc, 1, MPI_LONG, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&foundone, 1, MPI_LONG, i, 0, MPI_COMM_WORLD, &status);
            total+=pc;
            if(foundone > largest) largest = foundone;
        }
        printf("Done. Largest prime is %lld Total primes %lld\n", largest, total);*/
		
        /*gettimeofday(&finish, 0);
        sec = finish.tv_sec - start.tv_sec;
        usec = finish.tv_usec - start.tv_usec;
        printf("elapsed time: %f sec\n", sec+(usec/1000000.0));*/
    }
    else{
        /*printf("worker process %d of %d ", rank, size);
        printf("deal with %lld~%lld ", lower, upper);
        printf("pc: %lld, foundone: %lld\n", pc, foundone);*/
		
        /*MPI_Send(&pc, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&foundone, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);*/
      
    }
    MPI_Finalize();
    return 0;
}
