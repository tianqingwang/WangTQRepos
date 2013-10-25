#ifndef __CLIENT_H__
#define __CLIENT_H__

#define MAXREPS    (16777216)

typedef struct {
    int index;
    char **buf;
}LINES;

typedef struct {
    /*general info*/
    unsigned int     id;
    unsigned int     repeats;
    unsigned int     time;
    LINES            *lines;
   
    /*connection*/
    unsigned int     connection;  /*0:failed; 1:successful*/
    unsigned long    conn_elapsed;

    /*read*/
    unsigned int     nread_total; /*how many times it read*/
    unsigned int     nread_fail;  /*how many times it read failure.*/
    unsigned long    readbytes;   /*how many bytes it read*/
    unsigned long    read_elapsed; /*how much time it consumed.*/

    /*write*/
    unsigned int     nwrite_total;/*how many times it write*/
    unsigned int     nwrite_fail; /*how many times it write failure*/
    unsigned long    writebytes;  /*how many bytes it write*/
    unsigned long    write_elapsed;  /*how much time it consumed.*/

}CLIENT;


void *start_process(void *arg);

#endif
