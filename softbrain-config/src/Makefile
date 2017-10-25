level=../
include ${level}/make.config

SOURCES= model.cpp model_parsing.cpp fu_model.cpp sub_model.cpp direction.cpp sbinst.cpp
PRE_OBJECTS=$(SOURCES:.cpp=.o)
INST_MODEL_FILE=full.sbinst

LIB_DEST=${build}/lib
OBJ_DEST=${build}/obj

PRE_OBJECTS=$(SOURCES:.cpp=.o)
OBJECTS = $(patsubst %,$(OBJ_DEST)/%,$(PRE_OBJECTS))

CXXFLAGS += -fPIC

all: directories sbinst.cpp $(LIB_DEST)/libsbconfig.a $(LIB_DEST)/libsbconfig.so 

$(LIB_DEST)/libsbconfig.a: $(OBJECTS)
	ar crs $@ $^
    
$(LIB_DEST)/libsbconfig.so: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -MD -shared -o $@ $^ 

sbinst.cpp: sbinst.h insts/*.h
	@echo "done"

sbinst.h: inst_model.cpp inst_model.h model_parsing.cpp insts/*.h $(INST_MODEL_FILE)
	$(CXX) $(CXXFLAGS) -MD inst_model.cpp model_parsing.cpp -o inst_model
	./inst_model $(INST_MODEL_FILE) sbinst.h sbinst.cpp
    
$(OBJ_DEST)/%.o: %.cpp %.h sbinst.h
	$(CXX) $(CXXFLAGS) -MD -c -o $@ $<

$(OBJ_DEST)/sbinst.o: sbinst.cpp sbinst.h
	$(CXX) $(CXXFLAGS) -MD -c -o $@ $<


.phony: clean

clean:
	-rm -Rf libsbconfig.a *.o sbinst.cpp sbinst.h $(LIB_DEST)/*.a $(LIB_DEST)/*.so $(OBJ_DEST)/*.o *.d

include ${level}/make.rules

