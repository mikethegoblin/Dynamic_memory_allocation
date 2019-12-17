// #include "testsuite.c"

/*
<<Part 1 for Region M1>>
Number of allocated blocks : XXXX // 2 bytes
Number of free blocks  : XXXX (discounting padding bytes) // 2 bytes
Raw total number of bytes allocated : XXXX (which is the actual total bytes requested) // 4 bytes
Padded total number of bytes allocated : XXXX (which is the total bytes requested plus internally fragmented blocks wasted due to padding/alignment) // 4 bytes / 2 bytes
Raw total number of bytes free : XXXX // 
Aligned total number of bytes free : XXXX (which is sizeof(M1) minus the padded total number of bytes allocated. You should account for meta-datastructures inside M1 also)
Total number of Malloc requests : XXXX
Total number of Free requests: XXXX
Total number of request failures: XXXX (which were unable to satisfy the allocation or de-allocation requests)
Average clock cycles for a Malloc request: XXXX
Average clock cycles for a Free request: XXXX
Total clock cycles for all requests: XXXX

<<Part 2 for Region M2>>
Number of allocated blocks : XXXX
Number of free blocks  : XXXX
Raw total number of bytes allocated : XXXX
Padded total number of bytes allocated : XXXX
Raw total number of bytes free : XXXX
Aligned total number of bytes free : XXXX (which is sizeof(M2) minus the padded total number of bytes allocated. You should account for meta-datastructures inside M2 also)
Total number of VMalloc requests : XXXX
Total number of VFree requests: XXXX
Total number of request failures: XXXX
Average clock cycles for a VMalloc request: XXXX
Average clock cycles for a VFree request: XXXX
Total clock cycles for all requests: XXXX
*/
long long Num_Alloc_Blocks = 0;
long long Num_Free_Blocks = 1;        // initialy 1
long long Raw_bytes_allocated = 0;    // initially 0
long long Padded_bytes_allocated = 8; // initially 8 because 4 bytes are wasted at both ends due to alignment
long long Raw_free = 0;  // initially 1M-padding-4 because the first 8 bytes ad the last 4 bytes are used
long long Aligned_free = 0;  //total size - padded alloc - 8
long long Num_Malloc_req = 0; // ++ whenever Put is called
long long Num_Free_req = 0;
long long Num_fali = 0;
long long Total_Mclock = 0;
long long Total_Fclock = 0;

void print_heap_stats(void)
{
#ifdef VHEAP
    Raw_free = (1<<20) - Raw_bytes_allocated - (Num_Alloc_Blocks << 3) - 12;
    Aligned_free = (1<<20) - Padded_bytes_allocated;
    printf("<<Part 2 for Region M2>>\n");
#else
    Aligned_free = (1<<20) - Padded_bytes_allocated;
    Raw_free = (1<<20) - Raw_bytes_allocated - (Num_Alloc_Blocks + Num_Free_Blocks << 3) - 4;
    printf("<<Part 1 for Region M1>>\n");
#endif
    printf("Number of allocated blocks : %lld\n", Num_Alloc_Blocks);
    printf("Number of free blocks : %lld\n", Num_Free_Blocks);
    printf("Raw total number of bytes allocated : %lld\n", Raw_bytes_allocated);
    printf("Padded total number of bytes allocated : %lld\n", Padded_bytes_allocated);
    printf("Raw total number of bytes free : %lld\n", Raw_free);
    printf("Aligned total number of bytes free : %lld\n", Aligned_free);
    printf("Total number of Malloc requests : %lld\n", Num_Malloc_req);
    printf("Total number of Free requests: %lld\n", Num_Free_req);
    printf("Total number of request failures: %lld\n", Num_fali);
    // printf("Average clock cycles for a Malloc request: %lld\n", Total_Mclock / Num_Malloc_req);
    // printf("Average clock cycles for a Free request: %lld\n", Total_Fclock / Num_Free_req);
    // printf("Total clock cycles for all requests: %lld\n", Total_Mclock+Total_Fclock);
    /* 
Aligned total number of bytes free : XXXX (which is sizeof(M1) minus the padded total number of bytes allocated. You should account for meta-datastructures inside M1 also)
Total number of Malloc requests : XXXX
Total number of Free requests: XXXX
Total number of request failures: XXXX (which were unable to satisfy the allocation or de-allocation requests)
Average clock cycles for a Malloc request: XXXX
Average clock cycles for a Free request: XXXX
Total clock cycles for all requests: XXXX
*/
}
