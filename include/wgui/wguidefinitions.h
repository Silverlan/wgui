/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WGUIDEFINITIONS_H__
#define __WGUIDEFINITIONS_H__
#ifdef WGUI_DLL
	#ifdef __linux__
		#define DLLWGUI __attribute__((visibility("default")))
	#else
		#define DLLWGUI  __declspec(dllexport)
	#endif
#else
	#ifdef WGUI_EXE
		#define DLLWGUI
	#else
		#ifdef __linux__
			#define DLLWGUI
		#else
			#define DLLWGUI  __declspec(dllimport)
		#endif
	#endif
#endif

// #define WGUI_ENABLE_SANITY_EXCEPTIONS

#endif
