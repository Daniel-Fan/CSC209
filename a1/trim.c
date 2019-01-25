#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Reads a trace file produced by valgrind and an address marker file produced
 * by the program being traced. Outputs only the memory reference lines in
 * between the two markers
 */

int main(int argc, char **argv) {
    
    if(argc != 3) {
         fprintf(stderr, "Usage: %s tracefile markerfile\n", argv[0]);
         exit(1);
    }
    FILE *output_fp;
    FILE *marker_fp;
    output_fp = fopen(argv[1], "r");
    marker_fp = fopen(argv[2], "r");
    // Addresses should be stored in unsigned long variables
    // unsigned long start_marker, end_marker;
    unsigned long start_marker;
    unsigned long end_marker;
    char reference;
    unsigned long address;
    int size;
    int is_end = 0;
    int is_start = 0;
    fscanf(marker_fp, "%lx %lx", &start_marker, &end_marker);
    while(fscanf(output_fp, " %c %lx,%d", &reference, &address, &size) != EOF && !is_end){
        if(is_start == 1 && address != end_marker){
            printf("%c,%#lx\n", reference, address);
        }
        if(address == start_marker){
            is_start = 1;
        }
        if(address == end_marker){
            is_end = 1;
        }
    }
    fclose(output_fp);
    fclose(marker_fp);
    return 0;
}
