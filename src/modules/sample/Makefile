OBJS = sample.o
MODULE = sample.so
.SUFFIXES: .o .so

all: $(OBJS) $(MODULE)

.c.o:
	$(CC) $(CC_OPS) -c $<

.c.so:
	ld $(SHARED) -o ../modsample.so $*.o

clean:
	rm -f $(OBJS) ../modsample.so core *~

re: clean all
