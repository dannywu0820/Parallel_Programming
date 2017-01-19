#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<fstream>

using namespace std;

unsigned int *histogram(unsigned int *image_data, unsigned int _size){
	unsigned int *img = image_data;
	unsigned int *ref_histogram_results;
	unsigned int *ptr;
	
	ref_histogram_results = (unsigned int *)malloc(3 * 256 * sizeof(unsigned int));
	ptr = ref_histogram_results;
	memset(ref_histogram_results, 0x0, 3 * 256 * sizeof(unsigned int));
	
	//R0~255 is ptr[0~255]
	//G0~255 is ptr[256~511]
	//B0~255 is ptr[512~767]
	//histogram of R
	for(unsigned int i = 0; i < _size; i += 3){
		unsigned int index = img[i];
		ptr[index]++;
	}
	//histogram of G
	ptr += 256;
	for(unsigned int i = 1; i < _size; i += 3){
		unsigned int index = img[i];
		ptr[index]++;
	}
	//histogram of B
	ptr += 256;
	for(unsigned int i = 2; i < _size; i += 3){
		unsigned int index = img[i];
		ptr[index]++;
	}
	
	return ref_histogram_results;
}

int main(int argc, char const *argv[]){
	unsigned int *histogram_results;
	unsigned int i, a, input_size;
	fstream inFile("input", ios_base::in);
	ofstream outFile("answer.out", ios_base::out);
	
	inFile >> input_size; //first line of input file
	unsigned int *image = new unsigned int[input_size];
	while(inFile >> a){
		image[i++] = a;
	}
	//calculating R,G,B histogram can execute in parallel
	histogram_results = histogram(image, input_size);
	for(i = 0; i < 256 * 3; ++i){ 
	    //3 lines represent R,G,B histogram, respectively.
		//each line means value 0~255
		if(i % 256 == 0 && i != 0){
			outFile << endl;
		}
		outFile << histogram_results[i] << ' ';
	}
	
	inFile.close();
	outFile.close();
	
	return 0;
}
