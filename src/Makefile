CC = gcc
CC_OPS = -rdynamic -I./ -fPIC -I/usr/include/libxml2 -DLIBXML2 
LD_OPS = -ldl -lcrypt  -lxml2
SHARED = -shared 

OBJS = linkedList.o user.o room.o command.o socket.o syslog.o misc.o \
       main.o baseCommands.o colour.o module.o bgTask.o event.o \
       revbuf.o

MODULES = std sample admin

all: pwot
	( cd modules && $(MAKE) CC='$(CC)' CC_OPS='$(CC_OPS)' LD_OPS='$(LD_OPS)' SHARED='$(SHARED)' MODULES='$(MODULES)' all ) || exit 1;

re: clean all

.c.o:
	$(CC) $(CC_OPS) -c $*.c

pwot: $(OBJS)
	$(CC) $(CC_OPS) -o pwot $(OBJS) $(LD_OPS)

clean:
	rm -f $(OBJS) pwot core *~
	( cd modules && $(MAKE) MODULES='$(MODULES)' clean ) || exit 1;
