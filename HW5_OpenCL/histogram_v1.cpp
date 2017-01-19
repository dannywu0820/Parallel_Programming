#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<fstream>
#include<CL/cl.h> //still need to add -lOpenCL when compiling

//for read_kernel_from_file()
#include<sys/types.h>
#include<sys/stat.h>

using namespace std;

int read_kernel_from_file(char *filename, char **source, size_t *len){
    //need to put the kernel function into a file named xxx.cl
    struct stat statbuf;
    FILE *fp;
    size_t file_len;
 
    fp = fopen(filename, "r");
    if(fp == 0) return -1; //failed reading file

    stat(filename, &statbuf);
    file_len = (size_t)statbuf.st_size;
    *len = file_len;
    *source = (char *)malloc(file_len+1);
    fread(*source, file_len, 1, fp);
    (*source)[file_len] = '\0';

    fclose(fp);
    return 0;
}

int main(int argc, char const *argv[]){
    unsigned int *histogram_results = new unsigned int[3*256];
    for(int j = 0; j < 3*256; j++) histogram_results[j] = 0;
    unsigned int i = 0, a, input_size;
    fstream inFile("input", ios_base::in);
    ofstream outFile("0116233.out", ios_base::out);
	
    inFile >> input_size; //first line of input file
    unsigned int *image = new unsigned int[input_size];
    while(inFile >> a){
	image[i++] = a;
    }
        
    //calculating R,G,B histogram can execute in parallel

    cl_int err; //error return value of OpenCL function
    cl_platform_id platform;
    cl_device_id device;
    cl_device_type device_type = CL_DEVICE_TYPE_GPU;
    char Name[512], Vendor[512], Version[512];
    cl_context context;
    cl_command_queue queue;
    cl_mem i_buffer;
    cl_mem o_buffer;
    cl_program program;
    cl_kernel histogram_kernel;
    char filename[] = "histogram.cl";	
    char *source[1];
    size_t src_len[1];
       
    err = clGetPlatformIDs(1, &platform, NULL);
    /*if(err != CL_SUCCESS){
        printf("clGetPlatformIDs() failed: %d\n", err);
        return EXIT_FAILURE;
    }*/
      	
    err = clGetDeviceIDs(platform, device_type, 1, &device, NULL);
    /*if(err != CL_SUCCESS){
        printf("clGetDeviceIDs() failed: %d\n", err);
        return EXIT_FAILURE;
    }*/

     err = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(Vendor), Vendor, NULL);
     err |= clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(Name), Name, NULL);
     err |= clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(Version), Version, NULL);
     /*if(err != CL_SUCCESS){
        printf("clGetDeviceInfo() failed: %d\n", err);
        return EXIT_FAILURE;
     }
     else printf("OpenCL Device Vendor = %s, Name = %s, Version = %s\n", Vendor, Name, Version);*/

     context = clCreateContext(0, 1, &device, NULL, NULL, &err);
     /*if(!context || err){
        printf("clCreateContext() failed: %d\n", err);
        return EXIT_FAILURE;
     }*/

     queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
     /*if(!queue || err){
         printf("clCreateCommandQueue() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     i_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, input_size * sizeof(unsigned int), NULL, &err);
     /*if(!i_buffer || err){
         printf("clCreateBuffer() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     o_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 3 * 256 * sizeof(unsigned int), NULL, &err);
     /*if(!o_buffer || err){
         printf("clCreateBuffer() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     err = clEnqueueWriteBuffer(queue, o_buffer, CL_TRUE, 0, 3 * 256 * sizeof(unsigned int), (void *)histogram_results, 0, NULL, NULL);
     /*if(err != CL_SUCCESS){
         printf("clEnqueueWriteBuffer() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     err = clEnqueueWriteBuffer(queue, i_buffer, CL_TRUE, 0, input_size * sizeof(unsigned int), (void *)image, 0, NULL, NULL);
     /*if(err != CL_SUCCESS){
        printf("clEnqueueWriteBuffer() failed: %d\n", err);
        return EXIT_FAILURE;
     }*/

     err = read_kernel_from_file(filename, &source[0], &src_len[0]);
     /*if(err != 0){
         printf("read_kernel_from_file() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     program = clCreateProgramWithSource(context, 1, (const char **)source, (size_t *)src_len, &err);
     /*if(!program || err){
         printf("clCreateProgramWithSource() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/
     free(source[0]);

     err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
     /*if(err != CL_SUCCESS){
         printf("clBuildProgram() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     //histogram_kernel is the name of the kernel function
     histogram_kernel = clCreateKernel(program, "histogram_kernel", &err);
     /*if(!histogram_kernel || err){
         printf("clCreateKernel() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     clSetKernelArg(histogram_kernel, 0, sizeof(cl_mem), (void *)&i_buffer);
     clSetKernelArg(histogram_kernel, 1, sizeof(cl_mem), (void *)&o_buffer);
     clSetKernelArg(histogram_kernel, 2, sizeof(cl_int), (void *)&input_size);

     int dim = 1;
     //global_work_size needs to be multiple of local_work_size
     size_t global_work_size[1], local_work_size[1];
     global_work_size[0] = input_size/3; //number of work-items in total
     local_work_size[0] = (global_work_size[0]<1024 ? global_work_size[0] : 1024); //number of work-items in a work-group
     err = clEnqueueNDRangeKernel(queue, histogram_kernel, dim, 0, global_work_size, NULL, 0, NULL, NULL);
     /*if(err != CL_SUCCESS){
         printf("clEnqueueNDRangeKernel() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/

     err = clEnqueueReadBuffer(queue, o_buffer, CL_TRUE, 0, 3 * 256 * sizeof(unsigned int), (void *)histogram_results, NULL, NULL, NULL);
     /*if(err != CL_SUCCESS){
         printf("clEnqueueReadBuffer() failed: %d\n", err);
         return EXIT_FAILURE;
     }*/
 
     /*for(int j = 0; j < 768; j++){
         if(histogram_results[j] != 0)
         cout << "ref[" << j << "]:" << histogram_results[j] << endl;
     }*/
	
     for(i = 0; i < 256 * 3; ++i){ 
         //3 lines represent R,G,B histogram, respectively.
         //each line means value 0~255
         if(i%256 == 0 && i != 0){
             outFile << endl;
	     }
	     outFile << histogram_results[i] << ' ';
     }
        	
     clReleaseKernel(histogram_kernel);
     clReleaseProgram(program);
     clReleaseMemObject(i_buffer);
     clReleaseMemObject(o_buffer);
     clReleaseCommandQueue(queue);
     clReleaseContext(context);
	
     inFile.close();
     outFile.close();
        
     return 0;
}
