lib.name = sample_and_hold

class.sources = src/shold~.c src/sshot~.c src/penv~.c src/pulsenv~.c

PDLIBBUILDER_DIR=pd-lib-builder/
include ${PDLIBBUILDER_DIR}/Makefile.pdlibbuilder
