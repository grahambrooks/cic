
SRC		= src
TEST_SRC	= test
BUILD		= build
BUILD_NUMBER	?= dev

LIB_PATH = /usr/local/lib
#LT = dylib
LT = a

LIBS	= \
	$(LIB_PATH)/libboost_filesystem-mt.$(LT) \
	$(LIB_PATH)/libboost_system-mt.$(LT) \
	$(LIB_PATH)/libboost_program_options-mt.$(LT) \
	/usr/lib/libcurl.dylib

TEST_LIBS	= $(LIB_PATH)/libboost_unit_test_framework-mt.$(LT)

OBJECTS 	=


TEST_OBJECTS	=	$(BUILD)/ci_config_test.o	\
			$(BUILD)/command_line_options_parser_tests.o

all	:	ci test

build	:
	mkdir build

ci	:	build $(SRC)/*.cpp $(SRC)/*.hpp
	clang++ -g -O1 -o $@ -std=c++11 -D BUILD_NUMBER=$(BUILD_NUMBER) -Xclang "-stdlib=libc++" -lc++ $(SRC)/*.cpp $(LIBS)

citest	:	$(OBJECTS) $(TEST_OBJECTS)  $(BUILD)/test_main.o
	c++ $^ -o $@ -std=c++11 -lc++ $(LIBS) $(TEST_LIBS)

test: citest
	./$^

dist:	ci citest
	-rm ci-install.dmg
	-rm tmp.dmg
	-rm -rf dist
	mkdir dist
	ln -s /usr/local/bin dist/bin
	markdown README.md > dist/README.html
	cp ci dist
	hdiutil create tmp.dmg -ov -volname "ci console" -fs HFS+ -srcfolder "dist" 
	hdiutil convert tmp.dmg -format UDZO -o ci-install.dmg
	-rm tmp.dmg

clean:
	-rm ci
	-rm citest
	-rm -r dist
	-rm -rf $(BUILD)/*


$(BUILD)/%.o : $(TEST_SRC)/%.cpp
	clang++ -g -O1 -std=c++11 -Xclang "-stdlib=libc++" -I $(SRC) -I /usr/local/include -D MAKEFILE_BUILD -c $< -o $@

$(BUILD)/%.o : $(SRC)/%.cpp $(SRC)/%.hpp
	clang++ -g -O1 -std=c++11 -Xclang "-stdlib=libc++" -I $(SRC) -I /usr/local/include -c $< -o $@

