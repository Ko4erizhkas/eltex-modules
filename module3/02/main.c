#include <stdio.h>
#include <unistd.h>
void broker() {return;}
void publisher() {return;}
void subscriber() {return;}
int main(int argc, char* argv[])
{
    int opt = 0;
    while ((opt = getopt(argc, argv, "bps: ")) != 1)
    {
        switch(opt)
        {
            case 'b':
            {
                printf("broker\n");
                broker(); 
                break;
            }
            case 'p':
            {
                printf("publisher\n");
                publisher(); 
                break;
            }
            case 's':
            { 
                printf("subscriber");
                subscriber();
                break;
            }
        }
    }
    return 0;
}