#include "Sequence.h"
#include "SequenceParser.h"
#include <iostream>

int main(int argc, char **argv) {
	if (argc < 2 || !argv[1][0]) {
		std::cerr <<
			"Usage: SequenceParserTest.out [seqType] < [seqFile]\n"
			"seqType:\n"
			"\t0: normal\n"
			"\t1: fixed arpeggio\n"
			"\t2: arpeggio scheme\n"
			"\t3: 5B noise\n";
		return 1;
	}

	int kind = argv[1][0] - '0';
	auto pSeq = std::make_shared<CSequence>(sequence_t::Volume);

	auto pConv = [&] () -> std::unique_ptr<CSeqConversionBase> {
		switch (kind) {
		case 0: return std::make_unique<CSeqConversionDefault>(-96, 96);
		case 1: return std::make_unique<CSeqConversionArpFixed>();
		case 2: return std::make_unique<CSeqConversionArpScheme>(-27);
		case 3: return std::make_unique<CSeqConversion5B>();
		}
		return nullptr;
	}();

	auto parser = CSequenceParser { };
	parser.SetSequence(pSeq);
	parser.SetConversion(std::move(pConv));

	if (std::string mml; std::getline(std::cin, mml)) {
		parser.ParseSequence(mml);
		std::cout << parser.PrintSequence();
	}
}
