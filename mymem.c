#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>

/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */

struct memoryList
{
  // doubly-linked list
  struct memoryList *last;
  struct memoryList *next;

  int size;            // How many bytes in this block?
  char alloc;          // 1 if this block is allocated,
                       // 0 if this block is free.
  void *ptr;           // location of block in memory pool.
};

/* Prototypes for helper functions */
struct memoryList* first_block(size_t requested);
struct memoryList* best_block(size_t requested);
struct memoryList* worst_block(size_t requested);
struct memoryList* next_block(size_t requested);

strategies myStrategy = NotSet;    // Current strategy

size_t mySize;
void *myMemory = NULL;

static struct memoryList *head;
static struct memoryList *next;

/* initmem must be called prior to mymalloc and myfree.

   initmem may be called more than once in a given execution;
   when this occurs, all memory you previously malloc'ed  *must* be freed,
   including any existing bookkeeping data.

   strategy must be one of the following:
   - "best" (best-fit)
   - "worst" (worst-fit)
   - "first" (first-fit)
   - "next" (next-fit)
   sz specifies the number of bytes that will be available, in total,
   for all mymalloc requests.
*/
void initmem(strategies strategy, size_t sz) {
  myStrategy = strategy;

  /* All implementations will need an actual block of memory to use */
  mySize = sz;

  /* Release anything used by a memory-management process already under way */
  if(myMemory) free(myMemory);
  if(head) free(head);
  if(next) free(next);

  /* Initialize memory management structures */
  myMemory = malloc(sz);
  head = malloc(sizeof(struct memoryList));
  head->size = sz;
  head->alloc = 0;
  head->ptr = myMemory;

  /* Memory list should be circular for next-fit */
  head->last = head;
  head->next = head;
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1 
 */

void *mymalloc(size_t requested) {
  assert((int)myStrategy > 0);

  struct memoryList* block = NULL;
  switch (myStrategy) {
    case First:
      block = first_block(requested);
      break;
    case Best:
      block = best_block(requested);
      break;
    case Worst:
      block = worst_block(requested);
      break;
    case Next:
      block = next_block(requested);
      break;
    default:
      /* Strategy not set */
      fprintf(stderr, "myalloc: Invalid memory management strategy!\n");
      return NULL;
  }

  if(!block) {
    /* No valid blocks */
    fprintf(stderr, "myalloc: No available blocks of size %lu!\n", requested);
    return NULL;
  }

  if(block->size > requested) {
    /* Construct container for unallocated remainder of this block */
    struct memoryList* remainder = malloc(sizeof(struct memoryList));
    
    /* Insert into linked list */
    remainder->next = block->next;
    remainder->next->last = remainder;
    remainder->last = block;
    block->next = remainder;
    
    /* Divide up allocated memory */
    remainder->size = block->size - requested;
    remainder->alloc = 0;
    remainder->ptr = block->ptr + requested;
    block->size = requested;
  }
  
  block->alloc = 1;
  
  /* Return pointer to the allocated block */
  return block->ptr;
}

/* Find the first available block of memory larger than the requested size. */
struct memoryList* first_block(size_t requested) {

  /* Iterate over memory list, searching for the target block's container */
  struct memoryList* index = head;
  do {
    if(!(index->alloc) && index->size >= requested) {
      return index;
    }
  } while((index = index->next) != head);

  /* No suitable block in the memory list */
  return NULL;
}

/* Find the smallest available block larger than the requested size. */
struct memoryList* best_block(size_t requested) {
  // TODO
  return NULL;
}

/* Find the largest available block larger than the requested size. */
struct memoryList* worst_block(size_t requested) {
  // TODO
  return NULL;
}

/* Find the first suitable block after the last block allocated. */
struct memoryList* next_block(size_t requested) {
  // TODO
  return NULL;
}

/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void* block) {

  /* Iterate over memory list, searching for the target block's container */
  struct memoryList* container = head;
  do {
    if(container->ptr == block) {
      break;
    }
  } while((container = container->next) != head);

  /* Flag this block as freed */
  container->alloc = 0;

  /* If the previous block is also free, reduce to one contiguous block. */
  if(container != head && !(container->last->alloc)) {
    struct memoryList* prev = container->last;
    prev->next = container->next;
    prev->next->last = prev;
    prev->size += container->size;
    free(container);
    container = prev;
  }

  /* If the next block is also free, reduce to one contiguous block. */
  if(container->next != head && !(container->next->alloc)) {
    struct memoryList* next = container->next;
    container->next = next->next;
    container->next->last = container;
    container->size += next->size;
    free(next);
  }
}

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when we refer to "memory" here, we mean the 
 * memory pool this module manages via initmem/mymalloc/myfree. 
 */

/* Get the number of contiguous areas of free space in memory. */
int mem_holes() {
  return mem_small_free(mySize + 1);
}

/* Get the number of bytes allocated */
int mem_allocated() {
  return mySize - mem_free();
}

/* Number of non-allocated bytes */
int mem_free() {    
  int count = 0;

  /* Iterate over memory list */
  struct memoryList* index = head;
  do {
    /* Add to total only if this block isn't allocated */
    if(!(index->alloc)) {
      count += index->size;
    }
  } while((index = index->next) != head);

  return count;
}

/* Number of bytes in the largest contiguous area of unallocated memory */
int mem_largest_free() {
  
  int max_size = 0;

  /* Iterate over memory list */
  struct memoryList* index = head;
  do {
    if(!(index->alloc) && index->size > max_size) {
      max_size = index->size;
    }
  } while((index = index->next) != head);

  return max_size;
}

/* Number of free blocks smaller than "size" bytes. */
int mem_small_free(int size) {
  
  int count = 0;

  /* Iterate over memory list */
  struct memoryList* index = head;
  do {
    /* Add to total only if this block's size is less than the target */
    if(index->size <= size) {
      count += !(index->alloc);
    }
  } while((index = index->next) != head);

  return count;
}       

/* Is a particular byte allocated or not?
 * A necessary precondition of this function is that ptr points to somewhere in
 * the block allocated for our memory list. In fact, C99 defines pointer
 * comparison only for pointers that point to parts of the same memory object.
 */
char mem_is_alloc(void *ptr) {

  /* Iterate over the memory list */
  struct memoryList* index = head;
  while(index->next != head) {
    /* If the next block's ptr is after the target,
       the target must be in this block */
    if(ptr < index->next->ptr) {
      return index->alloc;
    }
    index = index->next;
  }

  /* Iterator is now at the last block, so we assume the target is here */
  return index->alloc;
}

/* 
 * Feel free to use these functions, but do not modify them.  
 * The test code uses them, but you may find them useful.
 */


// Returns a pointer to the memory pool.
void *mem_pool() {
  return myMemory;
}

// Returns the total number of bytes in the memory pool.
int mem_total() {
  return mySize;
}


// Get string name for a strategy. 
char *strategy_name(strategies strategy) {
  switch (strategy) {
    case Best:
      return "best";
    case Worst:
      return "worst";
    case First:
      return "first";
    case Next:
      return "next";
    default:
      return "unknown";
  }
}

// Get strategy from name.
strategies strategyFromString(char * strategy) {
  if (!strcmp(strategy,"best")) {
    return Best;
  } else if (!strcmp(strategy,"worst")) {
    return Worst;
  } else if (!strcmp(strategy,"first")) {
    return First;
  } else if (!strcmp(strategy,"next")) {
    return Next;
  } else {
    return 0;
  }
}

/* 
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */

/* Use this function to print out the current contents of memory. */
void print_memory() {
  printf("Memory List {\n");
  /* Iterate over memory list */
  struct memoryList* index = head;
  do {
    printf("\tBlock %p,\tsize %d,\t%s\n",
           index->ptr,
           index->size,
           (index->alloc ? "[ALLOCATED]" : "[FREE]"));
  } while((index = index->next) != head);
  printf("}\n");
}

/* Use this function to track memory allocation performance.  
 * This function does not depend on your implementation, 
 * but on the functions you wrote above.
 */ 
void print_memory_status() {
  printf("%d out of %d bytes allocated.\n",mem_allocated(),mem_total());
  printf("%d bytes are free in %d holes; maximum allocatable block is %d bytes.\n",mem_free(),mem_holes(),mem_largest_free());
  printf("Average hole size is %f.\n\n",((float)mem_free())/mem_holes());
}

/* Use this function to see what happens when your malloc and free
 * implementations are called.  Run "mem -try <args>" to call this function.
 * We have given you a simple example to start.
 */
void try_mymem(int argc, char **argv) {
  strategies strat;
  void *a, *b, *c, *d, *e;
  if(argc > 1) {
    strat = strategyFromString(argv[1]);
  } else {
    strat = First;
  }
	
  /* A simple example.  
     Each algorithm should produce a different layout. */
	
  initmem(strat,500);
	
  a = mymalloc(100);
  b = mymalloc(100);
  c = mymalloc(100);
  myfree(b);
  d = mymalloc(50);
  myfree(a);
  e = mymalloc(25);
	
  print_memory();
  print_memory_status();
	
}
