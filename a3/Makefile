FLAGS = -Wall -g -std=gnu99 
DEPENDENCIES = helper.h

all: psort

psort: psort.o helper.o
	gcc ${FLAGS} -o $@ $^

%.o: %.c ${DEPENDENCIES}
	gcc ${FLAGS} -c $<

clean: 
	rm -f *.o psort helper