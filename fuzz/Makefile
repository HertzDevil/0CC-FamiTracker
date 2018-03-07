MAIN_DIR = ../Source
LIBFT0CC_DIR = ../libft0cc
AFL_CXXFLAGS = -std=c++17 -DAFL_FUZZ_ENABLED $(CXXFLAGS)

SequenceParser_DEPS = \
	Sequence.cpp \
	SequenceParser.cpp \
	NoteName.cpp



all: SequenceParserTest.out

clean:
	rm *.o *.out

SequenceParserTest.out:
	AFL_HARDEN=1 afl-g++ $(AFL_CXXFLAGS) -I$(MAIN_DIR) -I$(LIBFT0CC_DIR)/include \
		-o SequenceParserTest.out \
		SequenceParserTest.cpp $(addprefix $(MAIN_DIR)/,$(SequenceParser_DEPS))

.PHONY: all clean
