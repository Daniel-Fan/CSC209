#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include<sys/wait.h> 
#include "helper.h"
#define UPPER 30000

int main(int argc, char *argv[]){
    extern char *optarg;
    int ch,num_process;
    char *infile = NULL, *outfile = NULL;

    if (argc != 7) {
        fprintf(stderr, "Usage: psort -n <number of processes> -f <inputfile> -o <outputfile>\n");
        exit(1);
    }

    /* read in arguments */
    while ((ch = getopt(argc, argv, "n:f:o:")) != -1) {
        switch(ch) {
        case 'n':
            num_process = strtol(optarg, NULL, 10);
        case 'f':
            infile = optarg;
            break;
        case 'o':
            outfile = optarg;
            break;
        default:
            fprintf(stderr, "Usage: psort -n <number of processes> -f <inputfile> -o <outputfile>\n");
            exit(1);
        }
    }

    int size_file = get_file_size(infile);
    int num_records = size_file / sizeof(struct rec);
    int *size_chunks = divide_records(num_process, num_records);
    int pipe_fd[num_process][2];
    for(int i=0; i<num_process; i++){
        if ((pipe(pipe_fd[i]))== -1){
            perror("pipe");
            exit(1);
        }

        int n = fork();
        if(n < 0){
            perror("fork");
            exit(1);
        }
        else if(n==0){
            int num_offset = 0;
            for(int pos=0; pos < i; pos++){
                num_offset += size_chunks[pos];
            }
            if(close(pipe_fd[i][0]) == -1){
                perror("closing read in child");
                exit(1);
            }
            FILE *fp;
            if((fp = fopen(infile, "rb")) == NULL){
                perror("fopen");
                exit(1);
            }
            if(fseek(fp, num_offset*sizeof(struct rec), SEEK_SET) != 0){
                perror("fseek");
                exit(1);
            }
            struct rec records[size_chunks[i]];
            if(fread(records, sizeof(struct rec), size_chunks[i], fp) != size_chunks[i]){
                perror("fread");
                exit(1);
            }
            qsort(records,size_chunks[i], sizeof(struct rec), compare_freq);
            for(int index=0; index < size_chunks[i]; index++){
                if(write(pipe_fd[i][1], records+index, sizeof(struct rec)) < 0){
                    perror("writing records to pipe in child");
                    exit(1);
                }
            }
            if(close(pipe_fd[i][1]) == -1){
                perror("closing write in child");
                exit(1);
            }
            if(fclose(fp) != 0){
                perror("fclose");
                exit(1);
            }
            exit(0);
        }
        else{
            if(close(pipe_fd[i][1]) == -1){
                perror("closing write in parent");
                exit(1);
            }
        }
    }
    for(int i=0; i<num_process; i++){
        pid_t pid;
        int status;
        if((pid = wait(&status)) == -1){
            fprintf(stderr, "Child terminated abnormally\n");
        }
    }

    int num_sorted = 0;
    struct rec sorting_recs[num_process];
    int index_smallest;
    int result;
    while(num_sorted < num_records){
        if(num_sorted == 0){
            for(int j=0; j < num_process; j++){
                if(read(pipe_fd[j][0],&sorting_recs[j], sizeof(struct rec)) < 0){
                    perror("reading records from pipe when num_sorted is 0");
                    exit(1);
                }
            }
        }
        else{
            if((result = (read(pipe_fd[index_smallest][0], &sorting_recs[index_smallest], sizeof(struct rec)))) == 0){
                sorting_recs[index_smallest].freq = UPPER + 1;
            }
            else if(result < 0){
                perror("reading records from pipe");
                exit(1);
            }
        }
        index_smallest = merge(sorting_recs, num_process, outfile, num_sorted);
        num_sorted++;
    }
    for(int i=0; i<num_process; i++){
        if(close(pipe_fd[i][0]) == -1){
            perror("closing read in parent");
            exit(1);
        }
    }
    free(size_chunks);
}