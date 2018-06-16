all: 
	( cd src && make all)

re: clean all

pwot:
	( cd src && make pwot )

clean:
	( cd src && make clean)
