// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#pragma once

#ifdef WGUI_DLL
#ifdef __linux__
#define DLLWGUI __attribute__((visibility("default")))
#else
#define DLLWGUI __declspec(dllexport)
#endif
#else
#ifdef WGUI_EXE
#define DLLWGUI
#else
#ifdef __linux__
#define DLLWGUI
#else
#define DLLWGUI __declspec(dllimport)
#endif
#endif
#endif

// #define WGUI_ENABLE_SANITY_EXCEPTIONS
