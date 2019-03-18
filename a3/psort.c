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
    //incorrect number of command arguments
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

    //the size of input file and count the number of records
    int size_file = get_file_size(infile);
    int num_records = size_file / sizeof(struct rec);

    //divide records to num_processor parts and store the amount for each chunk in an array
    int *size_chunks = divide_records(num_process, num_records);

    int pipe_fd[num_process][2];
    for(int i=0; i<num_process; i++){
        //create pipe
        if ((pipe(pipe_fd[i]))== -1){
            perror("pipe");
            exit(1);
        }

        //create child processor to sort 
        int n = fork();
        if(n < 0){
            perror("fork");
            exit(1);
        }
        //child processor
        else if(n == 0){
            //count the position which need to offset in this child processor
            int num_offset = 0;
            for(int pos=0; pos < i; pos++){
                num_offset += size_chunks[pos];
            }

            //close the read in the child
            if(close(pipe_fd[i][0]) == -1){
                perror("closing read in child");
                exit(1);
            }

            //open the input file
            FILE *fp;
            if((fp = fopen(infile, "rb")) == NULL){
                perror("fopen");
                exit(1);
            }
            //locate the position in the file for this child 
            if(fseek(fp, num_offset*sizeof(struct rec), SEEK_SET) != 0){
                perror("fseek");
                exit(1);
            }
            //read size_chunks[i] records to the array
            struct rec records[size_chunks[i]];
            if(fread(records, sizeof(struct rec), size_chunks[i], fp) != size_chunks[i]){
                perror("fread");
                exit(1);
            }
            //sort in each child processor
            qsort(records,size_chunks[i], sizeof(struct rec), compare_freq);

            //write each sorted records to pipe
            for(int index=0; index < size_chunks[i]; index++){
                if(write(pipe_fd[i][1], records+index, sizeof(struct rec)) < 0){
                    perror("writing records to pipe in child");
                    exit(1);
                }
            }
            //close write in child after finishing writing the data to pipe
            if(close(pipe_fd[i][1]) == -1){
                perror("closing write in child");
                exit(1);
            }
            //close the input file pointer
            if(fclose(fp) != 0){
                perror("fclose");
                exit(1);
            }
            //exit the child processor
            exit(0);
        }
        //parent processor
        else{
            //close write pipe in parent processor
            if(close(pipe_fd[i][1]) == -1){
                perror("closing write in parent");
                exit(1);
            }
        }
    }
    //parent use wait to check whether the child exit normally
    for(int i=0; i<num_process; i++){
        pid_t pid;
        int status;
        if((pid = wait(&status)) == -1){
            perror("wait");
        }else if(!WIFEXITED(status)){
            fprintf(stderr, "Child terminated abnormally\n");
        }
    }

    //parent merge each chunks from the child processors
    int num_sorted = 0;
    //the current array which need to find the smallest
    struct rec sorting_recs[num_process];
    //the smallest record
    struct rec *smallest_rec = malloc(sizeof(struct rec *));
    //the position of smallest record in the array
    int index_smallest = 0;

    int result;

    //open the output file
    FILE *outfp;
    if((outfp = fopen(outfile, "w")) == NULL){
        perror("fopen");
        exit(1);
    }
    //start to find smallest record and add to the output file
    while(num_sorted < num_records){
        //the first time to merge
        if(num_sorted == 0){
            //read one record from each pipe and write the sorting array
            for(int j=0; j < num_process; j++){
                if(read(pipe_fd[j][0], &sorting_recs[j], sizeof(struct rec)) < 0){
                    perror("reading records from pipe when num_sorted is 0");
                    exit(1);
                }
            }
        }
        else{
            //there is no record left in pipe
            if((result = (read(pipe_fd[index_smallest][0], &sorting_recs[index_smallest], sizeof(struct rec)))) == 0){
                sorting_recs[index_smallest].freq = UPPER + 1;
            }
            else if(result < 0){
                perror("reading records from pipe");
                exit(1);
            }
        }
        //find the smallest_rec and the index in the array
        merge(smallest_rec, sorting_recs, num_process, outfile, &index_smallest);
        //write the smallest record to the output file
        if(fwrite(smallest_rec, sizeof(struct rec), 1, outfp) != 1){
            perror("fwrite");
            exit(1);
        }
        num_sorted++;
    }

    //close the output file
    if(fclose(outfp) != 0){
        perror("fclose");
        exit(1);
    }
    //close the read pipe in parent
    for(int i=0; i<num_process; i++){
        if(close(pipe_fd[i][0]) == -1){
            perror("closing read in parent");
            exit(1);
        }
    }
    free(smallest_rec);
    free(size_chunks);
}