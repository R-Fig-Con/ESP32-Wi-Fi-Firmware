include ../Makefile.FLTK

compile: communication.c
	$(CXX) -c communication.c $(CXXFLAGS)
	
compile_test: compile
	$(CXX) test.c -o test $(CXXFLAGS) communication.o

clean:
	rm -f *.o
