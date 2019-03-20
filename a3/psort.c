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

    //check the error for input file and num_processor
    if(num_process <= 0){
        fprintf(stderr, "The process should not be zero or negative\n");
        exit(1);
    }
    int num_remind = size_file % sizeof(struct rec);
    if(num_remind != 0){
        fprintf(stderr, "Invalid input file\n");
        exit(1);
    }

    //divide records to num_processor parts and store the amount for each chunk in an array
    int *size_chunks = malloc(sizeof(int)*num_process);
    if(size_chunks == NULL){
        perror("malloc for chunks");
        exit(1);
    }
    int init_size_chunks = num_records / num_process;
    int reminder = num_records % num_process;
    for(int i=0; i < num_process; i++){
        size_chunks[i] = init_size_chunks;
        if(reminder != 0){
            size_chunks[i] += 1;
            reminder--;
        }
    }

    int **pipe_fd = malloc(sizeof(int *) * num_process);
    if(pipe_fd == NULL){
        perror("malloc for pipe");
        exit(1);
    }
    for(int i = 0; i< num_process; i++){
        if((pipe_fd[i] = malloc(sizeof(int) * 2)) == NULL){
            perror("malloc for pipe for each child");
            exit(1);
        }
    }
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
        //child process
        else if(n == 0){
            //count the position which need to offset in this child processor
            int num_offset = 0;
            for(int pos=0; pos < i; pos++){
                num_offset += size_chunks[pos];
            }
            int child_no;
            //close the read in the child
            for(child_no = 0; child_no <= i; child_no++){
                if(close(pipe_fd[child_no][0]) == -1){
                    perror("closing read in child");
                    exit(1);
                }
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
            struct rec *records = malloc(sizeof(struct rec) * size_chunks[i]);
            if(records == NULL){
                perror("malloc for records in each child");
                exit(1);
            }
            for(int index = 0; index < size_chunks[i]; index ++){
                if (fread(&(records[index]), sizeof(struct rec), 1, fp) == -1){
                    perror("fread");
                    exit(1);
                }
            }
            
            //sort in each child processor
            qsort(records, size_chunks[i], sizeof(struct rec), compare_freq);

            //write each sorted records to pipe
            for (int index = 0; index < size_chunks[i]; index++){
                if (write(pipe_fd[i][1], &(records[index]), sizeof(struct rec)) != sizeof(struct rec)){
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
            free_fd(pipe_fd, num_process);
            free(records);
            free(size_chunks);
            //exit the child processor
            exit(0);
        }
        //parent process
        else{
            //close write pipe in parent processor
            if(close(pipe_fd[i][1]) == -1){
                perror("closing write in parent");
                exit(1);
            }
        }
    }

    //parent merge each chunks from the child processors
    int num_sorted = 0;
    int num_left[num_process];
    for(int i = 0; i< num_process; i++){
        num_left[i] = size_chunks[i];
    }
    //the current array which need to find the smallest
    struct rec *sorting_recs = malloc(sizeof(struct rec) * num_process);
    if(sorting_recs == NULL){
        perror("malloc for sorting_rec for each child");
        exit(1);
    }
    //the smallest record
    struct rec **smallest_rec = malloc(sizeof(struct rec *));
    //the position of smallest record in the array
    int index_smallest = 0;

    int result;

    //open the output file
    FILE *outfp;
    if((outfp = fopen(outfile, "w")) == NULL){
        perror("fopen");
        exit(1);
    }

    //the first time to merge
    //read one record from each pipe and write the sorting array
    for(int j=0; j < num_process; j++){
        if(num_left[j] == 0){
            sorting_recs[j].freq = UPPER + 1;
        }
        else if(read(pipe_fd[j][0], &sorting_recs[j], sizeof(struct rec)) < 0){
            perror("reading records from pipe when num_sorted is 0");
            exit(1);
        }
            num_left[j]--;
    }
    //start to find smallest record and add to the output file
    while(num_sorted < num_records){
        //find the smallest_rec and the index in the array
        merge(smallest_rec, sorting_recs, num_process, &index_smallest);

        //write the smallest record to the output file
        if(fwrite(*smallest_rec, sizeof(struct rec), 1, outfp) != 1){
            perror("fwrite");
            exit(1);
        }
        num_sorted++;

        //there is no record left in pipe
        if(num_left[index_smallest] <= 0){
            sorting_recs[index_smallest].freq = UPPER + 1;
        }
        else{
            result = (read(pipe_fd[index_smallest][0], &sorting_recs[index_smallest], sizeof(struct rec)));
            if(result < 0){
                perror("reading records from pipe");
                exit(1);
            }
        }
        num_left[index_smallest]--;
    }

    //close the output file
    if(fclose(outfp) != 0){
        perror("fclose");
        exit(1);
    }
    //close the read pipe in parent
    for(int i=0; i<num_process; i++){
        //parent use wait to check whether the child exit normally
            pid_t pid;
            int status;
            if((pid = wait(&status)) == -1){
                perror("wait");
                exit(1);
            }else if(!WIFEXITED(status)){
                fprintf(stderr, "Child terminated abnormally\n");
            }
        if(close(pipe_fd[i][0]) == -1){
            perror("closing read in parent");
            exit(1);
        }
    }
    free_fd(pipe_fd, num_process);
    free(smallest_rec);
    free(sorting_recs);
    free(size_chunks);
    return 0;
}