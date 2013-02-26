CXX = g++
LDFLAGS ?= -L./tests/UnitTest++/ -I./tests/UnitTest++/src/
SED = sed
MV = mv
RM = rm

lib = UnitTest++
test = TestUnit.o
play = Play

src_play = play.cpp \
	bindict.cpp \
	corrector.cpp

src_test = tests/unit/test.cpp \
	bindict.cpp \
	corrector.cpp

all: $(test)

test: $(test)

play:
	@$(CXX) $(LDFLAGS) -l$(lib) -o $(play) $(src_play)
	@./$(play)

$(test):
	@$(CXX) $(LDFLAGS) -l$(lib) -o $(test) $(src_test)
	@./$(test)

clean:
	-@$(RM) $(test) $(play) 2> /dev/null