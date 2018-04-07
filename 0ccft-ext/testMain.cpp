#include "FamiTrackerModule.h"
#include "ChannelMap.h"
#include "Compiler.h"
#include "Kraid.h"
#include "FamiTrackerDocIOJson.h"
#include "SimpleFile.h"

#include <iostream>

class CStdoutLog : public CCompilerLog {
public:
	void WriteLog(std::string_view text) override {
		std::cout << text;
	};
	void Clear() override { }
};

int main() {
	CFamiTrackerModule modfile;

	auto pMap = std::make_unique<CChannelMap>(sound_chip_t::APU, 0);
	pMap->GetChannelOrder().AddChannel(apu_subindex_t::pulse1);
	pMap->GetChannelOrder().AddChannel(apu_subindex_t::pulse2);
	pMap->GetChannelOrder().AddChannel(apu_subindex_t::triangle);
	pMap->GetChannelOrder().AddChannel(apu_subindex_t::noise);
	pMap->GetChannelOrder().AddChannel(apu_subindex_t::dpcm);
	modfile.SetChannelMap(std::move(pMap));

	Kraid { }(modfile);

	CCompiler compiler(modfile, std::make_unique<CStdoutLog>());
#ifdef WIN32
	CSimpleFile file(L"kraid.nsf", std::ios::out | std::ios::binary);
#else
	CSimpleFile file("kraid.nsf", std::ios::out | std::ios::binary);
#endif

	compiler.ExportNSF(file, 0);

	auto j = nlohmann::json(modfile);
//	std::cout << j.dump(2) << '\n';
	std::ofstream("kraid.json", std::ios::out) << j.dump() << '\n';
}
