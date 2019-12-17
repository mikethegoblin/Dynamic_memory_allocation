#include <stdio.h>
#include <stdlib.h>
#include "heapChecker.c"
#include <string.h>
#include <stdint.h>
#define align8(size) size = (((size - 1) >> 3) << 3) + 8 // 8 byte allignment. Makes sure size is divisible by 8
#define FTR(bp) (char *)bp + (*(mtdata *)bp & -2) - 4    // locate address of footer by the address of header
#define clear_first_4bytes(footer_data) (unsigned)(footer_data << 4) >> 4;
unsigned long long rdtsc(void)
{
    unsigned int tscl, tsch;
    __asm__ __volatile__("rdtsc"
                         : "=a"(tscl), "=d"(tsch));
    return ((unsigned long long)tsch << 32) | tscl;
}

typedef char *addrs_t;
typedef void *any_t;
typedef unsigned long uintptr; // used to convert address to unsigned long
typedef int mtdata;            // Stores size of block and size of padding

addrs_t M1;  // points to begining of heap area, the header of the first block
addrs_t end; // points to end of heap area
int Heap_size = 1 << 20; // defualt size of heap (1 meg)

void Init(size_t size)
{
    addrs_t baseptr = (addrs_t)malloc(size); // use system malloc to allocate a heap area of 1 meg
    if (!baseptr)
    {
        printf("System malloc failed, heap not initialized");
        return;
    }
    Heap_size = align8(size);
    end = baseptr + size - 4;        // initialize pointer end, the last 4 bytes are unusable due to alignment, so reserve for future use
    M1 = baseptr + 4;                // initialize pointer M1, the first 4 bytes act as an offset to the lowest free block
    *(mtdata *)M1 = end - M1;        // initialize header (entire heap is free at this point)
    *(mtdata *)(end - 4) = end - M1; // initialize footer
}

// addrs_t Malloc(size_t size)
// {
//     align8(size); // do arithmetic to make size divisible by 8
//     size += 8;    // include size of header and footer
//     // printf("size = %d\n", size);
//     int offset=*(int *)(M1-4);
//     addrs_t p = M1 + offset;
//     int temp = *(int *)p;
//     while ((p < end) && (temp & 1) || (temp < size)))
//     { // do first fit search
//         p += (*(int *)p & -2);
//         temp = *(int *)p;
//     }
//     if (p >= end)
//         return NULL;
//     else
//     {
//         if (temp >= size + 16)
//         { // if free area is larger than size + 16, split it into 2 separate area
//             addrs_t x = p + size;
//             *(mtdata *)(p + temp - 4) = temp - size;                               // set footer of available free area after splitting
//             *(mtdata *)p = size | 1;                                               // set header of allocated area (lsb = 1 indicates this area is allocated)
//             *(long long *)(x - 4) = (size | 1) | ((long long)(temp - size) << 32); // set footer and header of available free area after splitting
//             *(int *)(M1-4) = (p-M1);
//         }
//         else
//         {
//             *(mtdata *)p = temp | 1;
//             *(mtdata *)(p + temp - 4) = temp | 1;
//         }
//         return p + 4;
//     }
// }

addrs_t Malloc(size_t size) // total size of the block to be allocated
{
    //unsigned long long start, finish;
    unsigned long long start;
    start = rdtsc();
    ++Num_Malloc_req;
    int payload_size = size;
    align8(size);                      // do arithmetic to make size divisible by 8
    size += 8;                         // total size of the block includes header and footer
    addrs_t p = M1 + *(int *)(M1 - 4); // first block + offset
    int temp = *(int *)p;              // temp stores the mtdata at p (size of block p)
    while (1)
    {
        //if (!(temp & -3))
        if (p >= end)
        {
            ++Num_fali;
            
            Total_Mclock += rdtsc() - start;
            return NULL;
        }
        if (temp & 1)
        {
            //p += (*(int *)p & -2);
            p += (temp & -4);
            temp = *(int *)p | (temp & 2);
        }
        else if (temp < size)
        {
            p += (*(int *)p & -2);
            //p += (temp & -4);
            temp = *(int *)p | 2; // a flag for marking whether a free block has been skipped
        }
        else
            break;
    }
    if (temp >= size + 16)
    { // if free area is larger than size + 16, split it into 2 separate area
        if (temp & 2)
            temp &= -3;
        else
            *(int *)(M1 - 4) = (p - M1);
        addrs_t x = p + size;
        *(mtdata *)(p + temp - 4) = temp - size;                                                                 // set footer of available free area after splitting
        *(mtdata *)p = size | 1;                                                                                 // set header of allocated area (lsb = 1 indicates this area is allocated)
        *(long long *)(x - 4) = (size | 1 | (size - payload_size - 8 << 28)) | ((long long)(temp - size) << 32); // set footer of this allocated block and header of available free area after splitting
        // footer also stores the size of the padding in its most sig 4 bits, assumes the block < 256 MB
        Padded_bytes_allocated += size;
    }
    else
    {
        temp &= -3;
        *(mtdata *)p = temp | 1;
        *(mtdata *)(p + temp - 4) = temp | 1 | (temp - payload_size - 8 << 28);
        //printf("%d\n", temp - payload_size - 8);
        Padded_bytes_allocated += (temp & -4);
        --Num_Free_Blocks;
    }
    Raw_bytes_allocated += payload_size;
    ++Num_Alloc_Blocks;
    Total_Mclock += rdtsc() - start;
    return p + 4;
}

/*
void Free(addrs_t addr) {
    addr -= 4;
    // mtdata* p = (mtdata*)addr;
    mtdata header= *addr & -2;
    //mtdata* next = (mtdata*) (addr + header); // header of next block
    //mtdata* prev_f = (mtdata*)(addr - 4); // footer of previous block
    mtdata nextV = *(addr + header); //value at next
    mtdata prev_fV = *(addr - 4); //value at prev_f
    char temp=0;
    if ((addr + header)<end) temp |= !(nextV & 1);
    if (addr > M1) temp |= (!(prev_fV & 1))<<1;
    if (!temp){
        *addr = header & -2; // set lsb of header to 0, indicating this block is freed
        *(addr+header-4) &= -2; // set lsb of footer to 0
    }
    else if (temp==1){
        *addr = header + nextV; 
        *(addr+header+nextV-4) = header + nextV;
    }
    else if (temp==2){
        *(addr - prev_fV)=prev_fV+header;
        *(addr+header-4) = prev_fV+header;
    }
    else{
        *(addr - prev_fV)=prev_fV+header+nextV;
        *(addr+header+nextV-4) = prev_fV+header+nextV;
    }
    
}
*/

// void Free(addrs_t addr)
// {
//     addr -= 4;
//     mtdata *p = (mtdata *)addr;
//     *p &= -2;                              // set lsb of header to 0, indicating this block is freed
//     *(mtdata *)(FTR(addr)) &= -2;          // set lsb of footer to 0
//     mtdata *next = (mtdata *)(addr + *p);  // header of next block
//     mtdata *prev_f = (mtdata *)(addr - 4); // footer of previous block
//     if (addr + *p < end && !(*next & 1))
//     { // check if there is next block within heap and if the next block is free
//         *p += *next;
//         *(mtdata *)(FTR(p)) = *p;
//         // merge two blocks
//     }
//     if (addr > M1 && !(*prev_f & 1))
//     {                                                // check if there is previous block within heap and if previous block is free
//         mtdata *prev_h = (mtdata *)(addr - *prev_f); // locate header of previous block
//         *prev_h += *p;
//         *(mtdata *)(FTR(prev_h)) = *prev_h;
//         // merge previous block with current block
//     }
// }

// void Free(addrs_t addr)
// {
//     addr -= 4;
//     mtdata size = *(mtdata *)(addr) & -2, prevMeta = 1, nextMeta = 1;
//     addrs_t newHead = addr, newFoot = addr + size - 4;
//     if (addr > M1)
//         prevMeta = *(mtdata *)(addr - 4);
//     if (addr + size < end)
//         nextMeta = *(mtdata *)(addr + size);
//     if (!(prevMeta & 1))
//     {
//         size += prevMeta;
//         newHead = addr - prevMeta;
//     }
//     if (!(nextMeta & 1))
//     {
//         size += nextMeta;
//         newFoot += nextMeta;
//     }
//     *(mtdata *)newHead = size;
//     *(mtdata *)newFoot = size;
//     newHead -= M1;
//     if (newHead < *(int *)(M1 - 4))
//         *(int *)(M1 - 4) = newHead;
// }

void Free(addrs_t addr)
{
    unsigned long long start;
    start = rdtsc();
    ++Num_Free_req;
    if (!addr)
    {
        ++Num_fali;
        Total_Fclock += rdtsc() - start;
        return;
    }
    addr -= 4;
    mtdata size = *(mtdata *)(addr) & -2, prevMeta = 1, nextMeta = 1, Num_Free_Blocks_delta = 1;
    unsigned long long temp = *(unsigned long long *)(addr + size - 4);
    //addrs_t newHead = addr, newFoot = addr + size - 4;
    Padded_bytes_allocated -= size;
    Raw_bytes_allocated -= (size - ((temp >> 28) & 15) - 8);
    //if (Num_Malloc_req<12) printf("%lld\n", size - ((temp >> 28)&15) -8);
    addrs_t newFoot = addr + size - 4;
    if (addr > M1)
        prevMeta = *(mtdata *)(addr - 4);
    if (addr + size < end)
        nextMeta = temp >> 32;
    if (~prevMeta & 1)
    {
        size += prevMeta;
        addr = addr - prevMeta;
        --Num_Free_Blocks_delta;
    }
    if (~nextMeta & 1)
    {
        size += nextMeta;
        newFoot += nextMeta;
        --Num_Free_Blocks_delta;
    }
    Num_Free_Blocks += Num_Free_Blocks_delta;
    *(mtdata *)newFoot = size;
    *(mtdata *)addr = size;
    addr -= M1;
    if (addr < *(int *)(M1 - 4))
        *(int *)(M1 - 4) = addr;
    --Num_Alloc_Blocks;
    Total_Fclock += rdtsc() - start;
}

/*
addrs_t Put(any_t data, size_t size) {
    addrs_t p = Malloc(size); // allocate size block in heap
    if (p == NULL) { // if there is no free space in heap, return null
        return NULL;
    }
    addrs_t addr = p; // start address where data is going to be written
    char* str = (char*)data;
    int i = 0;
    while (i < size) { // write data byte by byte into heap
        *addr = *str;
        addr += 1;
        str += 1;
        i += 1;
    }
    return p; // return start address of payload (data area)
}
*/

addrs_t Put(any_t data, size_t size)
{
    addrs_t p = Malloc(size); // allocate size block in heap
    if (p)
    {
        memcpy(p, data, size);
    }

    return p;
}

void Get(any_t return_data, addrs_t addr, size_t size)
{
    if (addr)
    {
        memcpy(return_data, addr, size);
        Free(addr); // free current block
    }
    else
        ++Num_fali;
}
