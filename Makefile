# dette er en insane makefil, jeg forstaar halvparten
# jeg fikk det fra gruppelaerer som ogsaa ikke forstaar
# alt. Dette er den den eneste maaten jeg har faatt det til aa funke
# at man kan include "inode.h" i load*_example uten at man faar file
# not found error

# For aa kjoere noe maa man gaa inn i riktig mappe foerst og kjoere
# det derifra

CFLAGS=-g -Wall -Wextra -std=gnu11
# disse har jeg ikke klart aa implementere, men jeg lar dem vaere her saa det
# er lett for retter og kopiere det inn om de har lyst til aa sjekke med valgrind
VFLAGS=--track-origins=yes --malloc-fill=0x40 --free-fill=0x23 --leak-check=full --show-leak-kinds=all
BIN=./create_example1/create_fs1  ./create_example2/create_fs2  ./create_example3/create_fs3 ./load_example1/load_fs1 ./load_example2/load_fs2 ./load_example3/load_fs3

all: $(BIN)

# create1
./create_example1/create_fs1: create_fs1.o allocation.o inode.o 
	# $^ alle dependencies
	# $@ filen foer :
	# -o ikke remake filen med mindre du maa
	gcc $(CFLAGS) $^ -o $@

create_fs1.o: ./create_example1/create_fs.c
	gcc $(CFLAGS) -I. -c $^ -o $@

# create2
./create_example2/create_fs2: create_fs2.o allocation.o inode.o 
	gcc $(CFLAGS) $^ -o $@

create_fs2.o: ./create_example2/create_fs.c
	gcc $(CFLAGS) -I. -c $^ -o $@

# create3
./create_example3/create_fs3: create_fs3.o allocation.o inode.o 
	gcc $(CFLAGS) $^ -o $@

create_fs3.o: ./create_example3/create_fs.c
	gcc $(CFLAGS) -I. -c $^ -o $@
# vi bruker o filer fordi man slipper aa rekompile alt hver
# gang noe endres, man bare rekompiler den ene fila og 
# relinker den til de andre objektfilene

# load1
./load_example1/load_fs1: load_fs1.o allocation.o inode.o 
	gcc $(CFLAGS) $^ -o $@

load_fs1.o: ./load_example1/load_fs.c
	gcc $(CFLAGS) -I. -c $^ -o $@

# load2
./load_example2/load_fs2: load_fs2.o allocation.o inode.o 
	gcc $(CFLAGS) $^ -o $@

load_fs2.o: ./load_example2/load_fs.c
	gcc $(CFLAGS) -I. -c $^ -o $@

# load3
./load_example3/load_fs3: load_fs3.o allocation.o inode.o 
	gcc $(CFLAGS) $^ -o $@

load_fs3.o: ./load_example3/load_fs.c
	gcc $(CFLAGS) -I. -c $^ -o $@

%.o: %.c
	gcc $(CFLAGS) $^ -c

clean:
	rm -rf *.o
	rm -f $(BIN)
