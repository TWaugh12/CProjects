#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "p3Heap.h"

/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block.
 */
typedef struct blockHeader {           

    /*
     * 1) The size of each heap block must be a multiple of 8
     * 2) heap blocks have blockHeaders that contain size and status bits
     * 3) free heap block contain a footer, but we can use the blockHeader 
     *.
     * All heap blocks have a blockHeader with size and status
     * Free heap blocks have a blockHeader as its footer with size only
     *
     * Status is stored using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * Start Heap: 
     *  The blockHeader for the first block of the heap is after skip 4 bytes.
     *  This ensures alignment requirements can be met.
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
    int size_status;

} blockHeader;         

/* 
 * It must point to the first block in the heap and is set by init_heap()
 * i.e., the block at the lowest address.
 */
blockHeader *heap_start = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int alloc_size;


/**
 * Finds and returns the best fit block for the specified size from a memory heap.
 * 
 * Pre-conditions: heap_start must be initialized and 
 * pointing to the start of the heap.
 *
 * size: The requested size of the block in bytes
 *
 * retval: Returns a pointer to the blockHeader of the best-fit block. 
 * If no block is found returns NULL.
 */
blockHeader* best_block(int size) {
    blockHeader* current = heap_start;
    blockHeader* fit = NULL;

    while (current->size_status != 1) {  
        
        // Clear bits to get size
        int blockSize = current->size_status & ~3; 
        
        if (blockSize <= 0) {
            break;  // Shouldn't happen but check just in case
        }

        // Check if block is free and can hold size
        if (!(current->size_status & 1) && blockSize >= size) { 
            
            // Selects block if available and it's either the first fit found or it's a better fit than the previous
            if (fit == NULL || blockSize < (fit->size_status & ~3)) {
                fit = current;
            }
        }

        // Move to the next block
        current = (blockHeader*)((char*)current + blockSize);
    }

    if (fit != NULL && (fit->size_status & ~3) < size) {
        return NULL;  // If the best fit found is not large enough, return NULL
    }

    return fit;
}

/**
 * Coaloesces adjacent free blocks, helper method for bfree()
 * Checks next and previous blocks, coalesces if free
 *
 * *block a pointer to the curent block that was just freed
 *
 * If no block is found returns NULL.
 */
void coalesce(blockHeader *block) {
    // Getting the size of the current block
    int blockSize = block->size_status & ~3;

    // Check the previous block
    blockHeader *prevFooter = (blockHeader*)((char*)block - sizeof(blockHeader));
    int prevBlockSize = prevFooter->size_status;
    blockHeader *prevHeader = (blockHeader*)((char*)prevFooter - prevBlockSize + sizeof(blockHeader));
    
    // Check if the previous block is free
    if (((char*)prevFooter >= (char*)heap_start) && !(prevHeader->size_status & 1)) {
        // Coalesce with the previous block
        int newBlockSize = blockSize + prevBlockSize;
        prevHeader->size_status = newBlockSize;
        blockHeader *newFooter = (blockHeader*)((char*)block + blockSize - sizeof(blockHeader));
        newFooter->size_status = newBlockSize;

        // Update pointers for next block check
        block = prevHeader;
        blockSize = newBlockSize;
    }

    // Check the next block
    blockHeader *nextHeader = (blockHeader*)((char*)block + blockSize);
    
    // Check if we're not at the end of the heap and the block is free
    if (nextHeader->size_status != 1 && !(nextHeader->size_status & 1)) {
        int nextBlockSize = nextHeader->size_status & ~3;

        // Coalesce with the next block
        int newBlockSize = blockSize + nextBlockSize;
        block->size_status = newBlockSize;
        blockHeader *newFooter = (blockHeader*)((char*)nextHeader + nextBlockSize - sizeof(blockHeader));
        newFooter->size_status = newBlockSize;
    }
}


/* 
 * - Check size - Return NULL if size < 1 
 * - Determine block size rounding up to a multiple of 8 
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split 
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements 
 *       - Update all heap block header(s) and footer(s) 
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 *   Return if NULL unable to find and allocate block for required size
 *
 */
void* balloc(int size) {
    if (size < 1) {
        return NULL;
    }

    // Set rounded size variable
    int headerSize = sizeof(blockHeader);
    int rounded_size = (size + headerSize + 7) & ~7; 

    // Find fit for block
    blockHeader* fitBlock = best_block(rounded_size);
    
    // If no fit for block, return null
    if (fitBlock == NULL) {
        return NULL;
    }
    
    // Calculate remaining bits after allocating memory
    int remaining_bits = (fitBlock->size_status & ~3) - rounded_size; 
    

    // Check if split is needed
    if (remaining_bits >= headerSize + 8) { 
       
        // Create new block to use in split, and set size to the remainder
        blockHeader* newBlock = (blockHeader*)((char*)fitBlock + rounded_size);
        newBlock->size_status = remaining_bits;
        
        // Update footer for new free block
        blockHeader* footer = (blockHeader*)((char*)newBlock + remaining_bits - headerSize);
        footer->size_status = remaining_bits;

        // Update size_status for original allocated block
        fitBlock->size_status = rounded_size | 1 | (fitBlock->size_status & 2); 
    } else {
        
        // If no split, mark as allocated
        fitBlock->size_status |= 1; 
    }
    
    // Update the next block's previous block status bit
    blockHeader* nextBlock = (blockHeader*)((char*)fitBlock + (fitBlock->size_status & ~3));
    
    // Set previous block's status to allocated
    nextBlock->size_status |= 2;
    
    return (void*)((char*)fitBlock + headerSize);
}


/* 
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 *
 * If free results in two or more adjacent free blocks,
 * they will be immediately coalesced into one larger free block.
 * so free blocks require a footer (blockHeader works) to store the size
 *
 */                    

int bfree(void *ptr) {    
    if (!ptr) {
        return -1;
    }
    if ((int)ptr % 8 != 0) {
        return -1;
    }
    
    // Initalize variable of pointer to the block to be freed of correct size
    blockHeader *b_to_free = (blockHeader*)((char*)ptr - sizeof(blockHeader));
    
    // Check if the block is already freed.
    if (!(b_to_free->size_status & 1)) {
        return -1;  
    }
    // Check if ptr is within the heap range.
    if ((char*)b_to_free < (char*)heap_start || b_to_free->size_status == 1) {
        return -1;  
    }
    int blockSize = b_to_free->size_status & ~3;
    
    // Free the block.
    b_to_free->size_status &= ~3;
    
    // Set the block's footer.
    blockHeader *footer = (blockHeader*)((char*)b_to_free + blockSize - sizeof(blockHeader));
    footer->size_status = blockSize;
    
    // Call coalesce function to coalesce adjacent free blocks
    coalesce(b_to_free);
    return 0;
}

/* 
 * Initializes the memory allocator.
 * Called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int init_heap(int sizeOfRegion) {    

    static int allocated_once = 0; //prevent multiple myInit calls

    int   pagesize; // page size
    int   padsize;  // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int   fd;

    blockHeader* end_mark;

    if (0 != allocated_once) {
        fprintf(stderr, 
                "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize from O.S. 
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }

    allocated_once = 1;

    // for double word alignment and end mark
    alloc_size -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heap_start = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    end_mark = (blockHeader*)((void*)heap_start + alloc_size);
    end_mark->size_status = 1;

    // Set size in header
    heap_start->size_status = alloc_size;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heap_start->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heap_start + alloc_size - 4);
    footer->size_status = alloc_size;

    return 0;
} 

/* 
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void disp_heap() {     

    int    counter;
    char   status[6];
    char   p_status[6];
    char * t_begin = NULL;
    char * t_end   = NULL;
    int    t_size;

    blockHeader *current = heap_start;
    counter = 1;

    int used_size =  0;
    int free_size =  0;
    int is_used   = -1;

    fprintf(stdout, 
            "*********************************** HEAP: Block List ****************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
            "---------------------------------------------------------------------------------\n");

    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;

        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;

        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
                p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);

        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
            "---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
            "*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
            "*********************************************************************************\n");
    fflush(stdout);

    return;  
} 


