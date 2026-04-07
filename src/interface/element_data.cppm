// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:element_data;

export import pragma.math;

export namespace pragma::gui {
#pragma pack(push, 1)
	struct ElementData {
		static constexpr auto ALIGNMENT_PADDING_TO_VEC4 = sizeof(float) * 3;
		static inline uint32_t ToViewportSize(const Vector2i &res) { return (res.x << 16) | res.y; }
		Mat4 modelMatrix;
		Vector4 color;
		uint32_t viewportSize;
	};
#pragma pack(pop)
};
