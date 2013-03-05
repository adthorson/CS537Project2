///////////////////////////////////////////////////////////////////////////////
// PROGRAM 2 - Memory
// Title:            Main.c
// Files:            Main.c, disk.c, page_table.c, program.c, Makefile
// Semester:         CS537 Spring 2013
//
//                   PAIR PROGRAMMERS
//
// Pair Partner:     (Ted) Tianchu Huang thuang33@wisc.edu
// CS Login:         Tianchu
// Lecturer's Name:  Michael Swif
//
// Pair Partner:     Tyson Williams tjwilliams4@wisc.edu
// CS Login:         twilliam
// Lecturer's Name:  Michael Swift
//
// Pair Partner:     Adam Thorson @wisc.edu
// CS Login:         
// Lecturer's Name:  Michael Swift
//
//////////////////////////// 80 columns wide //////////////////////////////////
/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct frame {
    struct frame * next;
    struct frame * prev;
    int VPN;
    int flags;
};

struct frame * PFDB;
struct disk *disk;

void randPRA( struct page_table *pt, int page);
void fifoPRA( struct page_table *pt, int page);
void SfifoPRA( struct page_table *pt, int page);
void customPRA( struct page_table *pt, int page);
void addFrameEntry( struct page_table *pt, int page);
void removeFrameEntry( struct page_table *pt, int page);
char *returnFreeSpace( struct page_table *pt, int page);


/*
void page_fault_handler( struct page_table *pt, int page)
{

	//we assume that reference in page table was invalid, causing page-fault trap
	//we assume data is in disk
	//we find a free frame (from a free-frame list)
	
	//check if new PTE already exists with different permission bits
	//pt->page_bits
	
	//int frameNumber = page_table_get_physmem(pt); //return number of frames
	int *freeLocation = findFreeFrame(freeFrameList); //freeLocation = addr of freeMem
	if(freeLocation != NULL)
		setFreeFrame(freeFrameList,freeLocation); //remove from freeFrameList
	
	//if there are no free frames
	else if(freeLocation == NULL){
		// store a frame (from an algorithm) into disk
		page_table_set_entry(pt, int page, int frame, int bits );
		addPageFrameToTable(freeLocation,frame);
	}
	//change the page table to reflect accordingly
	//use this f	=[ree frame
	
	
	//using a schedule, read the desired page into new allocated frame
	
	//find page in disk
	disk_read(disk, page, freeLocation); //assumeing block = block number where we find page
	page_table_set_entry(pt,page,freeLocation,PROT_READ);
	//modify page table to indicate that page is now in memory and off disk
	//restart instruction as though it had been in memory
	
	
	printf("page fault on page #%d\n",page);
	exit(1);
}
*/

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	const char *PRA = argv[3];	//PRA = Page Replacement Algorithm
	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages);
	PFDB = malloc(sizeof(struct frame)*nframes);
	
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	
	if(!strcmp(PRA,"rand")) {
		struct page_table *pt = page_table_create( npages, nframes, randPRA );
		
	} else if(!strcmp(PRA, "fifo")){
		struct page_table *pt = page_table_create( npages, nframes, fifoPRA );
		
	} else if(!strcmp(PRA, "2fifo")){
		struct page_table *pt = page_table_create( npages, nframes, SfifoPRA );
		
	} else if(!strcmp(PRA, "custom")){
		struct page_table *pt = page_table_create( npages, nframes, customPRA );
		
	} else{
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}
	
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);
	
	
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}


/*
 * Random page replacement algorithm
 *
 * Single List Queue
 * @param 
 */
void randPRA( struct page_table *pt, int page){
	
};

/*
 * First In First Out page replacement algorithm
 *
 * Single List Queue
 * @param 
 */
void fifoPRA( struct page_table *pt, int page){

};

/*
 * Second-Chance First In First Out page replacement algorithm
 *
 * Double List Queues
 * @param 
 */
void SfifoPRA( struct page_table *pt, int page){

};

/*
 * Custom page replacement algorithm
 *
 * 
 * @param
 */
void customPRA( struct page_table *pt, int page){

};











