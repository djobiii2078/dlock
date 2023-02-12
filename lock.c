#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>


#define NBPAGES 1024
#define NBTHREADS 100

pthread_mutex_t lock_processed_ops; 

struct page{
	int array[512];
}page;

pthread_t thread_ids[NBTHREADS];
int processed_ops; 

int process(void *p)
{
	struct page *myPage = malloc(1024*8*sizeof(array[512]));
	//initialize the array 
	int nbfinal_ops = (int)p; 
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
			pthread_mutex_lock(&lock_processed_ops);
			processed_ops +=1; 
			pthread_mutex_unlock(&lock_processed_ops);  
			printf("ops = %d/%d", processed_ops, nbfinal_ops); 
		}

	}


}


int main(void)
{
	//Global variable : overall_counter
	int err; 
	int gc_ops=0;
	int nbthreads_counter=0;


	//le but est de créer plusieurs threads 
	//chaque threads réalise un traitement 
	//lise et mette à jour gc_ops en bout de course 
	//avant de continuer leur traitement

	if (pthread_mutex_init(&lock_processed_ops, NULL) != 0)
    {
        printf("\n mutex initialization failed\n");
        return 1;
    }


	while (nbthreads_counter<NBTHREADS)
	{
		err = pthread_create(&(thread_ids[nbthreads_counter]),NULL,&process,NULL);
		if(!err)
			printf("\ncannott create thread :[%s]", strerror(err));
		
		nbthreads_counter++;
	}

	while(nbthreads_counter>1)
	{
		pthread_join(threads_ids[nbthreads_counter--],NULL); 
	}

	pthread_mutex_destroy(&lock);

	return 0; 


}

