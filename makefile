CXX = clang 
CXXFLAGS = -Wall -Wextra
TESTFLAGS = `pkg-config --cflags --libs glib-2.0`
INC=./include
LIBS=./libs
SRC=./src
OBJ=./obj
BIN=./bin
TEST=./tests
TESTBIN=$(BIN)/tests
DOXIFILE=./doc/doxys/Doxyfile.in


# Build release
release: CXXFLAGS += -O3
release: unbrk

# Build debug
debug: CXXFLAGS += -g
debug: unbrk


unbrk: libraries $(BIN)/unbrk

$(BIN)/unbrk: $(OBJ)/unbrk.o $(OBJ)/configuration.o $(OBJ)/inih.o
	mkdir -p $(BIN)
	$(CXX) -L$(LIBS)/libunbrkcore/bin -L$(LIBS)/libunbrkfio/bin \
	-L$(LIBS)/libunbrkstats/bin $^ -lunbrkfio \
	-lunbrkstats -lunbrkcore -o $@ 

$(OBJ)/unbrk.o: $(SRC)/unbrk.c $(INC)/unbrk.yucc $(LIBS) $(INC)/unbrk.h
	mkdir -p $(OBJ)
	$(CXX) -I$(LIBS) -I$(LIBS)/libunbrkcore/include \
	-I$(LIBS)/libunbrkfio/include -I$(LIBS)/libunbrkstats/include \
	-I$(INC) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/configuration.o: $(SRC)/configuration.c $(INC)/configuration.h 
	mkdir -p $(OBJ)
	$(CXX) -I$(INC) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/inih.o: $(LIBS)/inih/ini.c $(LIBS)/inih/ini.h
	mkdir -p $(OBJ)
	$(CXX) -I$(LIBS)/inih -c -o $@ $<
	
libraries:
	make -C libs/libunbrkcore
	make -C libs/libunbrkfio
	make -C libs/libunbrkstats

$(INC)/unbrk.yucc: $(SRC)/unbrk.yuck
	yuck gen $< > $@

# Clean
clean:
	rm $(BIN)/* $(INC)/~* $(OBJ)/*.o


# Build documentation
doc: $(INC)/* $(DOXIFILE)
	doxygen $(DOXIFILE)
