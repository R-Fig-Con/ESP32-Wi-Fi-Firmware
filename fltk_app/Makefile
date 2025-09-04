include Makefile.FLTK

BIN_DIR := bin
LIB_DIRS := esp_communication \
	instances_search

COMM_DIR := esp_communication
SEARCH_DIR := instances_search

SRC := app.cxx
TARGET := $(BIN_DIR)/app


compile: $(LIB_DIRS) Esp_Group.o  exception_handling.o bin
	$(CXX) $(CXXFLAGS) -o $(TARGET) app.cxx $(LINKFLTK) \
	esp_communication/communication.o Esp_Group.o exception_handling.o instances_search/instances_search.o $(shell pkg-config --libs avahi-client)

#needs to be phony to force recreation of all, including test
#TODO change if new rule structure for tests is implemented
.PHONY: $(LIB_DIRS)
$(LIB_DIRS):
	$(MAKE) -C $@

Esp_Group.o:
	$(CXX) -c Esp_Group.cxx $(CXXFLAGS)

exception_handling.o:
	$(CXX) -c exception_handling.cxx $(CXXFLAGS)

bin:
	mkdir bin

.PHONY: clean	
clean:
	make clean -C esp_communication
	make clean -C instances_search
	rm -f *.o
