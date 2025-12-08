// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.gui:anchor;

export import std.compat;

export namespace pragma::gui {
	struct DLLWGUI WIAnchor {
		float left = 0.f;
		float right = 0.f;
		float top = 0.f;
		float bottom = 0.f;

		int32_t pxOffsetLeft = 0;
		int32_t pxOffsetRight = 0;
		int32_t pxOffsetTop = 0;
		int32_t pxOffsetBottom = 0;

		uint32_t referenceWidth = 0;
		uint32_t referenceHeight = 0;

		bool initialized = false;
	};
}
