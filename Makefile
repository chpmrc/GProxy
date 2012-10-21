BIN_DIR=bin
SEN_DIR=src/
REC_DIR=src/
UTILS_DIR=src/
RIT_DIR=src/ritardatore
GCCFLAGS= -ansi -Wall -Wunused -pedantic -ggdb
LINKERFLAGS=-lpthread -lm

# Main files
all: init ritardatore psender preceiver

# Initial configuration
init: 
	-mkdir bin

ritardatore:
	@echo "\n\nCOMPILING RITARDATORE\n\n"
	cd ${RIT_DIR}; make clean; make
	mv ${RIT_DIR}/Ritardatore.exe ${BIN_DIR}
	
psender:
	@echo "\n\nCOMPILING PSENDER\n\n"
	cd ${SEN_DIR}; gcc ${GCCFLAGS} -o psender psender.c ${LINKERFLAGS}
	mv ${SEN_DIR}/psender ${BIN_DIR}

preceiver:
	@echo "\n\n\COMPILING PRECEIVER\n\n"
	cd ${REC_DIR}; gcc ${GCCFLAGS} -o preceiver preceiver.c ${LINKERFLAGS}
	mv ${REC_DIR}/preceiver ${BIN_DIR}

clean:
	rm -Rf bin
	rm ${SEN_DIR}/*.o
	rm ${REC_DIR}/*.o
