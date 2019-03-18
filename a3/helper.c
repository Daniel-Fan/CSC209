#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "helper.h"


int get_file_size(char *filename) {
    struct stat sbuf;

    if ((stat(filename, &sbuf)) == -1) {
       perror("stat");
       exit(1);
    }

    return sbuf.st_size;
}

//divide the input file to n parts for n child processors
int *divide_records(int num_chunks, int num_records){
    //malloc for chunks
    int *chunks_records;
    chunks_records = malloc(sizeof(int)*num_chunks);
    //find the size of each chunks
    int size_chunks = num_records / num_chunks;
    int reminder = num_records % num_chunks;
    for(int i=0; i < num_chunks; i++){
        chunks_records[i] = size_chunks;
        if(reminder != 0){
            chunks_records[i] += 1;
            reminder--;
        }
    }
    return chunks_records;
}

/* A comparison function to use for qsort */
int compare_freq(const void *rec1, const void *rec2) {

    struct rec *r1 = (struct rec *) rec1;
    struct rec *r2 = (struct rec *) rec2;

    if (r1->freq == r2->freq) {
        return 0;
    } else if (r1->freq > r2->freq) {
        return 1;
    } else {
        return -1;
    }
}

//merge each chunks from child processors
void merge(struct rec *smallest_rec, struct rec *source, int size, char *outfile, int *index){
    //find the smallest record in the source array
    for(int i=0; i < size; i++){
        if(i==0){
            *smallest_rec = source[i];
            *index = i;
        }
        else{
            if(smallest_rec->freq > source[i].freq){
                *smallest_rec = source[i];
                *index = i;
            }
        }
    }
}