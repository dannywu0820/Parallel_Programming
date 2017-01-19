#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<fstream>
#include<time.h>

using namespace std;

int main(int argc, char *argv[]){
    int input_size = atoi(argv[1]); //# of pixels * 3
    ofstream outFile("input", ios_base::out);

    srand(time(NULL));
    outFile << input_size << endl;
    for(int i = 0; i < input_size; i++){
        int num = rand()%256;
        outFile << num << " ";
        if(i%3 == 2) outFile << endl;
    }
    
}
