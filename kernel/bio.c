// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define lock_num 13
#define hash_base 59
#define hash_dev(a, b) ((a * hash_base + b) % lock_num)

struct {
  struct spinlock lock[lock_num];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i = 0; i < lock_num; i++) {
    initlock(&bcache.lock[i], "bcache.bucket");
  }

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  int hash_id = hash_dev(dev, blockno);
  struct buf *b;

  acquire(&bcache.lock[hash_id]);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hash_id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[hash_id]);


  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i = 0; i < lock_num; i++) {
    acquire(&bcache.lock[i]);
    for(b = bcache.head.prev; b != &bcache.head; b = b->prev) {
      if(hash_dev(b->dev, b->blockno) == i) {
        if(b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          release(&bcache.lock[i]);
          acquiresleep(&b->lock);
          return b;
        }
      }
    }
    release(&bcache.lock[i]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int hash_id = hash_dev(b->dev, b->blockno);

  acquire(&bcache.lock[hash_id]);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  release(&bcache.lock[hash_id]);
}

void
bpin(struct buf *b) {
  int hash_id = hash_dev(b->dev, b->blockno);
  acquire(&bcache.lock[hash_id]);
  b->refcnt++;
  release(&bcache.lock[hash_id]);
}

void
bunpin(struct buf *b) {
  int hash_id = hash_dev(b->dev, b->blockno);
  acquire(&bcache.lock[hash_id]);
  b->refcnt--;
  release(&bcache.lock[hash_id]);
}