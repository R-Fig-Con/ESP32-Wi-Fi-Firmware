include ../Makefile.FLTK

# --libs

compile:
	$(CXX) -c avahi_search.c -o instances_search.o $(shell pkg-config --cflags avahi-client)

clean: 
	rm -f *.o