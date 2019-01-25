#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_state(char *state, int size){
    for(int i = 0; i< size; i++){
        printf("%c", state[i]);
    }
    printf("\n");
}

void update_state(char *state, int size){
    char temp[size];
    for(int i = 0;i < size; i++){
        temp[i] = state[i];
    }
    for(int i = 1; i < (size -1); i++){
        if(temp[i-1] == temp[i+1]){
            state[i] = '.';
        }
        else if(temp[i-1] != temp[i+1]){
            state[i] = 'X';
        }
    }
}