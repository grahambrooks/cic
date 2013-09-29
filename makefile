BOOST_LIB_PATH = /usr/local/lib
#LT = dylib
LT = a
BOOST_LIBS = \
	$(BOOST_LIB_PATH)/libboost_filesystem-mt.$(LT) \
	$(BOOST_LIB_PATH)/libboost_system-mt.$(LT) \
	/usr/lib/libcurl.dylib

all	:	ci


ci	:	src/*.cpp src/*.hpp
	clang++ -g -O1 -o ci -std=c++11 -Xclang "-stdlib=libc++" -Xlinker -lc++ src/*.cpp $(BOOST_LIBS)
