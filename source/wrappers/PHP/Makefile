CPP             = g++
RM              = rm -f
CPP_FLAGS       = -Wall -c -I. -O2 -std=c++11

#PREFIX			= /usr
#Edit these lines to correspond with your own directories
#LIBRARY_DIR	= /opt/lampp/etc
#PHP_CONFIG_DIR	= /opt/lampp/etc

LD              = g++
LD_FLAGS        = -Wall -shared -O2 
RESULT          = ./idamphp.so

#PHPINIFILE		= 30-idamphp.ini

SOURCES			= $(wildcard *.cpp)
OBJECTS         = $(SOURCES:%.cpp=%.o)

all:	${OBJECTS} ${RESULT}

${RESULT}: ${OBJECTS}
		${LD} ${LD_FLAGS} -o $@ ${OBJECTS} -lphpcpp -lidam

clean:
		${RM} *.obj *~* ${OBJECTS} ${RESULT}

${OBJECTS}: 
		${CPP} ${CPP_FLAGS} -fpic -o $@ ${@:%.o=%.cpp}

#install:
#		cp -f ${RESULT} ${LIBRARY_DIR}
#		cp -f ${PHPINIFILE}	${PHP_CONFIG_DIR}
 
