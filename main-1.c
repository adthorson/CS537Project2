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
// Pair Partner: Adam Thorson adthorson@wisc.edu
// CS Login: thorson
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
int nframes, pageFault=0, diskRead=0, diskWrite=0;
char *virtmem;
char *physmem;


//RANDOM SEED STUFF
unsigned short seedv[3] = {1,1,1};
double randomDouble;


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
    seed48(seedv);
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
    PFDB = malloc(sizeof(struct frame) * nframes);
    
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
    
    
    
    virtmem = page_table_get_virtmem(pt);
    //virtmem = malloc(sizeof(struct page_table));
    //virtmem = page_table_get_virtmem(pt);
    physmem = page_table_get_physmem(pt);
    
    
    if(!strcmp(program,"sort")) {
        sort_program(virtmem, npages*PAGE_SIZE);
        
    } else if(!strcmp(program,"scan")) {
        scan_program(virtmem, npages*PAGE_SIZE);
        
    } else if(!strcmp(program,"focus")) {
        focus_program(virtmem, npages*PAGE_SIZE);
        
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
    
    pageFault++;
    
    int i, replacement=1;
    int *frame;
    frame = malloc(sizeof(int*));
    int *bits;
    bits = malloc(sizeof(int*));
    
    physmem = page_table_get_physmem(pt);
    
    page_table_get_entry(pt, page, frame, bits);
    
    // If page fault occurred because a write was attempted to a read-only page, add PROT_WRITE bit
    if (*bits == PROT_READ) {
        page_table_set_entry(pt, page, *frame, PROT_READ|PROT_WRITE);
        return;
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
        diskRead++;
    }
    
    if (replacement == 1) {
        //
        // SEE GLOBAL!!!!!!
        //
        //unsigned short seedv[3];// = {1,1,1};
        //long seed48(seedv);
        randomDouble = drand48();
        int randFrame = (int)(randomDouble* ((double) nframes));
        
        //REMOVE
        //printf("%d\n",randFrame);
        
        int removedPage = PFDB[randFrame].VPN;
        
        //NEED TO CHECK IF WRITE BIT IS SET
        page_table_get_entry(pt, removedPage, frame, bits);
        if (*bits == (PROT_READ|PROT_WRITE)) {
            disk_write(disk, removedPage, &physmem[*frame * PAGE_SIZE]);
            diskWrite++;
        }
        PFDB[randFrame].VPN = -1;
        
        PFDB[randFrame].VPN = page; // set new page
        disk_read(disk, page, &physmem[(nframes-1) * BLOCK_SIZE]); // write page from disk to physical memory
        diskRead++;
        page_table_set_entry(pt, removedPage, 0, 0); // dereference the page we removed from physical memory
        page_table_set_entry(pt, page, nframes-1, PROT_READ); // map page to last frame
    }
    //printf("Page faults: %d\tDisk Reads: %d\tDisk Writes: %d\n", pageFault, diskRead, diskWrite);
    
    free(frame);
    free(bits);
}



/*
 * First In First Out page replacement algorithm
 *
 * Single List Queue
 * @param
 */
void fifoPRA( struct page_table *pt, int page) {
    
    pageFault++;
    
    int i, j, replacement=1;
    int *frame;
    frame = malloc(sizeof(int*));
    int *bits;
    bits = malloc(sizeof(int*));
    
    
    page_table_get_entry(pt, page, frame, bits);
    
    // If page fault occurred because a write was attempted to a read-only page, add PROT_WRITE bit
    if (*bits == PROT_READ) {
        //printf(" READ_BIT\n");
        page_table_set_entry(pt, page, *frame, PROT_READ|PROT_WRITE);
        //PFDB[frame].flags = 1;
        return;
    }
    
    // Check to see if there is an empty frame and set replacement flag
    for (i=0; i < nframes; i++) {
        if (PFDB[i].VPN == -1) {
            //printf(" EMPTY i: %d ",i);
            PFDB[i].VPN = page;
            page_table_set_entry(pt, page, i, PROT_READ);
            //page_table_print(pt);
            disk_read(disk, page, &physmem[i * PAGE_SIZE]);
            diskRead++;
            replacement = 0;
            break;
        }
    }
    
    // DO FIFO
    if (replacement == 1) {
        
        //printf("DOFIFO ");
        
        // Remove head -- NEED TO CHECK IF WRITE BIT IS SET
        int removedPage = PFDB[0].VPN;
        
        page_table_get_entry(pt, removedPage, frame, bits);
        if (*bits == (PROT_READ|PROT_WRITE)) {
            disk_write(disk, removedPage, &physmem[*frame * PAGE_SIZE]);
            diskWrite++;
        }
        
        PFDB[0].VPN = -1;
        
        // Shift elements towards head
        for (j=0; j < nframes-1; j++) {
            PFDB[j].VPN = PFDB[j+1].VPN;
        }
        
        PFDB[nframes-1].VPN = page; // set new page to tail
        disk_read(disk, page, &physmem[(nframes-1) * BLOCK_SIZE]); // write page from disk to physical memory
        diskRead++;
        page_table_set_entry(pt, removedPage, 0, 0); // dereference the page we removed from physical memory
        page_table_set_entry(pt, page, nframes-1, PROT_READ); // map page to last frame
        
        
    }
    //printf("Page faults: %d\tDisk Reads: %d\tDisk Writes: %d\n", pageFault, diskRead, diskWrite);
    
    free(frame);
    free(bits);
}

/*
 * Second-Chance First In First Out page replacement algorithm
 *
 * Double List Queues
 * @param
 */
void SfifoPRA( struct page_table *pt, int page) {
    //Queue 1 = 75% (rounded down) of PFDB
    //Queue 2 = 25% of PFDB
    
    //printf("INCOMING PAGE: %d\n",page);
    
    pageFault++;
    
    int i, j, replacement=1;
    int frame;
    int *bits;
    bits = malloc(sizeof(int));
    int headOfFirstQueue = nframes - (nframes/4) - 1 ;
    int tailOfSecondQueue = nframes - (nframes/4);
    int headOfSecondQueue = nframes-1;
    physmem = page_table_get_physmem(pt);
    
    page_table_get_entry(pt, page, &frame, bits);
    
    // If page fault occurred because a write was attempted to a read-only page, add PROT_WRITE bit
    if (*bits == PROT_READ) {
        //printf("WRITE_BIT SET \n");
        page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
        if (PFDB[frame].flags == 0) {
            //printf("REVIVE FRAME: %d\n",frame);
            int phoenix = PFDB[frame].VPN;  // This is the frame which will move to the 1st queue
            // Shift 2nd queue elements towards head
            for (j = frame; j > tailOfSecondQueue; j--) {
                PFDB[j].VPN = PFDB[j-1].VPN;
            }
            PFDB[tailOfSecondQueue].VPN = -1;
            int removedFirstPage = PFDB[headOfFirstQueue].VPN;
            PFDB[tailOfSecondQueue].VPN = removedFirstPage;
            // DO WE HAVE TO SET THIS IN THE PAGE TABLE?
            page_table_set_entry(pt, removedFirstPage, tailOfSecondQueue, PROT_READ);
            
            /*for (k = tailOfSecondQueue; k < nframes; k++) {
             if (PFDB[k].VPN == -1) {
             PFDB[k].VPN = removedFirstPage;
             PFDB[k].flags = 0;
             page_table_set_entry(pt, removedFirstPage, k, PROT_READ);  //DO WE HAVE TO USE THIS?
             break;
             }
             }
             */
            // Shift first queue elements towards head
            for (j=headOfFirstQueue; j > 0; j--) {
                PFDB[j] = PFDB[j-1];
            }
            PFDB[0].VPN = phoenix;
            PFDB[0].flags = 1;
        }
        return;
    }
    
    // Check to see if there is an empty frame within the first queue and set replacement flag
    // if there is, append the PTE to the empty frame
    for (i=0; i < tailOfSecondQueue; i++) {
        //printf("CHECKING FOR EMPTY FRAME IN FIRST \n");
        if (PFDB[i].VPN == -1) {
            PFDB[i].VPN = page;
            PFDB[i].flags = 1;
            //printf("flag is now: %d\n", PFDB[i].flags);
            disk_read(disk, page, &physmem[i * PAGE_SIZE]);
            diskRead++;
            page_table_set_entry(pt, page, i, PROT_READ);
            replacement = 0;
            
            break;
        }
    }
    
    
    // If the first queue is Full, we begin to check if the second is empty and push last frame of 1st queue to 2nd queue
    if (replacement == 1) {
        int removedFirstPage = PFDB[headOfFirstQueue].VPN;       // head from the first queue -> used to put in tail of second queue
        //printf("FIRST QUEUE FULL \n");
        
        // Check to see if there is an empty frame within the second queue and set replacement flag
        
        // Shift second queue if there is an empty spot
        if (PFDB[headOfSecondQueue].VPN == -1) {
            for (i = headOfSecondQueue; i > tailOfSecondQueue; i--) {
                PFDB[i].VPN = PFDB[i-1].VPN;
            }
            PFDB[tailOfSecondQueue].VPN = removedFirstPage;
            PFDB[tailOfSecondQueue].flags = 0;
            replacement = 0;
            
            //page_table_set_entry(pt, removedFirstPage, tailOfSecondQueue, PROT_READ);
            // DO WE HAVE TO UPDATE PAGE TABLE?
        }
        
        // If the second queue is full
        if (replacement == 1) {
            //printf("SECOND QUEUE FULL\n");
            int removedSecondPage = PFDB[headOfSecondQueue].VPN;
            
            // Write back to disk if the evicted frame has write bit set
            // Maybe the problem is here?
            page_table_get_entry(pt, removedSecondPage, &frame, bits);
            if (*bits == (PROT_READ|PROT_WRITE)) {
                //printf("HELLO");
                disk_write(disk, removedSecondPage, &physmem[frame * PAGE_SIZE]);
                diskWrite++;
            }
            
            
            // Shift elements towards head of 2nd queue
            for (i = headOfSecondQueue; i > tailOfSecondQueue; i--) {
                PFDB[i].VPN = PFDB[i-1].VPN;
            }
            
            PFDB[tailOfSecondQueue].VPN = removedFirstPage; // set new page to tail of second queue
            PFDB[tailOfSecondQueue].flags = 0;
            //printf("flag is now: %d\n", PFDB[nframes-1].flags);
            page_table_set_entry(pt, removedSecondPage, 0, 0); // dereference the page we removed from physical memory
            //page_table_set_entry(pt, removedFirstPage, headOfSecondQueue, PROT_READ); // map page to last frame
            
        }
        
        // Shift elements towards head
        
        for (j = headOfFirstQueue; j > 0; j--) {
            PFDB[j].VPN = PFDB[j-1].VPN;
        }
        
        PFDB[0].VPN = page;
        PFDB[0].flags = 1;
        //printf("flag is now: %d\n", PFDB[headOfFirstQueue-1].flags);
        disk_read(disk, page, &physmem[0]); // read page from disk to physical memory
        diskRead++;
        page_table_set_entry(pt, page, 0, PROT_READ); // map page to tail of first queue
        
    }
    printf("Page faults: %d\tDisk Reads: %d\tDisk Writes: %d\t\n",pageFault,diskRead,diskWrite);
    
    free(bits);
    
    
}

/* Custom page replacement algorithm
*
*
* @param
*/

void customPRA( struct page_table *pt, int page) {
    
    ++pageFault;
    //NOTE:
    //      ARE WE GOING TO KEEP SAME RATIO OF FIRST TO SECOND LIST????????????????????????????????????????
    
    int i, j, k, replacement=1, secondFull=1;
    int frame;
    int dumbyFrame;
    int removedSecondPage = -1;
    int removedIndex;
    
    int *bits;
    bits = malloc(sizeof(int));
    int *dumbyBits;
    dumbyBits = malloc(sizeof(int));
    
    int headOfFirstQueue = nframes - (nframes/2)-1;
    int tailOfSecondQueue = nframes - (nframes/2);
    physmem = page_table_get_physmem(pt);
    
    page_table_get_entry(pt, page, &frame, bits);
    
    // If page fault occurred because a write was attempted to a read-only page, add PROT_WRITE bit
    if (*bits == PROT_READ) {
        //printf("WRITE_BIT SET \n");
        page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
        if (PFDB[frame].flags == 0) {
            //printf("REVIVE FRAME: %d\n",frame);
            int phoenix = PFDB[frame].VPN; // This is the frame which will move to the 1st queue
            // Shift 2nd queue elements towards head
            for (j = frame; j > tailOfSecondQueue; j--) {
                PFDB[j].VPN = PFDB[j-1].VPN;
            }
            PFDB[tailOfSecondQueue].VPN = -1;
            int removedFirstPage = PFDB[headOfFirstQueue].VPN;
            PFDB[tailOfSecondQueue].VPN = removedFirstPage;
            // DO WE HAVE TO SET THIS IN THE PAGE TABLE?
            page_table_set_entry(pt, removedFirstPage, tailOfSecondQueue, PROT_READ);
            
            /*for (k = tailOfSecondQueue; k < nframes; k++) {
             if (PFDB[k].VPN == -1) {
             PFDB[k].VPN = removedFirstPage;
             PFDB[k].flags = 0;
             page_table_set_entry(pt, removedFirstPage, k, PROT_READ); //DO WE HAVE TO USE THIS?
             break;
             }
             }
             */
            // Shift first queue elements towards head
            for (j=headOfFirstQueue; j > 0; j--) {
                PFDB[j] = PFDB[j-1];
            }
            PFDB[0].VPN = phoenix;
            PFDB[0].flags = 1;
        }
        return;
    }
    
    // Check to see if there is an empty frame within the first queue and set replacement flag
    // if there is, append the PTE to the empty frame
    for (i=0; i < tailOfSecondQueue; i++) {
        //printf("CHECKING FOR EMPTY FRAME IN FIRST \n");
        if (PFDB[i].VPN == -1) {
            PFDB[i].VPN = page;
            PFDB[i].flags = 1;
            //printf("flag is now: %d\n", PFDB[i].flags);
            disk_read(disk, page, &physmem[i * PAGE_SIZE]);
            diskRead++;
            page_table_set_entry(pt, page, i, PROT_READ);
            replacement = 0;
            
            break;
        }
    }
    
    
    // If the first queue is Full, we begin to check if the second is empty and push last frame of 1st queue to 2nd queue
    if (replacement == 1) {
        int removedFirstPage = PFDB[headOfFirstQueue].VPN; // head from the first queue -> used to put in tail of second queue
        //printf("FIRST QUEUE FULL \n");
        
        // Check to see if there is an empty frame within the second queue and set replacement flag
        // if there is, append the PTE to the empty frame
        
        // Shift second queue if there is an empty spot
        if (PFDB[nframes-1].VPN == -1) {
            for (i = nframes-1; i > tailOfSecondQueue; i--) {
                PFDB[i].VPN = PFDB[i-1].VPN;
            }
            PFDB[tailOfSecondQueue].VPN = removedFirstPage;
            PFDB[tailOfSecondQueue].flags = 0;
            replacement = 0;
            
            //page_table_set_entry(pt, removedFirstPage, tailOfSecondQueue, PROT_READ);
            // DO WE HAVE TO UPDATE PAGE TABLE?
        }
        
        
        
        
        if (replacement == 1) {
            //printf("SECOND QUEUE FULL\n");
            
            
            //DECIDE WHAT THIS SHOULD BE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //
            //DECIDE WHAT THIS SHOULD BE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //
            //
            double tweekValue = 0.95;
            
            randomDouble = drand48();
            //printf("Double: %1.5f\n",randomDouble);
            
            //WHICH PAGE TO REMOVE???
            if(randomDouble < tweekValue){
                for (j = nframes - 1; j > tailOfSecondQueue - 1; --j){
                    page_table_get_entry(pt, PFDB[j].VPN, &dumbyFrame, dumbyBits);
                    if (*dumbyBits == PROT_READ) {
                        //AT BEGINNING, removedSecondPage = -1
                        removedSecondPage = PFDB[j].VPN;
                        removedIndex = j;
                        //printf("READ BITS\n\n");
                        break;
                    }
                }
            }
            //IF DIDN'T ENCOUNTER RD_ONLY OR DOESN'T MATTER WHAT VALUE...
            if(removedSecondPage == -1){
                removedSecondPage = PFDB[nframes - 1].VPN;
                removedIndex = nframes - 1;
            }
            
            
            
            page_table_get_entry(pt, removedSecondPage, &frame, bits);
            if (*dumbyBits == (PROT_READ|PROT_WRITE)) {
                disk_write(disk, removedSecondPage, &physmem[dumbyFrame * PAGE_SIZE]);
                diskWrite++;
            }
            
            
            // Shift elements towards head/removed element
            for (j = removedIndex; j > tailOfSecondQueue; --j) {
                PFDB[j].VPN = PFDB[j-1].VPN;
            }
            
            
            PFDB[tailOfSecondQueue].VPN = removedFirstPage; // set new page to head of second queue
            PFDB[tailOfSecondQueue].flags = 0;
            //printf("flag is now: %d\n", PFDB[nframes-1].flags);
            //disk_read(disk, removedFirstPage, &physmem[(nframes-1) * BLOCK_SIZE]); // write page from disk to physical memory
            //diskRead++;
            page_table_set_entry(pt, removedSecondPage, 0, 0); // dereference the page we removed from physical memory
            //page_table_set_entry(pt, removedFirstPage, headOfSecondQueue, PROT_READ); // map page to last frame
            
        }
        
        // Shift elements towards head
        
        for (j = headOfFirstQueue; j > 0; j--) {
            PFDB[j].VPN = PFDB[j-1].VPN;
        }
        
        PFDB[0].VPN = page;
        PFDB[0].flags = 1;
        //printf("flag is now: %d\n", PFDB[headOfFirstQueue-1].flags);
        disk_read(disk, page, &physmem[0]); // write page from disk to physical memory
        diskRead++;
        page_table_set_entry(pt, page, 0, PROT_READ); // map page to last frame
        
    }
    
    printf("Page faults: %d\tDisk Reads: %d\tDisk Writes: %d\t\n",pageFault,diskRead,diskWrite);
    
    free(bits);
    free(dumbyBits);
    
}
