include ../Makefile.FLTK

# --libs

compile:
	$(CXX) -c avahi_search.c -o instances_search.o $(shell pkg-config --cflags avahi-client)


compile_fake:
	$(CXX) -c fake_search.c -o instances_search.o -ggdb

clean: 
	rm -f *.o