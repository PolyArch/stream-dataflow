#CPP=riscv32-unknown-elf-g++
ifndef SS_TOOLS
$(error SS_TOOLS is undefined)
endif


prefix= $(SS_TOOLS)
MKDIR_P = mkdir -p
CPP=g++

CFLAGS= -c --std=c++11 -fPIC -g -gdwarf-3 #-DSB_DEBUG_MSG
DFG_FLAGS= --std=c++11 -O3
LIBFLAGS= -shared --std=c++11 

SRCDIR = src
OBJDIR = obj
BINDIR = lib
INCLUDEDIR = include
TARGET = libsoftbrain-emu

SOURCES := $(wildcard $(SRCDIR)/*.C)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.C=$(OBJDIR)/%.o)

all: mkdirs install install-lib
create-insts: $(BINDIR)/compile-insts

install-lib: $(BINDIR)/libsoftbrain-emu $(BINDIR)/libsoftbrain-emu-perf $(BINDIR)/libsoftbrain-emu-dbg

mkdirs:
	${MKDIR_P} ${SRCDIR}
	${MKDIR_P} ${OBJDIR}
	${MKDIR_P} ${BINDIR}
	${MKDIR_P} ${INCLUDEDIR}

install: install-lib
	${MKDIR_P} ${prefix}/lib
	cp lib/* ${prefix}/lib
	${MKDIR_P} ${prefix}/include/softbrain-lib
	cp include/* ${prefix}/include/softbrain-lib/

$(OBJDIR)/softbrain.o: $(SOURCES) $(INCLUDES)
	@echo "Building default library"
	$(CPP) $(CFLAGS) $(SOURCES) -o $@

$(OBJDIR)/softbrain-dbg.o: $(SOURCES) $(INCLUDES)
	@echo "Building debug feedback library."
	$(CPP) $(CFLAGS) $(SOURCES) -DSB_DEBUG_MSG -o $@

$(OBJDIR)/softbrain-perf.o: $(SOURCES) $(INCLUDES)
	@echo "Building performance feedback library."
	$(CPP) $(CFLAGS) $(SOURCES) -DSB_PERF_MSG -o $@

$(BINDIR)/libsoftbrain-emu: $(OBJDIR)/softbrain.o
	$(CPP) $(LIBFLAGS) -Wl,-soname,libsoftbrain-emu.so -o $@.so $^
	cp -f $(SRCDIR)/*.h $(INCLUDEDIR)/
	@echo "Build complete." 

$(BINDIR)/libsoftbrain-emu-perf: $(OBJDIR)/softbrain-perf.o
	$(CPP) $(LIBFLAGS) -Wl,-soname,libsoftbrain-emu-perf.so -o $@.so $^
	cp -f $(SRCDIR)/*.h $(INCLUDEDIR)/
	@echo "Build complete." 

$(BINDIR)/libsoftbrain-emu-dbg: $(OBJDIR)/softbrain-dbg.o
	$(CPP) $(LIBFLAGS) -Wl,-soname,libsoftbrain-emu-dbg.so -o $@.so $^
	cp -f $(SRCDIR)/*.h $(INCLUDEDIR)/
	@echo "Build complete."

$(BINDIR)/compile-insts: $(SRCDIR)/create_insts.c
	$(CPP) $(DFG_FLAGS)  $^ -o $@ 
	@echo "Built compile insts." 

clean:
	@rm -rf $(OBJECTS) $(OBJDIR)/*.o $(BINDIR)/*.so $(BINDIR)/compile-insts
	@echo "Clean complete."
