#ifndef __DATA_H__
#define __DATA_H__

#include <sys/time.h>
typedef struct _DATA{
    struct timeval start;
    struct timeval stop;
    
    unsigned int     client_total; /*total clients*/
    unsigned int     client_avail; /*available clients*/
    unsigned long    conn_elapsed; /*time elapsed for connection*/
    
    unsigned int     nread_total;
    unsigned int     nread_failed;
    unsigned long    readbytes;
    unsigned long    read_elapsed;
    
    unsigned int     nwrite_total;
    unsigned int     nwrite_failed;
    unsigned long    writebytes;
    unsigned long    write_elapsed;
}DATA;

DATA      *new_data();
void      destroy_data(DATA *data);

void      data_increment_client_total(DATA *data);
void      data_increment_client_avail(DATA *data,int avail);
void      data_increment_conn_elapsed(DATA *data, unsigned long conn_elapsed);

void      data_increment_read_total(DATA *data, unsigned int read_total);
void      data_increment_read_failed(DATA *data,unsigned int read_failed);
void      data_increment_read_bytes(DATA *data, unsigned long  readbytes);
void      data_increment_read_elapsed(DATA *data, unsigned long read_elapsed);

void      data_increment_write_total(DATA *data,unsigned int write_total);
void      data_increment_write_failed(DATA *data, unsigned int write_failed);
void      data_increment_write_bytes(DATA *data, unsigned long writebytes);
void      data_increment_write_elapsed(DATA *data, unsigned long write_elapsed);

void      data_set_time_start(DATA *data);
void      data_set_time_stop(DATA *data);
void      data_output(DATA *data);

#endif
