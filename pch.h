// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#pragma once
// add headers that you want to pre-compile here

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <Windows.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include "tchar.h"
#include "debugapi.h"

#include <windowsx.h>

#include "fsplugin.h"

#include "megaapi.h"

constexpr int TC_MAX_PATH = 1024;
constexpr int MAX_STRING_RESOURCE = 1024;
constexpr int MAX_LOG_LENGTH = 2048;