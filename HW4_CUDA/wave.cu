/*********************************************************
*  DESCRIPTION:                                          *
*    Serial Concurrent Wave Equation - C Version         *
*    This program implements the concurrent wave equation*
**********************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>

#define MAXPOINTS 1000000
#define MINPOINTS 20
#define MAXSTEPS 1000000
#define PI 3.14159265

void check_param(void);
void init_line(void);
void update(void);
void printfinal(void);

int nsteps, /*number of time steps*/
    tpoints, /*total points along string*/
    rcode; /*generic return code*/

float values[MAXPOINTS + 2], /*values at time t*/
      oldval[MAXPOINTS + 2], /*values at time (t-dt)*/
      newval[MAXPOINTS + 2]; /*values at time (t+dt)*/
float v[MAXPOINTS + 2], /*serial used for comparing answers with parallel*/
      o[MAXPOINTS + 2],
      n[MAXPOINTS + 2];

/*********************************************************
*  Check input values from parameters                    *
**********************************************************/
void check_param(void){
    char tchar[20];
	
    /*check number of points, number of iterations*/
    while((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)){
        printf("Enter number of points along vibrating string [%d-%d]: ");
        scanf("%s", tchar);
	tpoints = atoi(tchar);
	if((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)){
            printf("Invalid. Please enter value between %d and %d\n", MINPOINTS, MAXPOINTS);
	}
    }
    while((nsteps < 1) || (nsteps > MAXSTEPS)){
	printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
	scanf("%s", tchar);
	nsteps = atoi(tchar);
	if((nsteps < 1) || (nsteps > MAXSTEPS)){
            printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
	}
    }
	
    printf("Using points = %d, steps = %d\n", tpoints, nsteps);
}

/*********************************************************
*  Initialize points on line                             *
**********************************************************/
void init_line(void){
    int i, j;
    float x, fac, k, tmp;
	
    /*Calculate initial values based on sine curve*/
    fac = 2.0 * PI;
    k = 0.0;
    tmp = tpoints - 1;
    for(j = 1; j <= tpoints; j++){
	x = k/tmp;
	values[j] = sin(fac * x);
        v[j] = values[j];
	k = k + 1.0;
    }
	
    /*Initialize old values array*/
    for(i = 1; i <= tpoints; i++){
	oldval[i] = values[i];
        o[i] = v[i];
    }
}

/*********************************************************
*  Calculate new values using wave equation              *
**********************************************************/
__global__ void do_math_kernel(float *val_d, float *old_d, float *new_d, int num_of_steps, int num_of_points){
    float dtime, c, dx, tau, sqtau;
	
    dtime = 0.3;
    c = 1.0;
    dx = 1.0;
    tau = (c * dtime / dx);
    sqtau = tau * tau;

    __shared__ float val_ds[1024];
    __shared__ float old_ds[1024];
    __shared__ float new_ds[1024];
    int tx = threadIdx.x;
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    
    val_ds[tx] = val_d[index];
    old_ds[tx] = old_d[index];

    for(int i = 1; i <= num_of_steps; i++){
        /*This part needs to access elements from global memory of GPU*/
        /*if(index == 0 || index == num_of_points - 1) new_d[index] = 0.0;
        else new_d[index] = (2.0 * val_d[index]) - old_d[index] + (sqtau * (-2.0) * val_d[index]);
        old_d[index] = val_d[index];
        val_d[index] = new_d[index];*/
        /*This part accesses elements from shared memory of GPU -> faster*/
        /*if(index == 0 || index == num_of_points - 1) new_ds[tx] = 0.0;
        else new_ds[tx] = (2.0 * val_ds[tx]) - old_ds[tx] + (sqtau * (-2.0) * val_ds[tx]);
        old_ds[tx] = val_ds[tx];
        val_ds[tx] = new_ds[tx];*/
        /*This part only takes values[2~tpoints-1] total tpoins-2 points from CPU to GPU in order to reduce branch overhead*/ 
        new_ds[tx] = (2.0 * val_ds[tx]) - old_ds[tx] + (sqtau * (-2.0) * val_ds[tx]);
        old_ds[tx] = val_ds[tx];
        val_ds[tx] = new_ds[tx];
        
    }
    __syncthreads();
    val_d[index] = val_ds[tx];
    
}

void do_math(int i){
    float dtime, c, dx, tau, sqtau;
	
    dtime = 0.3;
    c = 1.0;
    dx = 1.0;
    tau = (c * dtime / dx);
    sqtau = tau * tau;
    newval[i] = (2.0 * values[i]) - oldval[i] + (sqtau * (-2.0) * values[i]);
    //n[i] = (2.0 * v[i]) - o[i] + (sqtau * (-2.0) * v[i]);
}

/***********************************************************
*  Update all values along line a specified number of times*
***********************************************************/
void updateOnDevice(){
    //int size = tpoints * sizeof(float);
    int size = (tpoints - 2) * sizeof(float);
    float *val_d, *old_d, *new_d; //memory on device

    /*1.Allocate device memory and move initiail values[] and oldval[] to GPU*/
    cudaMalloc(&val_d, size);
    //cudaMemcpy(val_d, values+1, size, cudaMemcpyHostToDevice);
    cudaMemcpy(val_d, values+2, size, cudaMemcpyHostToDevice);
    cudaMalloc(&old_d, size);
    //cudaMemcpy(old_d, oldval+1, size, cudaMemcpyHostToDevice);
    cudaMemcpy(old_d, oldval+2, size, cudaMemcpyHostToDevice);
    cudaMalloc(&new_d, size);
	
    /*2.Invoke kernel function, each thread calculates a value[] element*/
    int threads_per_block, blocks_per_grid = tpoints/1024 + 1;
    if(tpoints > 1024) threads_per_block = 1024;
    else threads_per_block = tpoints;
    dim3 dimBlock(threads_per_block); 
    dim3 dimGrid(blocks_per_grid);
    do_math_kernel<<<dimGrid,dimBlock>>>(val_d, old_d, new_d, nsteps, tpoints);

    /*3.Read final results from GPU to CPU*/
    //cudaMemcpy(values+1, val_d, size, cudaMemcpyDeviceToHost);
    cudaMemcpy(values+2, val_d, size, cudaMemcpyDeviceToHost);
    cudaFree(val_d); cudaFree(old_d); cudaFree(new_d);
}

void update(){
    int i, j;
	
    /*Update values for each time step*/
    for(i = 1; i <= nsteps; i++){
        /*Update points along line for this time step*/
        for(j = 1; j <= tpoints; j++){
	    /*global endpoints*/
            if((j == 1) || (j == tpoints)){
                newval[j] = 0.0;
                //n[j] = 0.0;
            }
	    else do_math(j);
	}
        /*Update old values with new values*/
        for(j = 1; j <= tpoints; j++){
	    oldval[j] = values[j];
	    values[j] = newval[j];
            /*o[j] = v[j];
            v[j] = n[j];*/
        }
    }
	
}

/**********************************************************
*  Print final results                                    *
**********************************************************/
void printfinal(){
    int i;

    for(i = 1; i <= tpoints; i++){
	printf("%6.4f ", values[i]);
	if(i%10 == 0) printf("\n");
    }	
}

/**********************************************************
*  Check serial and parallel answers                      *
**********************************************************/
void check_answer(){
    int wrong = 0, num = 0;
    for(int i = 1; i <= tpoints; i++){
        if(values[i]!=v[i]){
            wrong = 1;
            num++;
        }
    }
    if(wrong == 0) printf("right\n");
    else printf("%d are wrong\n", num);
    /*In command line ./ cuda_wave.out > [filename] to pipe output to the file
      then use diff file1 file2 to see if there is any difference*/
}

/*********************************************************
*  Main Program                                          *
**********************************************************/
int main(int argc, char *argv[]){
    sscanf(argv[1], "%d", &tpoints);
    sscanf(argv[2], "%d", &nsteps);
	
    check_param();
	
    printf("Initializing points on the line...\n");
    init_line();
    //printfinal();
	
    printf("Updating all points for all time steps...\n");
    //update();
    updateOnDevice();
	
    printf("Printing final results...\n");
    printfinal();
    printf("\nDone.\n\n");
	
    return 0;
}	  
