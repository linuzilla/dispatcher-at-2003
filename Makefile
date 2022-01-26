#

CC	= gcc
CCOPT	= -Wall -O2 -g
# CCOPT	= -Wall -O2
# INCLS	= -I. @V_INCLS@ -static
# INCLS   = -I /usr/include/pcap -static
INCLS   =  -DDEBUG_LEVEL=2 -DENABLE_PTHREAD=0
#INCLS   = -DYYDEBUG=1 -DDEBUG_LEVEL=1 -DHAVE_CONFIG_H=1
#DEFS   += -DHAVE_CONFIG_H=1
#DEFS    = -DUSE_PROCESSOR_TIME
#LOPT    = -pthread

# DEFS   += -I/usr/local/misc/ucd-snmp-4.2.2/include
#DEFS   += -I/usr/local/misc/db-3.2.9a/include
#DEFS   += -I/usr/local/include/mysql
#DEFS   += -I/usr/local/include
#DEFS   += -I/usr/include/pcap
# DEFS   += -L/usr/local/misc/ucd-snmp-4.2.2/lib 
#DEFS   += -L/usr/local/misc/db-3.2.9a/lib 
#DEFS   += -L/usr/local/lib/mysql
#DEFS   += -L/usr/local/lib
#LOPT   += -lpthread
#LOPT   += -ldb-3.2
#LOPT   += -ldb3
#LOPT   += -lmysqlclient
#LOPT   += -lucdagent -lucdmibs -lsnmp -lpcap
#LOPT   += -lpcap
#LOPT    += -lm
#LOPT   += -lreadline -ltermcap
# LOPT   += -lssl

CFLAGS = $(CCOPT) $(INCLS) $(DEFS) $(OSDEPOPT)

SRC = main.c student.c department.c dispunit.c studlist.c stdvalue.c \
	sys_conf.c misclib.c textio.c \
	rotation_v1v2.c rotation_v3.c rotation_v4.c rotation_v5.c \
	parallel.c incdp.c gen_random.c show_info.c postcheck.c usot.c

USOT_SRC := $(shell echo usot/*.c usot/*.h)
USOT_OBJ=usot/*.o
OBJ = $(SRC:.c=.o)
VER := $(shell sed -e 's/.*\"\(.*\)\"/\1/' VERSION)
LEXYACCTMP = lex.yy.c y.tab.c y.tab.h y.output y.tab.o lex.yy.o

GCCVER := $(shell gcc -v 2>&1 | grep "gcc version" | awk '{print $$3}')
OSREL  := $(shell uname -r | sed 's/\([.0-9]*\).*/\1/')
# CFLAGS += -DGCC_VERSION=\"$(GCCVER)\" -DOS_RELEASE=\"$(OSREL)\"
CFLAGS += -DVERSION=\"$(VER)\"
TARGET = dispatcher
CLEANFILES = $(OBJ) $(TARGET) ${LEXYACCTMP}

.c.o:
	@rm -f $@
	$(CC) $(CFLAGS) -c $*.c

all: $(TARGET)

department.o:	department.c code92/dept92.c
	@rm -f $@
	$(CC) $(CFLAGS) -c $*.c

student.o:	student.c code92/stud92.c
	@rm -f $@
	$(CC) $(CFLAGS) -c $*.c

dispunit.o:	dispunit.c code92/du92.c
	@rm -f $@
	$(CC) $(CFLAGS) -c $*.c

dispatcher:	$(OBJ) lex.yy.c y.tab.c libusot.a
	@rm -f $@
	$(CC) $(CFLAGS) -o $@ $(OBJ) lex.yy.c y.tab.c $(LOPT) -L./ -lusot

libusot.a:	$(USOT_SRC)
	rm -f libusot.a
	(cd usot && $(MAKE))
	$(AR) rc libusot.a $(USOT_OBJ)
	ranlib libusot.a

lex.yy.c:	lexer.l
	flex lexer.l

y.tab.c:	parser.y
	bison -v -t -d -y parser.y

install:
	install -m 755 -o bin -g bin -s dispatcher /usr/local/bin
	install -m 644 -o bin -g bin dispatcher.conf-sample /usr/local/etc

clean:
	rm -f $(CLEANFILES) libusot.a
	(cd usot && $(MAKE) clean)
