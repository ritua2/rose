#include <stdio.h>
    int a17 = 1;
    int a7 = 0;
    int a20 = 1;
    int a8 = 15;
    int a12 = 8;
    int a16 = 5;
    int a21 = 1;
    int calculate_output(int input) {
        if((((a8==15)&&(((((a21==1)&&(((a16==5)||(a16==6))&&(input==1)))&&(a20==1))&&(a17==1))&&!(a7==1)))&&(a12==8))){
            a7 = 1;
            a21 = 0;
            a8 = 13;
        } else if(((a8==13)&&(!(a21==1)&&((((input==3)&&((((a20==1)&&!(a17==1))&&(a16==6))||((!(a20==1)&&(a17==1))&&(a16==4))))&&(a12==8))&&(a7==1))))){
            a16 = 6;
            a20 = 1;
            a17 = 0;
            return -1;
        } else if(((((a8==13)&&((((!(a21==1)&&(input==6))&&(a20==1))&&(a12==8))&&(a17==1)))&&(a7==1))&&(a16==5))){
            a16 = 4;
            a20 = 0;
        } else if(((a7==1)&&((a8==13)&&((a12==8)&&(!(a21==1)&&((input==2)&&((((a20==1)&&!(a17==1))&&(a16==6))||(((a17==1)&&!(a20==1))&&(a16==4))))))))){
            a16 = 4;
            a20 = 0;
            a17 = 1;
            return -1;
        } 
        if((((((((a17==1)&&(a7==1))&&!(a20==1))&&(a8==15))&&(a12==8))&&(a16==5))&&(a21==1))){
        } 
        return -2; 
    }
int main()
{
    int output = -1;
    while(1)
    {
        int input;
        scanf("%d", &input);        
        output = calculate_output(input);
        if(output == -2)
            printf("%d\n", output);
    }
}