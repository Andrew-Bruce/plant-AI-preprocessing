CXXFLAGS	+= -Ofast
CXXFLAGS	+= -Wall -Wextra -Wpedantic -Wshadow -std=c++17

OBJECTS := main.o
OBJECTS += imageProcessor.o
OBJECTS += utils.o
OBJECTS += chunk.o
OBJECTS += pnm.o
OBJECTS += jpeg.o
OBJECTS += png.o
OBJECTS += args.o
OBJECTS += hsv.o

LDLIBS	+= $(shell pkg-config --cflags --libs libjpeg libpng)

MAIN_FILES		:= main.cpp common/utils.hpp common/pnm.hpp common/do_args.hpp common/jpeg.hpp common/png.cpp common/hsv.cpp imageProcessor.hpp
IMAGE_PROCESSOR_FILES	:= imageProcessor.cpp imageProcessor.hpp common/utils.hpp common/hsv.cpp chunk.cpp
UTILS_FILES		:= common/utils.cpp common/utils.hpp
CHUNK_FILES		:= chunk.cpp chunk.hpp
PNM_FILES		:= common/pnm.cpp common/pnm.hpp
JPEG_FILES		:= common/jpeg.cpp common/jpeg.hpp
PNG_FILES		:= common/png.cpp common/png.hpp
ARGS_FILES		:= common/do_args.cpp common/do_args.hpp
HSV_FILES		:= common/hsv.cpp common/hsv.hpp



foo:	$(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDLIBS) $(OBJECTS) $(LIBS) -o $(@) 

main.o:			$(MAIN_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)

imageProcessor.o:	$(IMAGE_PROCESSOR_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)

utils.o:		$(UTILS_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)

chunk.o:		$(CHUNK_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)

pnm.o:			$(PNM_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)
jpeg.o:			$(JPEG_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)
png.o:			$(PNG_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)

args.o:			$(ARGS_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)
hsv.o:			$(HSV_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $(@)

clean:
	$(RM) foo *~ common/*~ *.o
