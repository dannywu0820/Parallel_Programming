#include<stdio.h>
#include<math.h>
#include"mpi.h"
#include<sys/time.h>

#define PI 3.1415926535

int main(int argc, char *argv[]){
    long long int i, num_intervals, avg, upper, lower;
    double rect_width, area, sum = 0, x_middle, local_sum = 0;
    int size, rank;
    struct timeval start, finish;
    int sec, usec;
    MPI_Status status;

    sscanf(argv[1], "%llu", &num_intervals);
    rect_width = PI/num_intervals;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    gettimeofday(&start, 0);

    avg = num_intervals/size;
    lower = rank * avg + 1;
    if(rank == size - 1) upper = num_intervals;
    else upper = (rank + 1) * avg;//lower + avg;

    for(i = lower; i < upper + 1; i++){
        x_middle = (i - 0.5) * rect_width;
        area = sin(x_middle) * rect_width;
        local_sum = local_sum + area;
    }
    
    if(rank == 0){
        //printf("process %d deals with %lld~%lld\n", rank, lower, upper);
        sum += local_sum;
        for(int j = 1; j < size; j++){
            MPI_Recv(&local_sum, 1, MPI_DOUBLE, j, 0, MPI_COMM_WORLD, &status);
            sum += local_sum;
        }
        printf("The total area is: %f\n", (float)sum);
        gettimeofday(&finish, 0);
        sec = finish.tv_sec - start.tv_sec;
        usec = finish.tv_usec - start.tv_usec;
        //printf("elapsed time: %f msec\n", sec*1000.0+(usec/1000.0));
    }
    else{
        //printf("process %d deals with %lld~%lld\n", rank, lower, upper);
        MPI_Send(&local_sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
