/*LOL: optimal input is */
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include "barrier.h"

#define PI 3.14
#define OUTPUT

int thread_num;
int steps;
int elements;
int length = 1;
double a = 1;
double d_x;
double d_t;
double d_Z = 0;
double **Z;

pthread_barrier_t barr;

double get_initial_Z(double x) {	//A string configuration at t=0
	return sin(PI*x/length);
}

void set_initial_conditions() { //Initialising start conditions
	int i, j;
	for(i = 0; i < (steps + 1); ++i) {	//No movement of string edges
		Z[i][0] = 0;
		Z[i][elements] = 0;
	}
	for(j = 0; j < (elements + 1); ++j)	{	
		Z[0][j] = get_initial_Z(d_x * j);	//Initial string configuration
		Z[1][j] = Z[0][j] + d_Z * (d_t * d_t);	//because Z'(x, 0) = 0
	}
}

double f(double x, double t) {
	return 0;
}

void* node_calc(void *arg_p) {	//processing steps of FDM
	int i, j;
	int *border = (int*)arg_p;
	//printf("[%d:%d]\n", border[0], border[1]);	//DEBUG!
	for(i = 2; i < (steps + 1); ++i) {
		for(j = border[0]; j <= border[1]; ++j) {
		    Z[i][j] = a*a * ((d_t*d_t) / (d_x*d_x)) * (Z[i-1][j+1] - 2*Z[i-1][j] + Z[i-1][j-1]) + 2*Z[i-1][j] - Z[i-2][j] + f(d_x * j, d_t * i);
		}
		pthread_barrier_wait(&barr);
	}
}

void print() {	//Print curves to files "data/curveXX.dat" and create a script for gnuplot
	int i, j;
	FILE *params;
	FILE *cmd_script;
	char filename[32];

	cmd_script = fopen("cmd.script", "w");

	printf("Writing output data to files (use \'gnuplot \"cmd.script\"\')\n");
	for(i = 0; i < (steps + 1); ++i) {
		sprintf(filename, "data/curve%d.dat", i);
		params = fopen(filename, "w");
		for(j = 0; j < (elements + 1); ++j)	{
			fprintf(params, "%.2f\t%.2f\n", j * d_x, Z[i][j]);
		}
		fclose(params);
		fprintf(cmd_script, "plot [0:1] [-1:1] \"%s\" notitle smooth csplines\n", filename);
		fprintf(cmd_script, "pause %f\n", d_t);
	    //fprintf(cmd_script, "pause 0.5\n");	
	}
	//fprintf(cmd_script, "unset multiplot\n");
	fprintf(cmd_script, "pause mouse \"Click any mouse button to close.\"\n");
	fclose(cmd_script);
	printf("Ready.\n");
}

int main(int argc, char *argv[])	{
	pthread_t *tid;
	pthread_attr_t pattr;
	int ret;
	int i, j;
	int **border;
	char error[100];
	struct timeval start, end;

	if(argc < 4)	{
		perror("Too few arguments! Usage: ./threadlab <thread_num> <time_steps> <elements>.\n");
		exit(1);
	}
	sscanf(argv[1], "%d", &thread_num);
	sscanf(argv[2], "%d", &steps);
	sscanf(argv[3], "%d", &elements);
	tid = (pthread_t*)malloc((thread_num - 1) * sizeof(pthread_t));
	Z = (double**)malloc((steps + 1) * sizeof(double*));
	for(i = 0; i < (steps + 1); ++i)
		Z[i] = (double*)malloc((elements + 1) * sizeof(double));
	d_x = (double)length/(double)elements;
    	d_t = (d_x*d_x) / (2 * a*a);
	border = (int**)malloc(thread_num * sizeof(int*));
	for(i = 0; i < thread_num; ++i)
		border[i] = (int*)malloc(2 * sizeof(int));

	pthread_attr_init(&pattr);
	pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);
	pthread_barrier_init(&barr, NULL, thread_num);

	printf("Processing FDM with d_x=%f, d_t=%f.\n", d_x, d_t);
  	set_initial_conditions();
	//printf("d_x=%f, d_t=%.32f\n", d_x, d_t);	//DEBUG!
	for(i = 0; i < thread_num; ++i) {
		if((elements - 1) % thread_num == 0)	{
			border[i][0] = (elements - 1)/thread_num * i + 1;
			border[i][1] = (elements - 1)/thread_num * (i + 1);
		}
		else {
			border[i][0] = (elements - 1)/thread_num * i + 1;
			if(i != (thread_num - 1))
				border[i][1] = (elements - 1)/thread_num * (i + 1);
			else
				border[i][1] = elements - 1;		
		}
	}
	gettimeofday(&start, 0);
	for(i = 0; i < thread_num; ++i)	{
	    	if(ret = pthread_create(&tid[i], &pattr, node_calc, (void*)border[i]))	{
		    sprintf(error, "Error while creating a thread %d", i);
		    perror(error);
		    exit(1);
		}
  	}
  	for(i = 0; i < thread_num; ++i)	{
	    pthread_join(tid[i], NULL);
	    //printf("Thread %d ended\n", i);	//DEBUG!
	}
	gettimeofday(&end, 0);
	printf("Ready.\n");
	printf("Calculation pure time: %d sec %d usec.\n", end.tv_sec - start.tv_sec, end.tv_usec - start.tv_usec);
#ifdef OUTPUT
	print();
#endif
  
	pthread_exit(0);
}