CC = gcc
CFLAGS = -g -O2 -Wall

TARGET = foo
WOBJS = write_objclass.o write_permonst.o write_trobj.o write_shknam.o \
        write_rumors.o
OBJS = main.o swap.o $(WOBJS)

# generate the programs that create the gen-blah.c files (yeah, meta..)
def_%: def_%.o
	$(CC) -o $@ $<

# create the gen-blah.c files.
gen-%.h: def_%
	./$<

gen-%.c: def_%
	./$<

# compile the program that uses the gen-blah.c files.
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) gen-*.c

depend:
	makedepend -Y  *.c

# DO NOT DELETE

def_shknam.o: ../constant.h ../hack.onames.h
def_trobj.o: ../hack.onames.h
gen-objclass.o: gen-objclass.h
gen-permonst.o: gen-permonst.h
gen-shknam.o: gen-shknam.h
gen-trobj.o: gen-trobj.h
main.o: write_objclass.h write_permonst.h write_trobj.h write_shknam.h
main.o: records.h swap.h
write_objclass.o: gen-objclass.h swap.h
write_permonst.o: gen-permonst.h swap.h
write_shknam.o: gen-shknam.h swap.h
write_trobj.o: gen-trobj.h swap.h
