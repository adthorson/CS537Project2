///////////////////////////////////////////////////////////////////////////////
// PROGRAM 2 - Memory
// Title: Main.c
// Files: Main.c, disk.c, page_table.c, program.c, Makefile
// Semester: CS537 Spring 2013
//
// PAIR PROGRAMMERS
//
// Pair Partner: (Ted) Tianchu Huang thuang33@wisc.edu
// CS Login: Tianchu
// Lecturer's Name: Michael Swif
//
// Pair Partner: Tyson Williams tjwilliams4@wisc.edu
// CS Login: twilliam
// Lecturer's Name: Michael Swift
//
// Pair Partner: Adam Thorson @wisc.edu
// CS Login:
// Lecturer's Name: Michael Swift
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
    int VPN;
    int flags;
};

struct frame * PFDB;
struct disk *disk;
int nframes;
char *physmem;

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
 //use this f =[ree frame
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
    int i;
    struct page_table *pt;
    
    if(argc!=5) {
        printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
        return 1;
    }
    
    int npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char *PRA = argv[3]; //PRA = Page Replacement Algorithm
    const char *program = argv[4];
    
    disk = disk_open("myvirtualdisk",npages);
    PFDB = (struct frame*) malloc(nframes);
    
    // Initialize each VPN to -1 in PFDB
    for (i=0; i < nframes; i++) {
        PFDB[i].VPN = -1;
    }
    
    if(!disk) {
        fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
        return 1;
    }
    
    if(!strcmp(PRA,"rand")) {
        pt = page_table_create( npages, nframes, randPRA );
        
    } else if(!strcmp(PRA, "fifo")) {
        pt = page_table_create( npages, nframes, fifoPRA );
        
    } else if(!strcmp(PRA, "2fifo")) {
        pt = page_table_create( npages, nframes, SfifoPRA );
        
    } else if(!strcmp(PRA, "custom")) {
        pt = page_table_create( npages, nframes, customPRA );
        
    } else {
        printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
        return 1;
    }
    
    if(!pt) {
        fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
        return 1;
    }
    
    
    
    char *virtmem = page_table_get_virtmem(pt);
    physmem = page_table_get_physmem(pt);
    
    
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
void randPRA( struct page_table *pt, int page) {
    int i, replacement=1;
    int *frame;
    int *bits;
    
    page_table_get_entry(pt, page, frame, bits);
    
    // If page fault occurred because a write was attempted to a read-only page, add PROT_WRITE bit
    if (*bits == PROT_READ) {
        page_table_set_entry(pt, page, *frame, PROT_READ|PROT_WRITE);
    }
	
	
	for (i=0; i < nframes; i++) {
        if (PFDB[i].VPN == -1) {
            PFDB[i].VPN = page;
            replacement = 0;
            break;
        }
    }
	
	if (replacement == 0) {
        page_table_set_entry(pt, page, i, PROT_READ);
        disk_read(disk, page, &physmem[i * BLOCK_SIZE]);
    }
	
	if (replacement == 1) {
		unsigned short seedv[3]// = {1,1,1};
		long seed48(seedv);
		int randFrame = lrand48();
        int removedPage = PFDB[randFrame].VPN;
		
		//NEED TO CHECK IF WRITE BIT IS SET
		page_table_get_entry(pt, removedPage, frame, bits);
        if (*bits == (PROT_READ|PROT_WRITE)) {
            disk_write(disk, removedPage, &physmem[frame * PAGE_SIZE]);
        }
        PFDB[randFrame].VPN = -1;
    }
    
    PFDB[nframes-1].VPN = page;                                 // set new page to tail
    page_table_set_entry(pt, page, nframes-1, PROT_READ);       // map page to last frame
    disk_read(disk, page, &physmem[(nframes-1) * BLOCK_SIZE]);  // write page from disk to physical memory
    page_table_set_entry(pt, removedPage, 0, 0);                // dereference the page we removed from physical memory
    
}

/*
 * First In First Out page replacement algorithm
 *
 * Single List Queue
 * @param
 */
void fifoPRA( struct page_table *pt, int page) {
    
    //We come in realizing that "page" in "page_table" has been faulted
    //1. search in PFDB for free frame
    // 1.1. if PFDB full
    // 1.1.1 <removeFrameEntry> remove entry from PFDB at head of list
    // 1.1.2 <returnFreeSpace> return addr to free frame
    // 1.1.3 <disk_read> read "page" from "disk"
    // 1.1.4 <addFrameEntry> add this page into the free frame to tail of list
    // 1.1.5 <page_table_set_entry> append "page" to "page_table"
    // 1.1.6 ?restart process?
    // 1.2 if PFDB has free frame.
    // 1.2.1 <*returnFreeSpace> return addr to free frame
    // 1.2.2 <disk_read> read "page" from "disk"
    // 1.2.3 <addFrameEntry> add this page into the free frame to tail of list
    // 1.2.4 <page_table_set_entry> append "page" to "page_table"
    // 1.1.6 ?restart process?
    
    int i, j, replacement=1	;
    int *frame;
    int *bits;
    
    page_table_get_entry(pt, page, frame, bits);
    
    // If page fault occurred because a write was attempted to a read-only page, add PROT_WRITE bit
    if (*bits == PROT_READ) {
        page_table_set_entry(pt, page, *frame, PROT_READ|PROT_WRITE);
        //PFDB[frame].flags = 1;
    }
    
    // Check to see if there is an empty frame and set replacement flag
    for (i=0; i < nframes; i++) {
        if (PFDB[i].VPN == -1) {
            PFDB[i].VPN = page;
            replacement = 0;
            break;
        }
    }
    
    // If there is an empty frame, use it
    if (replacement == 0) {
        page_table_set_entry(pt, page, i, PROT_READ);
        disk_read(disk, page, &physmem[i * BLOCK_SIZE]);
    }
    
    // DO FIFO
    if (replacement == 1) {
        // Remove head -- NEED TO CHECK IF WRITE BIT IS SET
        int removedPage = PFDB[0].VPN;
        
        page_table_get_entry(pt, removedPage, frame, bits);
        if (*bits == (PROT_READ|PROT_WRITE)) {
            disk_write(disk, removedPage, &physmem[*frame * PAGE_SIZE]);
        }
        
        PFDB[0].VPN = -1;
        
        // Shift elements towards head
        for (j=0; j < nframes-1; j++) {
            PFDB[j].VPN = PFDB[j+1].VPN;
        }
        
        PFDB[nframes-1].VPN = page;                                 // set new page to tail
        page_table_set_entry(pt, page, nframes-1, PROT_READ);       // map page to last frame
        disk_read(disk, page, &physmem[(nframes-1) * BLOCK_SIZE]);  // write page from disk to physical memory
        page_table_set_entry(pt, removedPage, 0, 0);                // dereference the page we removed from physical memory
        
    }
}

/*
 * Second-Chance First In First Out page replacement algorithm
 *
 * Double List Queues
 * @param
 */
void SfifoPRA( struct page_table *pt, int page) {
    
}

/*
 * Custom page replacement algorithm
 *
 *
 * @param
 */
void customPRA( struct page_table *pt, int page) {
    
}


