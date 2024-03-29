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

//find the smallest_rec and the index in the array
void merge(struct rec **smallest_rec, struct rec *sorting_recs, int num_process, int *index_smallest){
    for(int i=0; i < num_process; i++){
        if(i==0){
            *smallest_rec = &sorting_recs[i];
            *index_smallest = i;
        }
        else{
            if((*smallest_rec)->freq > sorting_recs[i].freq){
                *smallest_rec = &sorting_recs[i];
                *index_smallest = i;
            }
        }
    }
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

void free_fd(int **pipe_fd, int num_process){
    for(int i=0; i < num_process; i++){
        free(pipe_fd[i]);
    }
    free(pipe_fd);
}