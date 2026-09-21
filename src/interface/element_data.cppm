// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:element_data;

export import pragma.math;

export namespace pragma::gui {
#pragma pack(push, 1)
	struct ElementData {
		static inline uint32_t ToViewportSize(const Vector2i &res) { return (res.x << 16) | res.y; }
		static inline Mat3x4 ToShaderModelMatrix(const Mat4 &m) { return {m[0], m[1], m[3]}; }

		ElementData() = default;
		ElementData(const Mat4 &m, const Vector4 &color, uint32_t viewportSize) : modelMatrix {ToShaderModelMatrix(m)}, color {color}, viewportSize {viewportSize} {}
		void SetModelMatrix(const Mat4 &m) { modelMatrix = ToShaderModelMatrix(m); }
		Mat3x4 modelMatrix;
		Vector4 _padding;
		Vector4 color;
		uint32_t viewportSize;
	};
#pragma pack(pop)
};
