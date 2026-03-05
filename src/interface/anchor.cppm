// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "util_enum_flags.hpp"

export module pragma.gui:anchor;

export import pragma.math;

export namespace pragma::gui {
	struct DLLWGUI Anchor {
		enum class Edge {
			Left = 0,
			Right,
			Top,
			Bottom,
			Count,
		};
		enum class StateFlags : uint8_t {
			None = 0,
			Initialized = 1,
			LeftEdgeEnabled = Initialized << 1,
			RightEdgeEnabled = LeftEdgeEnabled << 1,
			TopEdgeEnabled = RightEdgeEnabled << 1,
			BottomEdgeEnabled = TopEdgeEnabled << 1,
		};
		float left = 0.f;
		float right = 0.f;
		float top = 0.f;
		float bottom = 0.f;

		int32_t pxOffsetLeft = 0;
		int32_t pxOffsetRight = 0;
		int32_t pxOffsetTop = 0;
		int32_t pxOffsetBottom = 0;

		bool IsEdgeEnabled(Edge edge) const;
		void SetEdgeEnabled(Edge edge, bool enabled = true);
		bool IsInitialized() const;
		void SetInitialized(bool initialized = true);
		StateFlags stateFlags = StateFlags::None;
	};
	using namespace math::scoped_enum::bitwise;
}

export {
	REGISTER_ENUM_FLAGS(pragma::gui::Anchor::StateFlags)
};
