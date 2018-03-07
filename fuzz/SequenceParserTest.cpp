#include "Sequence.h"
#include "SequenceParser.h"
#include <iostream>

int main() {
	auto pSeq = std::make_shared<CSequence>(sequence_t::Volume);
	pSeq->SetSetting(seq_setting_t::SETTING_DEFAULT);

	auto pConv = std::make_unique<CSeqConversionDefault>(0, 15);

	auto parser = CSequenceParser { };
	parser.SetSequence(pSeq);
	parser.SetConversion(std::move(pConv));

	if (std::string mml; std::getline(std::cin, mml)) {
		parser.ParseSequence(mml);
		std::cout << parser.PrintSequence();
	}
}
