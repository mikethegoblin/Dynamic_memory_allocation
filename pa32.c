#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "heapChecker.c"
#define align8(size) size = (((size-1) >> 3) << 3) + 8 // 8 byte allignment. Makes sure size is divisible by 8
#define FTR(bp) (char*)bp + (*(int*)bp & -2) - 4 // locate address of footer by the address of header
#define clear3(num) (num << 3) >> 3

typedef char *addrs_t; 
typedef void *any_t;
typedef unsigned long uintptr; // used to convert address to unsigned long
typedef struct mcb { // structure mcb (malloc block)
    unsigned int size; // size of block
    int index; // index in RT that stores the address of this block
} mcb;
unsigned long long rdtsc (void) { 
    unsigned int tscl, tsch; 
    __asm__ __volatile__("rdtsc":"=a"(tscl),"=d"(tsch)); 
    return ((unsigned long long)tsch << 32)|tscl; 
}

addrs_t M2;
addrs_t end;
addrs_t RT[1<<18];
int Heap_size = 1<<20;


void VInit(size_t size) {
    addrs_t baseptr;
    baseptr = (addrs_t)malloc(size);
    end = baseptr + size;
    // uintptr base_addr = baseptr;
    // align8(base_addr);
    // baseptr = base_addr;
    M2 = baseptr + 4;
    *(addrs_t*)M2 = M2 + 12; // base address of the free area of heap
    *(int*)(M2 + 8) = 0; // store lowest index in RT that is empty
    Padded_bytes_allocated = 16;
}

addrs_t *VMalloc(size_t size) {
    unsigned long start, finish;
    start = rdtsc();
    Num_Malloc_req += 1;
    addrs_t Free = *(addrs_t*)M2; // base address of free heap area
    //printf("Free at %x\n", Free);
    int alloc_size = size;
    align8(size); // do arithmetic to make sure block is 8 byte aligned
    size += 8;
    if (end - Free < size) { // check if there is still enough space for allocation
        Num_fali += 1;
        finish = rdtsc();
        Total_Mclock += finish - start;
        return NULL;
    }
    Num_Alloc_Blocks += 1;
    Raw_bytes_allocated += alloc_size;
    Padded_bytes_allocated += size;
    int padding = size - 8 - alloc_size; // calculate size of padding
    mcb *addr = Free; // initialize pointer of mcb structure
    addr->size = size; // store size of the block
    addr->size |= (padding << 29); // store size of padding in first 3 significant bit
    addr->index = *(int*)(M2 + 8); //record index of RT that stores the address of this block
    RT[addr->index] = (addrs_t)addr + 8; // store the base address of payload in RT
    int i;
    for (i = addr->index; i < sizeof(RT); i++) { // find next lowest empty entry in RT
        if (RT[i] == NULL) {
            *(int*)(M2 + 8) = i;
            break; 
        }
    } 
    *(addrs_t*)M2 = Free + size; // update base address of free heap area
    if (Free + size == end) Num_Free_Blocks = 0;
    finish = rdtsc();
    Total_Mclock += finish - start;
    return RT + addr->index; // Pointer arith
}

void VFree(addrs_t *addr) {
    unsigned long start, finish;
    start = rdtsc();
    Num_Free_req += 1;
    if (addr) {
        Num_Alloc_Blocks -= 1;
        Num_Free_Blocks = 1;
        addrs_t Free = *(addrs_t*)M2; // get base address of free area
        mcb *cur = *addr - 8; // get pointer to mcb structure
        *addr = NULL; // set entry in RT to NULL
        int padding = (cur->size) >> 29; // get size of padding in block
        int cur_size = clear3(cur->size); // get real size of block
        Raw_bytes_allocated -= (cur_size - 8 - padding);
        Padded_bytes_allocated -= cur_size;
        if (addr < (RT + *(int*)(M2 + 8))) { // if this freed entry has lowest index of all empty entries in RT, update record for empty entry 
            *(int*)(M2 + 8) = addr - RT; // addr - RT gives the index (this is the lowest index in RT that has empty entry)
        }
        mcb *next_1 = (addrs_t)cur + cur_size; // get base address of next block
        //printf("%x\n", Free);
        while (next_1 !=  Free) { // do heap compaction if there are allocated blocks after currently freed block
            //printf("in compaction\n");
            //printf("%x\n", next_1);
            int next_1_size = clear3(next_1->size);
            mcb *next_2 = (addrs_t)next_1 + next_1_size;
            memcpy(cur, next_1, next_1_size);
            RT[cur->index] = (addrs_t)cur + 8;
            cur = (addrs_t)cur + next_1_size;
            next_1 = next_2;
            //printf("%x\n", next_1);
            //printf("out of compaction\n");
        }
        *(addrs_t*)M2 = cur; // update address of free area
        //printf("%x\n", cur);
    }
    else {
        Num_fali += 1;
    }
    finish = rdtsc();
    Total_Fclock += finish - start;
}

addrs_t *VPut(any_t data, size_t size) {
    addrs_t *addr = VMalloc(size); // call VMalloc
    if (addr) {
        memcpy(*addr, data, size); // copy size bytes of data to malloced area
    }
    return addr;

}

void VGet (any_t return_data, addrs_t *addr, size_t size) {
    if (addr) {
        memcpy(return_data, *addr, size); // copy size bytes of data from heap to return_data
        VFree(addr); // free this block
    }
    else {
        Num_fali += 1;
    }
}
