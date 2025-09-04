include Makefile.FLTK

BIN_DIR := bin
LIB_DIRS := esp_communication \
	instances_search

COMM_DIR := esp_communication
SEARCH_DIR := instances_search

SRC := exception_handling.cxx Esp_Group.cxx app.cxx
OBJ := ${SRC:.cxx=.o}
TARGET := $(BIN_DIR)/app


compile: $(LIB_DIRS) $(OBJ) bin
	$(CXX) $(CXXFLAGS) -o $(TARGET) \
	 esp_communication/communication.o $(OBJ) instances_search/instances_search.o \
	 $(LINKFLTK) $(shell pkg-config --libs avahi-client)

#needs to be phony to force recreation of all, including test
#TODO change if new rule structure for tests is implemented
.PHONY: $(LIB_DIRS)
$(LIB_DIRS):
	$(MAKE) -C $@

%.o: %.cxx
	$(CXX) -c $< $(CXXFLAGS)

bin:
	mkdir bin

.PHONY: clean	
clean:
	make clean -C esp_communication
	make clean -C instances_search
	rm -f *.o
