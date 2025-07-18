// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WIELEMENTDATA_HPP__
#define __WIELEMENTDATA_HPP__

#include <mathutil/uvec.h>

namespace wgui {
#pragma pack(push, 1)
	struct ElementData {
		static uint32_t ToViewportSize(const Vector2i &res) { return (res.x << 16) | res.y; }
		Mat4 modelMatrix;
		Vector4 color;
		uint32_t viewportSize;
		std::array<uint32_t, 3> placeholder;
	};
#pragma pack(pop)
};

#endif
