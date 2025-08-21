CXX = $(shell fltk-config --cxx)
CXXFLAGS = $(shell fltk-config --cxxflags) -Wall -I.
LINKFLTK = $(shell  fltk-config --ldstaticflags)

BIN_DIR := bin
SRC := app.cxx
TARGETS := ${BIN_DIR}/app


# Will always recreate
${TARGETS}: bin
	$(CXX) $(CXXFLAGS) -o app app.cxx $(LINKFLTK) 

bin:
	mkdir bin

.PHONY: clean	
clean:
	make clean -C esp_communication
