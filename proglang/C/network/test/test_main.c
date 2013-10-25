#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "data.h"
#include "client.h"
#include "workqueue.h"

#define MAX_LINE 64
#define MAX_SIZE 1024

struct param{
    unsigned int users;
    unsigned int repeats;
    unsigned int secs; 
    struct timeval time;
};

struct param myparam; 


static int read_file( char *filename,LINES *lines)
{
    int j;
	FILE *fp;
    
    char strLine[MAX_SIZE];
    if((fp = fopen(filename,"r")) == NULL){
		printf("cannot open the file.\n");
		exit(0);
	}
    
    memset(strLine,0,MAX_SIZE);
    while(fgets(strLine,MAX_SIZE,fp) != NULL){
        lines->buf = (char**)realloc(lines->buf,sizeof(char*)*(lines->index+1));
        lines->buf[lines->index] = (char*)strdup(strLine);
        lines->index++;
        memset(strLine,0,MAX_SIZE);
    }
    
    fclose(fp);
    
#if 0 /*for test*/
	for(j = 0; j < lines->index; j++){
		printf("%s\n",lines->buf[j]);
	}
#endif

    return lines->index;
}

static void parse_cmdline(int argc, char *argv[])
{
    int ch; 
	opterr = 0; 
	myparam.users = 0;
	myparam.repeats = MAXREPS;	
    
    if (argc < 2){
        printf("%s <-c> users [-r repeats] [-t seconds] [-h]\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    
    while ((ch = getopt(argc,argv,"c:r:t:h:"))!=-1)  
    {  
        switch(ch)  
            {  
                case 'c':  
                   // printf("option a:'%s'\n",optarg);  
					myparam.users = atoi(optarg);
                    if (myparam.users <= 0){
                        printf("%s <-c> users [-r repeats] [-t seconds] [-h]\n",argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;  
                case 'r':  
                    //printf("option r:'%s'\n",optarg);
					myparam.repeats = atoi(optarg);
                    break; 
				case 't':  
                    //printf("option t:'%s'\n",optarg);  
                    myparam.secs = atoi(optarg);
                    break;
				case 'h':  
                    //printf("option h:'%s'\n",optarg);  
                    break;
                default:
                    printf("%s <-c> users [-r repeats] [-t seconds] [-h]\n",argv[0]);
                    exit(EXIT_FAILURE);
            }
                
        }  
}  

static void signalhandler(int signum)
{
    if (signum == SIGALRM){
        workqueue_cancel();
    }
}

int main(int argc, char *argv[])
{	
    char filename[] = "test.txt";
    int  linecount = 0;
    
    LINES *lines = malloc(sizeof(LINES));
    lines->index = 0;
    lines->buf   = NULL;
    
    DATA *data = new_data();

    /*parse command argument.*/
    parse_cmdline(argc, argv);
    
    /*initialize thread pool*/
    workqueue_init(myparam.users);
    /*initialize timer*/
    if (myparam.secs > 0){
        signal(SIGALRM,signalhandler);
        alarm(myparam.secs);
    }
    
    linecount = read_file(filename,lines);
    
    CLIENT *client = malloc(sizeof(CLIENT)*myparam.users);
    if (client == NULL){
        fprintf(stderr,"malloc failed.\n");
        exit(EXIT_FAILURE);
    }

    /*time counting begins*/
    data_set_time_start(data);
    /*add callback function and argument to thread pool*/
	int i;
	for(i = 0; i < myparam.users; i++)
	{
        /*initialize client*/
        client[i].id           = i;
        client[i].repeats      = myparam.repeats;
        client[i].time         = myparam.secs;
        client[i].lines        = lines;
        
        client[i].connection   = 0;
        client[i].conn_elapsed = 0;
        client[i].nread_total  = 0;
        client[i].nread_fail   = 0;
        client[i].readbytes    = 0;
        client[i].read_elapsed = 0;
        client[i].nwrite_total = 0;
        client[i].nwrite_fail  = 0;
        client[i].writebytes   = 0;
        client[i].write_elapsed = 0;
        
		workqueue_add_job(start_process,&client[i]);	
	}
     
    workqueue_join();
    /*time counting ends*/
    data_set_time_stop(data);
    
    for (i=0; i<myparam.users; i++){
        data_increment_client_total(data);
        data_increment_client_avail(data,client[i].connection);
        //data_increment_conn_elapsed(data,client[i].conn_start,client[i].conn_stop);
        data_increment_conn_elapsed(data,client[i].conn_elapsed);
        
        data_increment_read_total(data,client[i].nread_total);
        data_increment_read_failed(data,client[i].nread_fail);
        data_increment_read_bytes(data,client[i].readbytes);
        //data_increment_read_elapsed(data,client[i].read_start,client[i].read_stop);
        data_increment_read_elapsed(data,client[i].read_elapsed);
        data_increment_write_total(data,client[i].nwrite_total);
        data_increment_write_failed(data,client[i].nwrite_fail);
        data_increment_write_bytes(data,client[i].writebytes);
        //data_increment_write_elapsed(data,client[i].write_start,client[i].write_stop);
        data_increment_write_elapsed(data,client[i].write_elapsed);
    }
    
    /*output result*/
    data_output(data);
    
    destroy_data(data);

    return 0;
}
