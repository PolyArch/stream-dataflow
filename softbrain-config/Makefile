include $(SS_STACK)/msg.mk
prefix:=$(SS_TOOLS)


level=./
include make.config



.PHONY: program

all: directories program

program:
	+make -C src
	
install: program
	${MKDIR_P} ${prefix}/lib
	cp ${build}/lib/* ${prefix}/lib
	${MKDIR_P} ${prefix}/include/softbrain-config
	cp src/*.h ${prefix}/include/softbrain-config/
	cp -rf configs ${prefix}/

clean:
	make -C src clean

include make.rules
