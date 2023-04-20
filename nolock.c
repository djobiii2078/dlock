#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<errno.h>

#define WAITTIME 10
#define NBPAGES 100
#define NBTHREADSMAX 1024
#define NBTHREADS 100
#define NBOPS_C 512*NBPAGES*NBTHREADS
#define ERRORLEN 100

pthread_mutex_t lock_processed_ops; 

struct page{
	int array[512];
}page;

pthread_t thread_ids[NBTHREADSMAX];
long processed_ops; 
long nbops = 0; 

/*
* The no-lock variant of this benchmark
* We expect the program to move faster 
* but with a very low consistency at the end. 
*/

void * process(void *nb_ops)
{
	pthread_t thread_id = pthread_self();
	printf("Starting thread id-[%ld]\n",	thread_id);

	struct page *myPage = malloc(NBPAGES*sizeof(page));
	//initialize the array 
	 
	int counter = 0;
	int j = 0;

	for(counter=0; counter<NBPAGES; counter++)
	{
		for(j=0; j<512; j++)
		{
			myPage[counter].array[j]=0;
		}	
	}
	
	//update the global counter
	//with a lock here 
	for(counter = 0; counter < NBPAGES; counter++)
	{
		for(j=0; j<512; j++)
		{			
			srand(time(NULL));  
			myPage[counter].array[j]=rand()*j + rand()*j;
		/*	pthread_mutex_lock(&lock_processed_ops);*/
			processed_ops +=1; 
		/*	pthread_mutex_unlock(&lock_processed_ops);  */
			printf("thread id-[%ld]ops = %ld/%ld\n", thread_id, processed_ops, *((unsigned long *) nb_ops)); 
		}

	}


}


int main(int argc, char **argv)
{
	//Global variable : overall_counter
	int err; 
	int gc_ops=0;
	int nbthreads_counter=0;
	int threads_to_create=0;
	char error_msg[ERRORLEN];
	int error_num = 0;



	//le but est de créer plusieurs threads 
	//chaque threads réalise un traitement 
	//lise et mette à jour gc_ops en bout de course 
	//avant de continuer leur traitement

	if (pthread_mutex_init(&lock_processed_ops, NULL) != 0)
    {
        printf("\n mutex initialization failed\n");
        return 1;
    }

	threads_to_create = (argc > 1) ? atoi(argv[1]) : NBTHREADS;
	threads_to_create = (threads_to_create > NBTHREADSMAX) ? NBTHREADSMAX : threads_to_create; 

	nbops = (argc <= 1) ? NBOPS_C : threads_to_create*512*NBPAGES;
	printf("Waiting %dsec to catch events ...\n",WAITTIME);
	sleep(WAITTIME);

	printf("Starting program with %d threads and %ld expected ops\n",threads_to_create, nbops);

	while (nbthreads_counter<threads_to_create)
	{
		err = pthread_create(&(thread_ids[nbthreads_counter]),NULL, process,&nbops);
		if(err!=0)
		{
			error_num = strerror_r(err,error_msg,ERRORLEN);
			printf("\ncannott create thread.\n Reason :[%s].\n Exiting", error_msg);
			exit(0); 
		}
		
		nbthreads_counter++;
	}

	while(nbthreads_counter>1)
	{
		pthread_join(thread_ids[nbthreads_counter--],NULL); 
	}

	pthread_mutex_destroy(&lock_processed_ops);

	return 0; 


}

