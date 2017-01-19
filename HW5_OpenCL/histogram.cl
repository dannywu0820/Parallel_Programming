__kernel void histogram_kernel(__global unsigned int *src_data, __global unsigned int *dest_data, int input_size){
    int group_size = get_local_size(0);
    int groupID = get_group_id(0);
    int localID = get_local_id(0);
    int globalID = groupID * group_size + localID;
    int histogramID;
    int i;
 
    
        histogramID = src_data[globalID*3];
        atomic_inc(&dest_data[histogramID]);
    

    
        histogramID = src_data[globalID*3+1]+256;
        atomic_inc(&dest_data[histogramID]);
    

   
        histogramID = src_data[globalID*3+2]+512;
        atomic_inc(&dest_data[histogramID]);
    
}
