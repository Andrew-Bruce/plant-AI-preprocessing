CC	:= g++

CFLAGS	:= -O3
CFLAGS	+= -Wall -Wextra
CFLAGS	+= -I./common

SRCS	:= main.cpp
SRCS	+= ./common/utils.cpp
SRCS	+= ./common/pnm.cpp
SRCS	+= ./common/do_args.cpp
SRCS	+= ./common/readJpeg.cpp
SRCS	+= ./common/png.cpp
SRCS	+= ./common/hsv.cpp
SRCS	+= normalize.cpp
SRCS	+= draw.cpp
SRCS	+= convert.cpp
SRCS	+= blockMatch.cpp
SRCS	+= chunkFlooding.cpp


LIBS	:= -ljpeg
LIBS	+= -lpng

foo:	$(SRCS)
	$(CC) $(CFLAGS) $< -o $(@) $(LIBS)

clean:
	rm -f foo *~
