OBJS = admin.o
MODULE = admin.so
.SUFFIXES: .o .so

all: $(OBJS) $(MODULE)

.c.o:
	$(CC) $(CC_OPS) -c $<

.c.so:
	ld $(SHARED) -o ../modadmin.so $*.o

clean:
	rm -f $(OBJS) ../modadmin.so core *~

re: clean all
