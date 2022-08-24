CC	:= g++

CFLAGS	:= -O3
CFLAGS	+= -Wall -Wextra

OBJECTS := main.o
OBJECTS += imageProcessor.o
OBJECTS += utils.o
OBJECTS += chunk.o

LIBS	:= -ljpeg
LIBS	+= -lpng

MAIN_FILES		:= main.cpp common/utils.hpp common/pnm.cpp common/do_args.cpp common/readJpeg.cpp common/png.cpp common/hsv.cpp imageProcessor.hpp
IMAGE_PROCESSOR_FILES	:= imageProcessor.cpp imageProcessor.hpp common/utils.hpp, common/hsv.cpp chunk.cpp
UTILS_FILES		:= common/utils.cpp common/utils.hpp
CHUNK_FILES		:= chunk.cpp chunk.hpp



foo:	$(OBJECTS)
	$(CC) $(CFLAGS) $(LIBS) $(OBJECTS) -o $(@) 


main.o:			$(MAIN_FILES)
	$(CC) -c $< -o $(@)

imageProcessor.o:	$(IMAGE_PROCESSOR_HEADERS)
	$(CC) -c $< -o $(@)

utils.o:		$(UTILS_FILES)
	$(CC) -c $< -o $(@)

chunk.o:		$(CHUNK_FILES)
	$(CC) -c $< -o $(@)

clean:
	rm foo *~

