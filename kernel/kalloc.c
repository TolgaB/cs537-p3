// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"

struct run {
  struct run *next;
  int hasNext;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

extern char end[]; // first address after kernel loaded from ELF file
int kinitUse;
int dumpFrames[512];
int numDumpFrames;
int totalArr;
// Initialize free list of physical pages.
void
kinit(void)
{
  char *p;
  initlock(&kmem.lock, "kmem");
  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE) {
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  acquire(&kmem.lock);
  for (int i = 0; i < 512; i++) {
         if (dumpFrames[i] == (int) v) {
                dumpFrames[i] = 0;
                numDumpFrames--;
                break;
         }
  }
  r = (struct run*)v;
  if (r->hasNext == 1) {
	  (r->next)->next = kmem.freelist;
	  kmem.freelist = r;
  } else {
  	r->next = kmem.freelist;
  	kmem.freelist = r;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
      if ((r->next)->next) {
      	r->hasNext = 1;
      	kmem.freelist = (r->next)->next;
      } else {
	      //cant follow the alternate allocation scheme so return null
	      return NULL;
      }
  }
  for (int i = totalArr; i < 512; i++) {
          if (dumpFrames[i] == 0) {
                  dumpFrames[i] = (int)r;
                  numDumpFrames++;
                  totalArr++;
                  break;
          }
  }
  release(&kmem.lock);
  return (char*)r;
}

int dump_allocated_helper(int *frames, int numframes) {
        if ((numframes > numDumpFrames) || (numframes <= 0)) {
                return -1;
        }
        int j = 0;
        for (int i = 511; i >= 0; i--) {
                if (dumpFrames[i] != 0) {
                        frames[j] = dumpFrames[i];
                        j++;
                        if (numframes <= j) {
                                break;
                        }
                }
        }
        return 0;
}
