CC = gcc -g -ansi -pedantic -D_POSIX_C_SOURCE -pthread -lpthread
CFLAGS = -Wall
EXE = miner
VAL = @valgrind --leak-check=full --track-origins=yes

all : $(EXE)

.PHONY : clean
clean :
	rm -f *.o core $(EXE) 

miner: miner.o pow.o minero.o 
	@echo "#---------------------------"
	@echo "# Generating $@"
	@echo "# Depends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -o $@ $@.c pow.o minero.o -lrt

pow.o : pow.c pow.h
	@echo "#---------------------------"
	@echo "# Generating $@"
	@echo "# Depends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<

minero.o : minero.c minero.h msgq.h wallet.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<
