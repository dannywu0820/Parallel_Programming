#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<fstream>
#include<CL/cl.h>

using namespace std;

int main(int argc, char *argv[]){
    unsigned int *image, *histogram_results; //input and output
    unsigned int i = 0, a, input_size;
    fstream inFile("input", ios_base::in);
    ofstream outFile("0116233.out", ios_base::out);

    inFile >> input_size;
    image = new unsigned int[input_size];
    while(inFile >> a){
        image[i++] = a;
    }
    histogram_results = new unsigned int[3*256];
    for(int j = 0; j < 3*256; j++) histogram_results[j] = 0;
    
    cl_int err; //error return value of OpenCL functions
    cl_platform_id platform;
    cl_device_id device;
    cl_device_type device_type = CL_DEVICE_TYPE_GPU;
    cl_context context;
    cl_command_queue queue;
    cl_mem in_buf, out_buf; //input and output buffer
    cl_program program;
    cl_kernel histogram_kernel;
    const char *source = "\
    __kernel void histogram_kernel(\
                  __global unsigned int *img_data,\
                  __global unsigned int *histogram,\
                  int input_size)\
    {\
        int group_size = get_local_size(0);\
        int group_id = get_group_id(0);\
        int local_id = get_local_id(0);\
        int global_id = get_global_id(0);\
        int histogram_id;\
        \
        for(int j = 0; j < 3; j++){\
            histogram_id = img_data[global_id*3+j] + j*256;\
            atomic_inc(&histogram[histogram_id]);\
        }\
    }\
    ";
    size_t src_len = strlen(source);

    //Initialization
    err = clGetPlatformIDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, device_type, 1, &device, NULL);
    context = clCreateContext(0, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device, 0, &err);
    
    //Allocate global memory on GPU
    in_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, input_size * sizeof(unsigned int), NULL, &err);
    out_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 3*256 * sizeof(unsigned int), NULL, &err);
    
    //Transfer data from CPU to GPU
    err = clEnqueueWriteBuffer(queue, in_buf, CL_TRUE, 0, input_size * sizeof(unsigned int), (void *)image, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(queue, out_buf, CL_TRUE, 0, 3*256 * sizeof(unsigned int), (void *)histogram_results, 0, NULL, NULL);

    //Create and build the program
    program = clCreateProgramWithSource(context, 1, (const char **)&source, (size_t *)&src_len, &err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    //Create the kernel function, set arguments and execute
    histogram_kernel = clCreateKernel(program, "histogram_kernel", &err);
    clSetKernelArg(histogram_kernel, 0, sizeof(cl_mem), (void *)&in_buf);
    clSetKernelArg(histogram_kernel, 1, sizeof(cl_mem), (void *)&out_buf);
    clSetKernelArg(histogram_kernel, 2, sizeof(cl_int), (void *)&input_size);

    int dim = 1;
    size_t global_work_size[1], local_work_size[1];
    global_work_size[0] = input_size/3; //# of work-items in total
    local_work_size[0] = 1; //# of work-items in a work-group
    err = clEnqueueNDRangeKernel(queue, histogram_kernel, dim, 0, global_work_size, NULL, 0, NULL, NULL);

   //Transfer resulting data back to CPU
   err = clEnqueueReadBuffer(queue, out_buf, CL_TRUE, 0, 3*256 * sizeof(unsigned int), (void *)histogram_results, NULL, NULL, NULL);

   //Release resources
   clReleaseKernel(histogram_kernel);
   clReleaseProgram(program);
   clReleaseMemObject(in_buf);
   clReleaseMemObject(out_buf);
   clReleaseCommandQueue(queue);
   clReleaseContext(context);

   for(i = 0; i < 256*3; ++i){
       if(i%256 == 0 && i != 0){
           outFile << endl;
       }
       outFile << histogram_results[i] << ' ';
   }

   inFile.close();
   outFile.close();

   return 0;
}
