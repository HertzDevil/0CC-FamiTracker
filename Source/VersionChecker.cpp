/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "json/json.hpp"
#include "VersionChecker.h"
#include "version.h"
#pragma comment(lib, "wininet.lib")

namespace {

LPCTSTR rgpszAcceptTypes[] = {_T("application/json"), NULL};

} // namespace

// // // TODO: cpr maybe
struct CHttpStringReader {
	CHttpStringReader() {
		hOpen = InternetOpen(_T("0CC_FamiTracker"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		hConnect = InternetConnect(hOpen, _T("api.github.com"),
			INTERNET_DEFAULT_HTTPS_PORT, _T(""), _T(""), INTERNET_SERVICE_HTTP, 0, 0);
		hRequest = HttpOpenRequest(hConnect, _T("GET"), _T("/repos/HertzDevil/0CC-FamiTracker/releases"),
			_T("HTTP/1.0"), NULL, rgpszAcceptTypes,
			INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE, NULL);
	}

	~CHttpStringReader() noexcept {
		if (hRequest)
			InternetCloseHandle(hRequest);
		if (hConnect)
			InternetCloseHandle(hConnect);
		if (hOpen)
			InternetCloseHandle(hOpen);
	}

	nlohmann::json ReadJson() {
		if (!hOpen || !hConnect || !hRequest)
			return nlohmann::json { };
		if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
			return nlohmann::json { };

		std::string jsonStr;
		while (true) {
			DWORD Size;
			if (!InternetQueryDataAvailable(hRequest, &Size, 0, 0))
				return nlohmann::json { };
			if (!Size)
				break;
			char *Buf = new char[Size + 1]();
			DWORD Received = 0;
			for (DWORD i = 0; i < Size; i += 1024) {
				DWORD Length = (Size - i < 1024) ? Size % 1024 : 1024;
				if (!InternetReadFile(hRequest, Buf + i, Length, &Received))
					return nlohmann::json { };
			}
			jsonStr += Buf;
			SAFE_RELEASE_ARRAY(Buf);
		}

		return nlohmann::json::parse(jsonStr);
	}

private:
	HINTERNET hOpen;
	HINTERNET hConnect;
	HINTERNET hRequest;
};

CVersionChecker::CVersionChecker(bool StartUp) : th_ {&ThreadFn, StartUp, std::move(promise_)} {
}

CVersionChecker::~CVersionChecker() {
	if (th_.joinable())		// // //
		th_.join();
}

bool CVersionChecker::IsReady() const {
	return future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

std::optional<stVersionCheckResult> CVersionChecker::GetVersionCheckResult() {
	return future_.get();
}

void CVersionChecker::ThreadFn(bool startup, std::promise<std::optional<stVersionCheckResult>> p) noexcept try {
	bool found = false;
	for (const auto &i : CHttpStringReader { }.ReadJson()) {
		int Ver[4] = { };
		sscanf_s(i["tag_name"].get<std::string>().c_str(),
					"v%u.%u.%u%*1[.r]%u", Ver, Ver + 1, Ver + 2, Ver + 3);
		if (Ver[2] >= 15) {
//				if (Compare0CCFTVersion(Ver[0], Ver[1], Ver[2], Ver[3]) > 0) {
			int Y = 1970, M = 1, D = 1;
			sscanf_s(i["published_at"].get<std::string>().c_str(), "%d-%d-%d", &Y, &M, &D);
			static const CString MONTHS[] = {
				_T("Jan"), _T("Feb"), _T("Mar"), _T("Apr"), _T("May"), _T("Jun"),
				_T("Jul"), _T("Aug"), _T("Sept"), _T("Oct"), _T("Nov"), _T("Dec"),
			};

			CString desc = i["body"].get<std::string>().c_str();
			if (int Index = desc.Find(_T("\r\n\r\n")); Index >= 0)
				desc.Delete(0, Index + 4);
			if (int Index = desc.Find(_T("\r\n\r\n#")); Index >= 0)
				desc.Truncate(Index);

			CString msg;
			msg.Format(_T("A new version of 0CC-FamiTracker is now available:\n\n"
							"Version %d.%d.%d.%d (released %s %d, %d)\n\n%s\n\n"
							"Pressing \"Yes\" will launch the Github web page for this release."),
				Ver[0], Ver[1], Ver[2], Ver[3], MONTHS[--M], D, Y, desc);
			if (startup)
				msg.Append(_T(" (Version checking on startup may be disabled in the configuration menu.)"));
			CString url;
			url.Format(_T("https://github.com/HertzDevil/0CC-FamiTracker/releases/tag/v%d.%d.%d.%d"),
				Ver[0], Ver[1], Ver[2], Ver[3]);
			p.set_value(stVersionCheckResult {msg, url, MB_YESNO | MB_ICONINFORMATION});
			found = true;
			break;
		}
	}

	if (!found)
		p.set_value(std::nullopt);
}
catch (...) {
	p.set_value(std::nullopt);
//	p.set_value(stVersionCheckResult {
//		_T("Unable to get version information from the source repository."), _T(""), MB_ICONERROR});
}
