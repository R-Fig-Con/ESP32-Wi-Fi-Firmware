include ../Makefile.FLTK

communication.o: communication.c communication.h
	$(CXX) -c communication.c -o communication.o

compile_fake:
	$(CXX) -c fake_communication.c -o communication.o -ggdb

clean:
	rm -f *.o
