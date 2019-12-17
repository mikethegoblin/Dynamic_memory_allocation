#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define KBLU  "\x1B[34m"
#define KRED  "\x1B[31m"
#define KRESET "\x1B[0m"

#define ERROR_OUT_OF_MEM    0x1
#define ERROR_DATA_INCON    0x2
#define ERROR_ALIGMENT      0x4
#define ERROR_NOT_FF        0x8
#define ERROR_VMALLOC       0x10
#define ERROR_VFREE         0x20

#define ALIGN 8

//#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x)))

// This should be used on x86-64 as the above rdtsc() fn only works on IA32
#define RDTSC(var)                                              \
  {                                                             \
    uint32_t var##_lo, var##_hi;                                \
    asm volatile("lfence\n\trdtsc" : "=a"(var##_lo), "=d"(var##_hi));     \
    var = var##_hi;                                             \
    var <<= 32;                                                 \
    var |= var##_lo;                                            \
  }



#ifdef VHEAP
  #include "pa32.c" // <-- Include the solution for part 2
  #define TESTSUITE_STR         "Virtualized Heap"
  #define INIT(msize)           VInit(msize)
  #define MALLOC(msize)         VMalloc(msize)
  #define FREE(addr)            VFree(addr)
  #define PUT(data,size)        VPut(data,size)
  #define GET(rt,addr,size)     VGet(rt,addr,size)
  #define ADDRS                 addrs_t*
  #define LOCATION_OF(addr)     ((size_t)(*addr))
  #define DATA_OF(addr)         (*(*(addr)))
#else
  #include "pa31.c" // <-- Include solution for part 1
  #define TESTSUITE_STR         "Heap"
  #define INIT(msize)           Init(msize)
  #define MALLOC(msize)         Malloc(msize)
  #define FREE(addr)            Free(addr)
  #define PUT(data,size)        Put(data,size)
  #define GET(rt,addr,size)     Get(rt,addr,size)
  #define ADDRS                 addrs_t
  #define LOCATION_OF(addr)     ((size_t)addr)
  #define DATA_OF(addr)         (*(addr))
#endif

void print_testResult(int code){
  if (code){
    printf("[%sFailed%s] due to: ",KRED,KRESET);
    if (code & ERROR_OUT_OF_MEM)
      printf("<OUT_OF_MEM>");
    if (code & ERROR_DATA_INCON)
      printf("<DATA_INCONSISTENCY>");
    if (code & ERROR_ALIGMENT)
      printf("<ALIGMENT>");
    if (code & ERROR_VMALLOC)
      printf("<VMALLOC>");
    if (code & ERROR_VFREE)
      printf("<VFREE>");
    printf("\n");
  }else{
    printf("[%sPassed%s]\n",KBLU, KRESET);
  }
}

int test_stability(int numIterations, unsigned long* tot_alloc_time, unsigned long* tot_free_time){
  int i, n, res = 0;
  char s[80];
  ADDRS addr1;
  ADDRS addr2;
  char data[80];
  char data2[80];

  unsigned long start, finish;
  *tot_alloc_time = 0;
  *tot_free_time = 0;

  for (i = 0; i < numIterations; i++) {
    n = sprintf (s, "String 1, the current count is %d\n", i);
    RDTSC(start);
    addr1 = PUT(s, n+1);
    RDTSC(finish);
    *tot_alloc_time += finish - start;
    addr2 = PUT(s, n+1);
    // Check for heap overflow
    if (!addr1 || !addr2){
      res |= ERROR_OUT_OF_MEM;
      break;
    }
    // Check aligment
    if ((LOCATION_OF(addr1) & (ALIGN-1)) || (LOCATION_OF(addr2) & (ALIGN-1)))
      res |= ERROR_ALIGMENT;
    // Check for data consistency
    RDTSC(start);
    GET((any_t)data2, addr2, n+1);
    RDTSC(finish);
    *tot_free_time += finish - start;
    GET((any_t)data, addr1, n+1);
    if (strcmp(data,data2))
      res |= ERROR_DATA_INCON;
  }
  return res;
}

int test_ff(){
  int err = 0;
  // Round 1 - 2 consequtive allocations should be allocated after one another
  ADDRS v1;
  ADDRS v2;
  ADDRS v3;
  ADDRS v4;
  v1 = MALLOC(8);
  v2 = MALLOC(4);
  if (LOCATION_OF(v1) >= LOCATION_OF(v2))
    err |= ERROR_NOT_FF;
  if ((LOCATION_OF(v1) & (ALIGN-1)) || (LOCATION_OF(v2) & (ALIGN-1)))
    err |= ERROR_ALIGMENT;
  // Round 2 - New allocation should be placed in a free block at top if fits
  FREE(v1);
  v3 = MALLOC(64);
  v4 = MALLOC(5);
  if (LOCATION_OF(v4) != LOCATION_OF(v1) || LOCATION_OF(v3) < LOCATION_OF(v2))
    err |= ERROR_NOT_FF;
  if ((LOCATION_OF(v3) & (ALIGN-1)) || (LOCATION_OF(v4) & (ALIGN-1)))
    err |= ERROR_ALIGMENT;
  // Round 3 - Correct merge
  FREE(v4);
  FREE(v2);
  v4 = MALLOC(10);
  if (LOCATION_OF(v4) != LOCATION_OF(v1))
    err |= ERROR_NOT_FF;
  // Round 4 - Correct Merge 2
  FREE(v4);
  FREE(v3);
  v4 = MALLOC(256);
  if (LOCATION_OF(v4) != LOCATION_OF(v1))
    err |= ERROR_NOT_FF;
  // Clean-up
  FREE(v4);
  return err;
}

int test_maxNumOfAlloc(){
  int count = 0;
  char *d = "x";
  const int testCap = 1000000;
  ADDRS allocs[testCap];

  while ((allocs[count]=PUT(d,1)) && count < testCap){
    if (DATA_OF(allocs[count])!='x') break;
    count++;
  }
  // Clean-up
  int i;
  for (i = 0 ; i < count ; i++)
    FREE(allocs[i]);
  return count;
}

int test_maxSizeOfAlloc(int size){
  char* d = "x";
  if (!size) return 0;
  ADDRS v1 = MALLOC(size);
  if (v1){
    return size + test_maxSizeOfAlloc(size>>1);
  }else{
    return test_maxSizeOfAlloc(size>>1);
  }
}

int test_vheap(size_t mem_size){
  int err = 0;
  // Round 1 - 2 consequtive allocations should be allocated after one another
  ADDRS v1;
  ADDRS v2;
  ADDRS v3;
  ADDRS v4;
  ADDRS v5;
  size_t l1, l2, l3, l4;
  v1 = MALLOC(64);
  v2 = MALLOC(250);
  v3 = MALLOC(500);
  v4 = MALLOC(128);

  l1 = LOCATION_OF(v1);
  l2 = LOCATION_OF(v2);
  l3 = LOCATION_OF(v3);
  l4 = LOCATION_OF(v4);


  if ((LOCATION_OF(v1) >= LOCATION_OF(v2)) ||
      (LOCATION_OF(v2) >= LOCATION_OF(v3)) ||
      (LOCATION_OF(v3) >= LOCATION_OF(v4)))
    err |= ERROR_VMALLOC;
  if ((LOCATION_OF(v1) & (ALIGN-1)) ||
      (LOCATION_OF(v2) & (ALIGN-1)) ||
      (LOCATION_OF(v3) & (ALIGN-1)) ||
      (LOCATION_OF(v4) & (ALIGN-1)))
    err |= ERROR_ALIGMENT;
  // Round 2 - Defragmentation test
  FREE(v3);
  if ((LOCATION_OF(v4) != l3) ||
      (LOCATION_OF(v1) != l1) ||
      (LOCATION_OF(v2) != l2) ){
    err |= ERROR_VFREE;
  }
  FREE(v1);
  if ((LOCATION_OF(v2) != l1) || (LOCATION_OF(v4) - l1 < 256) || (LOCATION_OF(v4) - l1 > 300)){
    err |= ERROR_VFREE;
  }
  // Round 3 - Allocation again
  v5 = MALLOC(mem_size / 2);
  if (LOCATION_OF(v5) & (ALIGN - 1))
    err |= ERROR_ALIGMENT;
  if (LOCATION_OF(v5) < LOCATION_OF(v4) + 128)
    err |= ERROR_VFREE;

  FREE(v5);
  FREE(v2);
  FREE(v4);
  return err;
}

int test_double_free(){
  ADDRS v1 = MALLOC(50);
  int i;
  for (i = 0 ; i < 100; i++)
    FREE(v1);
}

int test_integrity_after_vfree() {
  int res = 0;

  ADDRS v1;
  ADDRS v2;
  ADDRS v3;
  ADDRS v4;

  char s[52];
  char data1[52];
  char data2[52];
  char data3[52];
  char data4[52];
  int i;
  for(i = 0; i < 52; i++) {
    s[i] = 'a' + i;
    s[i + 26] = 'A' + i;
  }

  v1 = PUT(s, 16);
  v2 = PUT(s + 16, 10);
  v3 = PUT(s + 26, 20);
  v4 = PUT(s + 46, 6);

  FREE(v2);
  GET((any_t) data3, v3, 20);
  GET((any_t) data1, v1, 16);
  GET((any_t) data4, v4, 6);

  if(strncmp(s, data1, 16) || strncmp(s + 26, data3, 20) || strncmp(s + 46, data4, 6)) {
    res |= ERROR_DATA_INCON;
  }
  
  return res;
}

void test_memleak() {
  int res = 0;

  ADDRS v1, v2, v3, v4, v5;

  v1 = MALLOC(20);
  v2 = MALLOC(28);
  v3 = MALLOC(56);
  v4 = MALLOC(89);
  v5 = MALLOC(65);

  FREE(v2); FREE(v5); FREE(v4); FREE(v1); FREE(v3);
}

int main (int argc, char **argv) {
  int res;
  unsigned mem_size = (1<<20); // Default
  // Parse the arguments
  if (argc > 2){
    fprintf(stderr, "Usage: %s [buffer size in bytes]\n",argv[0]);
    exit(1);
  }else if (argc == 2){
    mem_size = atoi(argv[1]);
  }

  printf("Evaluating a %s of %d KBs...\n",TESTSUITE_STR, mem_size/(1 << 10));

  unsigned long tot_alloc_time, tot_free_time;
  int numIterations = 1000000;
  // Initialize the heap
  INIT(mem_size);
#ifndef MEMLEAK
  // Test 1
  printf("Test 1 - Stability and consistency:\t");
  print_testResult(test_stability(numIterations,&tot_alloc_time,&tot_free_time));
  printf("\tAverage clock cycles for a Malloc request: %lu\n",tot_alloc_time/numIterations);
  printf("\tAverage clock cycles for a Free request: %lu\n",tot_free_time/numIterations);
  printf("\tTotal clock cycles for %d Malloc/Free requests: %lu\n",numIterations,tot_alloc_time+tot_free_time);
  // Test 2
  #ifndef VHEAP
  printf("Test 2 - First-fit policy:\t\t");
  print_testResult(test_ff());
  #else
  printf("Test 2a - VHeap alloc./compaction:\t");
  print_testResult(test_vheap(mem_size));
  printf("Test 2b - Integrity\t");
  print_testResult(test_integrity_after_vfree());
  #endif
  // Test 3:
  printf("Test 3 - Max # of 1 byte allocations:\t");
  printf("[%s%i%s]\n",KBLU,test_maxNumOfAlloc(), KRESET);
  // Test 4:
  printf("Test 4 - Max allocation size:\t\t");
  printf("[%s%i KB%s]\n", KBLU, test_maxSizeOfAlloc(4*1024*1024)>>10, KRESET);
  // Test 5: Seg fault due to double free
  printf("Test 5 - Double free:\n");
  test_double_free();
  printf("FINISHED\n");
#endif

#ifdef MEMLEAK
  printf("Test Memory Leak:\n"
         "Heapchecker output from Student (Insert student's heapchecker call)\n");
  print_heap_stats();
  test_memleak();
  print_heap_stats();
  printf("Heapchecker output again (Insert student's heapchecker)\n");
#endif
  return 0;
}
