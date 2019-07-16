
all:	pedump

pedump:	common.obj exedump.obj objdump.obj pedump.obj utils.obj
	bcc -mh -v pedump.c common.c exedump.c objdump.c utils.c

