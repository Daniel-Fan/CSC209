#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Constants that determine that address ranges of different memory regions

#define GLOBALS_START 0x400000
#define GLOBALS_END   0x700000
#define HEAP_START   0x4000000
#define HEAP_END     0x8000000
#define STACK_START 0xfff000000

int main(int argc, char **argv) {
    
    FILE *fp = NULL;

    if(argc == 1) {
        fp = stdin;

    } else if(argc == 2) {
        fp = fopen(argv[1], "r");
        if(fp == NULL){
            perror("fopen");
            exit(1);
        }
    } else {
        fprintf(stderr, "Usage: %s [tracefile]\n", argv[0]);
        exit(1);
    }
    /* Complete the implementation */
    char reference;
    unsigned long address;
    int num_Instr=0, num_Mod=0, num_Load=0, num_Store=0, global=0, heap=0, stack=0;
    while(fscanf(fp, "%c,%lx", &reference, &address) != EOF){
        if(reference == 'I'){
            num_Instr += 1;
        }else{
            if(reference == 'M'){
                num_Mod += 1;
            }else if(reference == 'S'){
                num_Store += 1;
            }else if(reference == 'L'){
                num_Load += 1;
            }
            if(address >= GLOBALS_START && address <= GLOBALS_END){
                global += 1;
            }else if(address >= HEAP_START && address <= HEAP_END){
                heap += 1;
            }else if(address >= STACK_START){
                stack += 1;
            }
        }
    }
    printf("Reference Counts by Type:\n");
    printf("    Instructions: %d\n", num_Instr);
    printf("    Modifications: %d\n", num_Mod);
    printf("    Loads: %d\n", num_Load);
    printf("    Stores: %d\n", num_Store);
    printf("Data Reference Counts by Location:\n");
    printf("    Globals: %d\n", global);
    printf("    Heap: %d\n", heap);
    printf("    Stack: %d\n", stack);
    return 0;
}
