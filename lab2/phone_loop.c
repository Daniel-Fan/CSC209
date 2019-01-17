#include <stdio.h>

int main(){
    char phone[11];
    int number;
    int return_value = 0;
    scanf("%s", phone);
    while(scanf("%d", &number) != EOF){
        if(number >= -1 && number <= 9){
            if(number == -1){
                printf("%s\n", phone);
            }
            else{
            printf("%c\n", phone[number]);
            }
        }
        else{
        printf("ERROR\n");
        return_value = 1;
        }
    }
    return return_value;
}