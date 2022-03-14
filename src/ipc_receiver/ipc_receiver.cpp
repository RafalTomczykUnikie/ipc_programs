#include <unistd.h>
#include <stdio.h>


int main(int argc, char* argv[])
{
    while(1)
    {
        printf("ipc_receiver is alive!\r\n");

        sleep(1);        
    }
}