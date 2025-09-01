include ../Makefile.FLTK

# --libs

compile:
	$(CXX) -c avahi_search.c $(shell pkg-config --cflags avahi-client)

clean: 
	rm -f *.o