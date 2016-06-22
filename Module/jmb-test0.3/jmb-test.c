
#define VERSION						"0.3"
#define DEFAULT_NR_LOOPS			10
#define DEFAULT_BANK_SIZE			671088640	//64*10*1024*1024

#define INTERNAL_NR_LOOPS			100
#define MAX_TESTS					2

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

extern int ReaderSSE2_64bytes(void *ptr, unsigned long, unsigned long);
extern int WriterSSE2_64bytes(void *ptr, unsigned long, unsigned long);

unsigned char a[DEFAULT_BANK_SIZE];
unsigned char t = 0;

void usage()
{
    printf("\nJSC memory bandwidth test (JMB v%s)\n", VERSION);
    printf("Usage: jmb-test [options]\n");
    printf("Options:\n");
    printf("	-n: number of runs per test (default: %d)\n", DEFAULT_NR_LOOPS);
    printf("	-t0: read test\n");
    printf("	-t1: write test\n");
    printf("	-c: cache size in MiB (must set)\n");
    printf("\nThe default is to run all tests available.\n\n");
}


void printout(double te, double mt, int type)
{
	switch(type)
	{
		case 0:
			printf("Method: MEM READ\t");
			break;
		case 1:
			printf("Method: MEM WRITE\t");
			break;
		default:
			printf("Method: ERROR\t");
	}

	printf("Elapsed: %.2f\t", te);
	printf("MiB: %.2f\t", mt);
	printf("Bandwidth: %.2f MiB/s\n", mt/te);
}

double do_read()
{
	struct timeval starttime, endtime;
	double te,mt;
	
	//unsigned char *a = malloc(DEFAULT_BANK_SIZE);
	memset(a,1,DEFAULT_BANK_SIZE);

	gettimeofday(&starttime,NULL);

	ReaderSSE2_64bytes(a,DEFAULT_BANK_SIZE,INTERNAL_NR_LOOPS);

	gettimeofday(&endtime,NULL);
	//free(a);
	te = ((double)(endtime.tv_sec*1000000-starttime.tv_sec*1000000+endtime.tv_usec-starttime.tv_usec))/1000000;
	mt = (double)INTERNAL_NR_LOOPS*DEFAULT_BANK_SIZE/1024/1024.0;

	printout(te,mt,0);

	return mt/te;
}

double do_write(unsigned int cache_size)
{
	struct timeval starttime, endtime;
	double te,mt;
	
	//unsigned char *a = malloc(DEFAULT_BANK_SIZE);
	memset(a,1,DEFAULT_BANK_SIZE);

	gettimeofday(&starttime,NULL);

	WriterSSE2_64bytes(a,DEFAULT_BANK_SIZE,INTERNAL_NR_LOOPS);

	gettimeofday(&endtime,NULL);
	//free(a);
	te = ((double)(endtime.tv_sec*1000000-starttime.tv_sec*1000000+endtime.tv_usec-starttime.tv_usec))/1000000;
	mt = (double)2*INTERNAL_NR_LOOPS*DEFAULT_BANK_SIZE/1024/1024.0 - cache_size;

	printout(te,mt,1);
	
	return mt/te;

}

//void* rd_thread(

int main(int argc,char **argv)
{
	int o;/* getopt options */
	int i;

	/* how many runs */
	int nr_loops = DEFAULT_NR_LOOPS;
	int tests[MAX_TESTS];
	double test_result[MAX_TESTS];
	int test_type;
	int test_g = 0;
	int cache_size = 0;

	for(i=0;i<MAX_TESTS;i++)
	{
		tests[i] = 0;
		test_result[i] = 0;
	}

	while((o=getopt(argc,argv,"hn:t:c:")) != EOF)
	{
		switch(o)
		{
			case 'h':
				usage();
				exit(1);
				break;
			case 'n': /* no. loops */
				nr_loops = strtoul(optarg, (char **)NULL, 10);
				break;
			case 't': /* type to run */
				test_type = strtoul(optarg, (char **)NULL, 10);
				if(test_type>=MAX_TESTS)
				{
					printf("Error: test number must be between 0 and %d\n",MAX_TESTS);
					exit(1);
				}
				tests[test_type] = 1;
				break;
			case 'c': /* cache size in Mi Bytes */
				cache_size = strtoul(optarg, (char **)NULL, 10);
				break;
			default:
				break;
		}
	}

	if(cache_size == 0)
	{
		printf("Error: cache size must be set\n");
		exit(1);
	}

	printf("\nJSC memory bandwidth test (JMB v%s)\n",VERSION);
	printf("* Loops(-n):      %d\n",nr_loops);
	printf("* Cache size(-c): %d MiB\n",cache_size);

	for(i=0;i<MAX_TESTS;i++)
	{
		test_g += tests[i];
	}

	if(test_g == 0)
	{
		printf("Warning, not set test case type, simulate all test case in default.\n");
		for(i=0;i<MAX_TESTS;i++)
		{
			tests[i] = 1;
		}
	}
	else
	{
		printf("Note, simulation test %d type case(s).\n",test_g);
	}

	

	printf("\n[Testing...]\n");

	if(tests[0] == 1)
	{
		for(i=0;i<nr_loops;i++)
		{
			test_result[0] += do_read();
		}
	}
	
	if(tests[1] == 1)
	{
		for(i=0;i<nr_loops;i++)
		{
			test_result[1] += do_write(cache_size);
		}
	}

	printf("\n[Summary]\n");
		
	if(tests[0] == 1)
	{
		printf("* Read Bandwidth (av.):  %.3f MiB/s\n",test_result[0]/nr_loops);
	}
	if(tests[1] == 1)
	{
		printf("* Write Bandwidth (av.): %.3f MiB/s\n",test_result[1]/nr_loops);
	}

	return 0;
}

