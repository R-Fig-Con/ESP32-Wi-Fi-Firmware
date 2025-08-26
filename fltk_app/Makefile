include Makefile.FLTK

BIN_DIR := bin
LIB_DIRS := esp_communication
SRC := app.cxx
TARGET := ${BIN_DIR}/app


${TARGET}: $(LIB_DIRS) utils.o bin
	$(CXX) $(CXXFLAGS) -o ${TARGET} app.cxx $(LINKFLTK) esp_communication/communication.o utils.o

.PHONY: $(LIB_DIRS)
$(LIB_DIRS):
	$(MAKE) -C $(LIB_DIRS)

utils.o:
	$(CXX) -c utils.cxx $(CXXFLAGS)

app.o:
	$(CXX) $(CXXFLAGS) -c app.cxx -o app.o

bin:
	mkdir bin

.PHONY: clean	
clean:
	make clean -C esp_communication
	rm -f *.o
