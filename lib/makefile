.PHONY: all

INCLUDE := .
GCCFLAGS := 	-std=gnu99 	-g	-O2 	-Wall 	-Wextra 	-I${INCLUDE} -pthread -lrt

all:	demo_test 	

clean:
	rm -rf demo_test *.o
demo_test:	demo_test.c 	lcepoll.c 	network.c 	pool.c	event.c
	gcc ${GCCFLAGS}		$^ 	-o 	$@
