OBJS = std.o
MODULE = std.so
.SUFFIXES: .o .so

all: $(OBJS) $(MODULE)

.c.o:
	$(CC) $(CC_OPS) -c $<

.c.so:
	ld $(SHARED) -o ../modstd.so $*.o

clean:
	rm -f $(OBJS) ../modstd.so core *~

re: clean all
