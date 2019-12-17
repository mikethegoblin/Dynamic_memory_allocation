Final Project, Basic Memory Management Systems developed by
Zeyu Gu, BU id: U94989001
and Junyu Liu, BU id: U55856281

Included in this final project are two types of heap management systems: 
pa31.c (part 1) implements a basic memory management system that uses impilicit list and first fit policy.

pa32.c (part 2) implements a virtualized heap allocation scheme, where a redirection table is used to facilitate 
memory access and management. A compaction scheme is used whenever VFree is called, maximizing the size of usable
free region and insuring that the number of usable free region is at most 1.

heapChecker.c records several statistics that the user might be interested in; it does so by including a few variables 
that are updated accordingly whenever Malloc/VMalloc or Free/VFree is called. The user can call the print_heap_stats()
function included in heapChecker.c to print out these statistics.

-------WARNING-------: pa31.c, pa32.c, and heapChecker.c defined many things including rdtsc() as required by testing, 
there might be definition conflicts if linked with other files.


------Limitations------
Most of the code is written and tests with a 1 MB heap in mind, although it can potentially manage more.
The heap management scheme developed in part 1 should be able to handle heap objects of size smaller than 
256 MB, and any total heap size smaller than 4 GB.

The heap management scheme developed in part 2 should be able to handle up to 2^18 heap objects, but any
single object must be smaller than 512 MB, and the total heap size should be smaller than 4 GB.


-----------More details----------
Given the constrains on the project, we didn't think much optimization/variation is possible at the beginning, but we
were surprised. 

We made quite a lot of revisions even after we had our first working version of both parts, some of the progress is 
recorded in pa31.c as commented out older version of code. Some revisions are for speed/space optimization, others 
for accomadating heapChecker, and still others for bug fixing. 

For part 1, we originally intended to use 4-byte int for the impilicit list, placing a 4-byte header and 4-byte footer
for each block that records its size and free/allocated status. Every header and footer should be on an address that's
divisible by 4 but not by 8, so the payload would be 8-byte aligned, and we achieved this by reserving 4 bytes at both
ends of the heap. Later we developed an optimization scheme by storing the offset of the lowest free block in the 4 bytes
reserved at the start of the heap; this should reduce the first fit search time by 50% on average. 
We also tried to reduce the number of memory accesses by using temp variables.

Later when writting heapChecker, we realized that the padding size of each block needs to be recorded in order to update
raw_total_bytes_allocated accurately. Therefore, we decided to sacrifice the most significant 4 bits of each footer to
store the padding size information, thus the 256 MB block size limitation. We also thought of a strategy to store padding
information without sacrificing header/footer bits; we didn't have time to implement it, but here's the basic idea:
The second last bit of the footer should act as a flag bit for indicating whether padding is used, and the last padding 
byte should store padding size info if it exists. 


For the second part, we used 4 bytes padding at the beginning of the heap. Following those 4 bytes is an 8-byte space 
used to store the base address of the free area of the heap. The next 4 byte is used to store the lowest index in RT that 
has an empty entry. In this case we used 16 bytes of heap space before allocating the actual data. Each allocated block 
has a 4-byte size and 4-byte index and a payload block that has size divisible by 8. The index component is used to 
store the index in RT where the base address of the current block is stored, so we can update the entries in RT in 
constant time. The first 3 significant bits of the size component is used to store the padding size in this block, this 
makes it easier to update the raw_total_bytes_allocated value in the heap checker. We  use 3 bits for this because the 
padding size of the block is at most 7 since we are extending the block size to the nearest multiple of 8 every time 
we allocate a block.


--------Unimplemented thoughts-------
For part 2, the compaction time could be reduced by nearly 50% if we store allocated blocks at both ends of the heap.
Compaction time can be further improved for certain cases if size info of any allocated block can be accessed from 
both of its ends. This can be achieved by storing size info at the beginning of the block and its index in RT at its 
end; this allows reading the size of the block right before the free region, as the address of free region - 4 is the
index footer of the last allocated block, from which we can know the address of its size header. 
The benefit of being able to read the size info from the end is that when a block is freed and compaction is needed, 
the management system can comapare the size of the just freed block and the last allocated block; if they have the 
same size, perfect compaction is achieved by simply moving the last allocated block to the location of the just freed
block. Furthermore, if we have a two-ended heap, the chance of this happening is roughly doubled. 
