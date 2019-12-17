LIBS        = -lm
INC_PATH    = -I.
CC_FLAGS    = -g

all: pa31 pa32 memleak1 memleak2

pa31: testsuite.c pa31.c
	gcc $(CC_FLAGS) $(INC_PATH) -o pa31 testsuite.c $(LIBS)

pa32: testsuite.c pa32.c
	gcc -DVHEAP $(CC_FLAGS) $(INC_PATH) -o pa32 testsuite.c $(LIBS)
	
memleak1: pa31.c testsuite.c
	gcc -DMEMLEAK $(CC_FLAGS) $(INC_PATH) -o memleak1 testsuite.c $(LIBS)

memleak2: pa32.c testsuite.c
	gcc -DMEMLEAK -DVHEAP $(CC_FLAGS) $(INC_PATH) -o memleak2 testsuite.c $(LIBS)

clean :
	rm -f *.o *~ pa31 pa32 memleak1 memleak2 $(PROGS)
