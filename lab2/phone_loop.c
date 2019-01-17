#include <stdio.h>

int main(){
    char phone[11];
    int number;
    while(scanf("%s%d", phone, &number) != EOF){
        if(number >= -1 && number <= 9){
            if(number == -1){
                printf("%s", phone);
            }
            else{
            printf("%c", phone[number]);
            }
        }
        else{
        printf("ERROR");
        }
    }
    return 0;
}