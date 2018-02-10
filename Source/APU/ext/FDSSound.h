#ifndef FDSSOUND_H
#define FDSSOUND_H

#include <cstdint>

#if defined(_MSC_VER)
#  define MSVC_FASTCALL __fastcall
#else
#  define MSVC_FASTCALL
#endif

#if defined(__GNUC__) && !defined(__LP64__)
#  define GXX_FASTCALL [[gnu::fastcall]]
#else
#  define GXX_FASTCALL
#endif

GXX_FASTCALL void MSVC_FASTCALL FDSSoundReset(void);
GXX_FASTCALL uint8_t MSVC_FASTCALL FDSSoundRead(uint16_t address);
GXX_FASTCALL void MSVC_FASTCALL FDSSoundWrite(uint16_t address, uint8_t value);
GXX_FASTCALL int32_t MSVC_FASTCALL FDSSoundRender(void);
GXX_FASTCALL void MSVC_FASTCALL FDSSoundVolume(unsigned int volume);
void FDSSoundInstall3(void);

#endif /* FDSSOUND_H */
