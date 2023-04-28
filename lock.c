#include<unistd.h>
#include<stdint.h>
#include<x86intrin.h> 
#include<stdio.h>

#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<errno.h>
#include<time.h>

#define WAITTIME 10
#define NBPAGES 100
#define NBTHREADSMAX 1024
#define NBTHREADS 100
#define NBREADERS 1 
#define NBOPS_C 512*NBPAGES*NBTHREADS
#define ERRORLEN 100
#define PERIOD 1

#define LOCK_WRITER_IMPACT 0
//#define NOLOCK 0 

pthread_mutex_t lock_processed_ops; 

struct page{
	int array[512];
}page;

pthread_t thread_ids[NBTHREADSMAX];
pthread_t reader_ids[NBTHREADSMAX]; 

long processed_ops; 
long nbops = 0; 
int writers_ended = 0; 

void * reader(void *period)
{
	long periodic_read = *((unsigned long *) period); 

	clock_t start = clock();
	uint64_t cycles_start = __rdtsc();
	float global_lock_time = 0.0;
	uint64_t global_lock_cycles = 0; 
	_mm_lfence();

	while(!writers_ended){
		sleep(periodic_read);

		#ifdef LOCK_WRITER_IMPACT
		clock_t lock_begin_reader = clock();
		uint64_t cycles_start_reader = __rdtsc();
		_mm_lfence(); 
		#endif 

		
//		#ifndef NOLOCK
		pthread_mutex_lock(&lock_processed_ops);
//		#endif
		#ifdef LOCK_WRITER_IMPACT
		clock_t lock_begin_end = clock();
		uint64_t cycles_end_lock_begin = __rdtsc();
		#endif 

		printf("Reader processed_ops = %ld",processed_ops); 

		#ifdef LOCK_WRITER_IMPACT
		clock_t lock_exit_start = clock();
		uint64_t cycles_lock_exit_begin = __rdtsc();
		#endif 

//		#ifndef NOLOCK
		pthread_mutex_unlock(&lock_processed_ops);
//		#endif 

		#ifdef LOCK_WRITER_IMPACT
		clock_t lock_exit_end = clock();
		uint64_t cycles_lock_exit_end = __rdtsc();
		printf("ReaderInnerMetric execTime:%.9f locktime:%.9f execcyles:%ld lockcycles:%ld\n", 
						1.0*(lock_exit_end-lock_begin_reader)/CLOCKS_PER_SEC,
						1.0*((lock_begin_end-lock_begin_reader)+(lock_exit_end-lock_exit_start))/CLOCKS_PER_SEC,
						(cycles_lock_exit_end-cycles_start_reader),
						((cycles_lock_exit_end-cycles_lock_exit_begin)+(cycles_end_lock_begin-cycles_start_reader))); 
		global_lock_time += 1.0*((lock_begin_end-lock_begin_reader)+(lock_exit_end-lock_exit_start))/CLOCKS_PER_SEC;
		global_lock_cycles += ((cycles_lock_exit_end-cycles_lock_exit_begin)+(cycles_end_lock_begin-cycles_start_reader));
		#endif 

	}
	clock_t end = clock();
	uint64_t cycles_end = __rdtsc();
	_mm_lfence(); 

	printf("ReaderFinalMetric execTime: %ld cycles:%ld globallocktime:%.9f globallockcycles:%ld\n", 
			(end-start)/CLOCKS_PER_SEC,(cycles_end-cycles_start),global_lock_time,global_lock_cycles);
} 


/**
 * @brief processing performed by the reader
 * allocate x pages and fills random entries.
 * Each random entry comes with an increment of the shared 
 * processed_ops with is filled by every writer
 * 
 * @param nb_ops : nb of operations to process globally when combining
 * results of every thread 
 */
void * writer(void *nb_ops)
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
	#ifdef LOCK_WRITER_IMPACT
	clock_t writer_time = clock();
	float global_lock_time = 0.0;
	uint64_t global_lock_cycles = 0; 
	uint64_t start_cycles = __rdtsc();
	#endif 
	for(counter = 0; counter < NBPAGES; counter++)
	{
		for(j=0; j<512; j++)
		{	
			#ifdef LOCK_WRITER_IMPACT
			clock_t lock_begin_writer = clock();
			uint64_t cycles_start_writer = __rdtsc();
			_mm_lfence(); 
			#endif 

			srand(time(NULL));  
			myPage[counter].array[j]=rand()*j + rand()*j;

			#ifdef LOCK_WRITER_IMPACT 
			clock_t lock_begin = clock();
			uint64_t cycles_start = __rdtsc();
			_mm_lfence(); //Force the timers to execute before the rest 

			#endif 

//			#ifndef NOLOCK
			pthread_mutex_lock(&lock_processed_ops);
//			#endif 

			#ifdef LOCK_WRITER_IMPACT
			clock_t lock_end = clock();
			uint64_t cycles_stop = __rdtsc();
			_mm_lfence();
			#endif 

			processed_ops +=1;

			#ifdef LOCK_WRITER_IMPACT
			clock_t lock_exit_begin = clock();
			uint64_t cycles_exit_start = __rdtsc(); 
			_mm_lfence();
			#endif  

//			#ifndef NOLOCK
			pthread_mutex_unlock(&lock_processed_ops); 
//			#endif 

			#ifdef LOCK_WRITER_IMPACT
			clock_t lock_exit_end = clock();
			uint64_t cycles_exit = __rdtsc();

			#endif 

			#ifdef LOCK_WRITER_IMPACT
			printf("Writer exectime:%.9f locktime:%.9f execcycles:%ld lockcycles:%ld\n",
						1.0*(lock_exit_end-lock_begin_writer)/CLOCKS_PER_SEC,
						((1.0*(lock_end-lock_begin)/CLOCKS_PER_SEC)+(1.0*(lock_exit_end-lock_exit_begin)/CLOCKS_PER_SEC)),
						(cycles_start_writer-cycles_exit),
						((cycles_stop - cycles_start)+(cycles_exit-cycles_exit_start))
						); 
			#endif 

			printf("Writerthread id-[%ld]ops = %ld/%ld\n", thread_id, processed_ops, *((unsigned long *) nb_ops)); 

			global_lock_time += (1.0*((lock_end-lock_begin)/CLOCKS_PER_SEC)+((lock_exit_end-lock_exit_begin)/CLOCKS_PER_SEC));
			global_lock_cycles += ((cycles_stop - cycles_start)+(cycles_exit-cycles_exit_start));
		
		}

	}

	#ifdef LOCK_WRITER_IMPACT
	clock_t writer_time_end = clock();
	uint64_t end_cycles = __rdtsc();
	#endif 

	printf("WriterFinalMetric exectime:%ld cycles:%ld globallocktime:%.9f globallockcycles:%ld\n",(writer_time_end-writer_time)/CLOCKS_PER_SEC,(end_cycles-start_cycles),global_lock_time,global_lock_cycles);
	//exit(0); 


}

/**
 * ./exec.o nbthreads nbreaders period_for_readers
 * default values check #define in headers 
 */
int main(int argc, char **argv)
{
	//Global variable : overall_counter
	int err; 
	int gc_ops=0;
	int nbthreads_counter=0;
	int threads_to_create=0;
	int readers_to_create=0; 
	int nb_readers = 0; 
	char error_msg[ERRORLEN];
	int error_num = 0;
	int period_to_use = 0; 



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
	readers_to_create = (argc > 2) ? atoi(argv[2]) : NBREADERS; 
	period_to_use = (argc > 3) ? atoi(argv[3]) : PERIOD; 
	nbops = (argc <= 1) ? NBOPS_C : threads_to_create*512*NBPAGES;

	printf("Waiting %dsec to catch events ...\n",WAITTIME);
	sleep(WAITTIME);

	printf("Starting program with %d threads and %ld expected ops\n",threads_to_create, nbops);

	clock_t start_program = clock(); 
	uint64_t cycles = __rdtsc();
	_mm_lfence();


	//writers init
	while (nbthreads_counter<threads_to_create)
	{
		err = pthread_create(&(thread_ids[nbthreads_counter]),NULL, writer,&nbops);
		if(err!=0)
		{
			error_num = strerror_r(err,error_msg,ERRORLEN);
			printf("\ncannott create writer thread.\n Reason :[%s].\n Exiting\n", error_msg);
			exit(0); 
		}
		
		nbthreads_counter++;
	}

	//readers init 
	while (nb_readers < readers_to_create)
	{
		err = pthread_create(&(reader_ids[nb_readers]),NULL, reader, &period_to_use);
		if(err!=0)
		{
			error_num = strerror_r(err, error_msg, ERRORLEN);
			printf("\ncannot create reader thread.\n Reason :[%s].\n Exiting\n",error_msg);
		}

		nb_readers++; 
	}

	

	while(nbthreads_counter>1)
	{
		pthread_join(thread_ids[nbthreads_counter--],NULL); 
	}

	writers_ended = 1; 

	while(nb_readers>1)
	{
		pthread_join(thread_ids[nb_readers--],NULL);
	}

	
	pthread_mutex_destroy(&lock_processed_ops);
	uint64_t finish_cycles = __rdtsc();

	printf("\n Finished Exectime:%.9f  cycles:%ld\n",
				1.0*(clock()-start_program)/CLOCKS_PER_SEC,
				(finish_cycles-cycles)); 

	return 0; 




}

