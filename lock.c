#include<stdio.h>
#include<stdlib.h>

#define NBPAGES 1024

struct page{
	int array[512];
};


int process(void *p)
{
	page *myPage = malloc(1024*8*sizeof(array[512]));
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

}


int main(void)
{
	//Global variable : overall_counter
	int gc_ops=0;

	//le but est de créer plusieurs threads 
	//chaque threads réalise un traitement 
	//lise et mette à jour gc_ops en bout de course 
	//avant de continuer leur traitement	
}

