#include <unistd.h>
#include <stdio.h>
#include "threadpool.h"

void process(void *args)
{
    printf("hello world\n");
}


int main(int argc, char *argv[])
{
    CThreadPool threadpool(1,1);    
    threadpool.register_task(process,NULL);
    
    while(1){
        sleep(2);  
    }
    return 0;
}
