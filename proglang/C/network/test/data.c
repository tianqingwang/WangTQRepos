#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/times.h>
#include <sys/time.h>
#include "data.h"

DATA *new_data()
{
	DATA *data;
	data = malloc(sizeof(DATA));
    return data;
}

void destroy_data(DATA *data)
{
	free(data);
	return;
}

void data_increment_client_total(DATA *data)
{
    data->client_total ++;
}

void data_increment_client_avail(DATA *data,int avail)
{
    data->client_avail += avail;
}

//void data_increment_conn_elapsed(DATA *data,unsigned long start, unsigned long stop)
void data_increment_conn_elapsed(DATA *data, unsigned long conn_elapsed)
{
    //data->conn_elapsed += (stop - start);
    data->conn_elapsed += conn_elapsed;
}

void data_increment_read_total(DATA *data, unsigned int read_total)
{
    data->nread_total += read_total;
}

void data_increment_read_failed(DATA *data,unsigned int read_failed)
{
    data->nread_failed += read_failed;
}

void data_increment_read_bytes(DATA *data, unsigned long  readbytes)
{
    data->readbytes += readbytes;
}

//void data_increment_read_elapsed(DATA *data, unsigned long start, unsigned long stop)
void data_increment_read_elapsed(DATA *data, unsigned long read_elapsed)
{
    //data->read_elapsed += (stop - start);
    data->read_elapsed += read_elapsed;
}

void data_increment_write_total(DATA *data,unsigned int write_total)
{
    data->nwrite_total += write_total;
}

void data_increment_write_failed(DATA *data, unsigned int write_failed)
{
    data->nwrite_failed += write_failed;
}

void data_increment_write_bytes(DATA *data, unsigned long writebytes)
{
    data->writebytes += writebytes;
}

//void data_increment_write_elapsed(DATA *data, unsigned long start, unsigned long stop)
void data_increment_write_elapsed(DATA *data, unsigned long write_elapsed)
{
    //data->write_elapsed += (stop - start);
    data->write_elapsed += write_elapsed;
}


void data_set_time_start(DATA *data)
{
    gettimeofday(&data->start,NULL);
}

void data_set_time_stop(DATA *data)
{
    gettimeofday(&data->stop,NULL);
}

void data_output(DATA *data)
{
	double usetime;
    usetime = (data->stop.tv_sec - data->start.tv_sec) + (data->stop.tv_usec - data->start.tv_usec)/1000000;

    fprintf(stdout,"total elapsed time :     %f\n",usetime);
    fprintf(stdout,"total clients      :     %d\n",data->client_total);
    fprintf(stdout,"available clients  :     %d\n",data->client_avail);
    fprintf(stdout,"connection elapsed :     %ld usecs\n\n",data->conn_elapsed);
    fprintf(stdout,"total read times   :     %d\n",data->nread_total);
    fprintf(stdout,"failed read times  :     %d\n",data->nread_failed);
    fprintf(stdout,"total read bytes   :     %ld\n",data->readbytes);
    fprintf(stdout,"read elapsed time  :     %ld usecs\n\n",data->read_elapsed);
    fprintf(stdout,"total write times  :     %d\n",data->nwrite_total);
    fprintf(stdout,"failed write times :     %d\n",data->nwrite_failed);
    fprintf(stdout,"total write bytes  :     %ld\n",data->writebytes);
    fprintf(stdout,"write elapsed time :     %ld usecs\n",data->write_elapsed);
}
