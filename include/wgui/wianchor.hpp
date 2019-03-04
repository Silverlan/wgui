/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIANCHOR_HPP__
#define __WIANCHOR_HPP__

#include "wguidefinitions.h"
#include <cinttypes>

struct DLLWGUI WIAnchor
{
	float left = 0.f;
	float right = 0.f;
	float top = 0.f;
	float bottom = 0.f;

	int32_t pxOffsetLeft = 0;
	int32_t pxOffsetRight = 0;
	int32_t pxOffsetTop = 0;
	int32_t pxOffsetBottom = 0;

	bool initialized = false;
};

#endif