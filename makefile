
SRC		= src
TEST_SRC	= test
BUILD		= build

BOOST_LIB_PATH = /usr/local/lib
#LT = dylib
LT = a

BOOST_LIBS	= \
	$(BOOST_LIB_PATH)/libboost_filesystem-mt.$(LT) \
	$(BOOST_LIB_PATH)/libboost_system-mt.$(LT) \
	/usr/lib/libcurl.dylib

TEST_LIBS	= $(BOOST_LIB_PATH)/libboost_unit_test_framework-mt.$(LT)

objects 	=


test_objects	=	$(BUILD)/ci_config_test.o

all	:	ci test

code :	$(objects) $(BUILD)/main.o
	c++ $^ -o $@ $(LIBS)

ci	:	src/*.cpp src/*.hpp
	clang++ -g -O1 -o ci -std=c++11 -Xclang "-stdlib=libc++" -lc++ src/*.cpp $(BOOST_LIBS)


test: citest
	./$^

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
	-rm citest
	-rm -r dist
	-rm -rf $(BUILD)/*


citest: $(objects) $(test_objects) $(BUILD)/test_main.o
	c++ $^ -o $@ $(BOOST_LIBS) $(TEST_LIBS)

$(BUILD)/%.o : $(TEST_SRC)/%.cpp
	clang++ -g -O1 -std=c++11 -Xclang "-stdlib=libc++" -lc++ -I src/cpp -c $< -I$(SRC) -o $@

$(BUILD)/%.o : $(SRC)/%.cpp $(SRC)/%.hpp
	clang++ -g -O1 -std=c++11 -Xclang "-stdlib=libc++" -lc++ -I src/cpp:/usr/local/include -c $< -I/src/cpp -o $@

