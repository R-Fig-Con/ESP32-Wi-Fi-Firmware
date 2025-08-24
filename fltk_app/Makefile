include Makefile.FLTK

BIN_DIR := bin
SRC := app.cxx
TARGET := ${BIN_DIR}/app


# Will always recreate
${TARGET}: esp_communication/communication.o utils.c bin
	$(CXX) $(CXXFLAGS) -o ${TARGET} app.cxx $(LINKFLTK) esp_communication/communication.o utils.o

esp_communication/communication.o:
	make -C esp_communication

utils.c:
	$(CXX) -c utils.cxx $(CXXFLAGS)

app.o:
	$(CXX) $(CXXFLAGS) -c app.cxx -o app.o

bin:
	mkdir bin

.PHONY: clean	
clean:
	make clean -C esp_communication
