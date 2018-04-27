#include "FamiTrackerModule.h"
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "ChannelMap.h"
#include "Compiler.h"
#include "Kraid.h"
#include "FamiTrackerDocIOJson.h"
#include "SimpleFile.h"

#include "FamiTrackerDocIO.h"
#include "DocumentFile.h"

#include <iostream>

class CStdoutLog : public CCompilerLog {
public:
	void WriteLog(std::string_view text) override {
		std::cout << text;
	};
	void Clear() override { }
};

int main() try {
	CFamiTrackerModule modfile;
	modfile.SetChannelMap(Env.GetSoundChipService()->MakeChannelMap(sound_chip_t::APU, 0));

	Kraid { }(modfile);

	CSimpleFile nsffile("kraid.nsf", std::ios::out | std::ios::binary);
	CCompiler compiler(modfile, std::make_unique<CStdoutLog>());
	compiler.ExportNSF(nsffile, 0);
	nsffile.Close();

	auto j = nlohmann::json(modfile);
//	std::cout << j.dump(2) << '\n';
	std::ofstream("kraid.json", std::ios::out) << j.dump() << '\n';

	CDocumentFile outfile;
	outfile.Open("kraid.0cc", std::ios::out | std::ios::binary);
	CFamiTrackerDocIO io {outfile, module_error_level_t::MODULE_ERROR_DEFAULT};
	io.Save(modfile);
	outfile.Close();
}
catch (std::exception &e) {
	std::cerr << "C++ exception: " << e.what() << '\n';
	return 1;
}
catch (...) {
	std::cerr << "Unknown exception\n";
	return 1;
}
