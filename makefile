
SRC		= src
TEST_SRC	= test
BUILD		= build

LIB_PATH = /usr/local/lib
#LT = dylib
LT = a

LIBS	= \
	$(LIB_PATH)/libboost_filesystem-mt.$(LT) \
	$(LIB_PATH)/libboost_system-mt.$(LT) \
	$(LIB_PATH)/libboost_program_options-mt.$(LT) \
	/usr/lib/libcurl.dylib

TEST_LIBS	= $(LIB_PATH)/libboost_unit_test_framework-mt.$(LT)

objects 	=


test_objects	=	$(BUILD)/ci_config_test.o	\
			$(BUILD)/command_line_options_parser_tests.o

all	:	ci test

ci	:	$(SRC)/*.cpp $(SRC)/*.hpp
	clang++ -g -O1 -o $@ -std=c++11 -Xclang "-stdlib=libc++" -lc++ $(SRC)/*.cpp $(LIBS)

citest: $(objects) $(test_objects) $(BUILD)/test_main.o
	c++ $^ -o $@ -std=c++11 -lc++ $(LIBS) $(TEST_LIBS)

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


$(BUILD)/%.o : $(TEST_SRC)/%.cpp
	clang++ -g -O1 -std=c++11 -Xclang "-stdlib=libc++" -I $(SRC) -I /usr/local/include -c $< -o $@

$(BUILD)/%.o : $(SRC)/%.cpp $(SRC)/%.hpp
	clang++ -g -O1 -std=c++11 -Xclang "-stdlib=libc++" -I $(SRC) -I /usr/local/include -c $< -o $@

