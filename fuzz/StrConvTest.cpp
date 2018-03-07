#include "str_conv/str_conv.hpp"
#include <iostream>

int main() {
	if (std::string narrow1; std::getline(std::cin, narrow1)) {
		auto wide1 = conv::to_utf16(narrow1);
		auto narrow2 = conv::to_utf8(wide1);
		auto wide2 = conv::to_utf16(narrow2);
		if (wide1 != wide2)
			throw std::runtime_error {"to_utf16 . to_utf8 . to_utf16 != to_utf16"};
		auto narrow3 = conv::to_utf8(wide2);
		if (narrow2 != narrow3)
			throw std::runtime_error {"to_utf8 . to_utf16 . to_utf8 != to_utf8"};
	}
}

