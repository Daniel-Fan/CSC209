#include <stdio.h>

int main(){
    char phone[11];
    int number;
    scanf("%s%d", phone, &number);
    if(number >= -1 && number <= 9){
        if(number == -1){
        printf("%s", phone);
        }
        else{
            printf("%c", phone[number]);
        }
        return 0;
    }
    else{
        printf("ERROR");
        return 1;
    }
}