#pragma once

// UBT supplies DYWXCORE_API for modular imports/exports. Do not silently erase
// that contract in an Unreal build. Portable harnesses explicitly opt in.
#if defined(DYWXCORE_STANDALONE) && (defined(DYWXCORE_WITH_UNREAL) || defined(WITH_ENGINE) || defined(UE_BUILD_DEBUG) || defined(UE_BUILD_DEVELOPMENT) || defined(UE_BUILD_SHIPPING))
#error "DYWXCORE_STANDALONE cannot be used in an Unreal build"
#endif
#if defined(DYWXCORE_WITH_UNREAL)
// ABI-only bridge: supplies platform DLLEXPORT/DLLIMPORT without a PCH.
#include "HAL/Platform.h"
#endif
#ifndef DYWXCORE_API
#if defined(DYWXCORE_STANDALONE)
#define DYWXCORE_API
#else
#error "Missing UBT DYWXCORE_API; standalone consumers must define DYWXCORE_STANDALONE"
#endif
#endif
