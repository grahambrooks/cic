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


dist:
	-rm ci-install.dmg
	-rm -rf dist
	mkdir dist
	ln -s /usr/local/bin dist/bin
	markdown README.md > dist/README.html
	cp ci dist
	hdiutil create /tmp/tmp.dmg -ov -volname "ci console" -fs HFS+ -srcfolder "dist" 
	hdiutil convert /tmp/tmp.dmg -format UDZO -o ci-install.dmg

clean:
	-rm ci
	-rm -r dist
