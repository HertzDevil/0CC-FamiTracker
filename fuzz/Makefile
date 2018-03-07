MAIN_DIR = ../Source
LIBFT0CC_DIR = ../libft0cc
AFL_CXXFLAGS = -std=c++17 -DAFL_FUZZ_ENABLED $(CXXFLAGS) \
	-I$(MAIN_DIR) -I$(LIBFT0CC_DIR)/include

SequenceParser_DEPS = \
	Sequence.cpp \
	SequenceParser.cpp \
	NoteName.cpp



all: SequenceParserTest.out StrConvTest.out

clean:
	rm *.o *.out

SequenceParserTest.out: SequenceParserTest.cpp
	AFL_HARDEN=1 afl-g++ $(AFL_CXXFLAGS) -o SequenceParserTest.out \
		SequenceParserTest.cpp $(addprefix $(MAIN_DIR)/,$(SequenceParser_DEPS))

StrConvTest.out: StrConvTest.cpp
	AFL_HARDEN=1 afl-g++ $(AFL_CXXFLAGS) -o StrConvTest.out \
		StrConvTest.cpp

.PHONY: all clean
