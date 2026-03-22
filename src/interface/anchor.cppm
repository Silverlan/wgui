// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "util_enum_flags.hpp"

export module pragma.gui:anchor;

export import pragma.math;

export namespace pragma::gui {
	struct DLLWGUI Anchor {
		enum class Edge : uint8_t {
			Left = 0,
			Right,
			Top,
			Bottom,
			HorizontalCenter,
			VerticalCenter,
			Count,
		};
		enum class EdgeFlags : uint8_t {
			None = 0,
			Left = 1,
			Right = Left << 1,
			Top = Right << 1,
			Bottom = Top << 1,
			HorizontalCenter = Bottom << 1,
			VerticalCenter = HorizontalCenter << 1,
		};
		enum class StateFlags : uint8_t {
			None = 0,
			LeftEdgeEnabled = 1,
			RightEdgeEnabled = LeftEdgeEnabled << 1,
			TopEdgeEnabled = RightEdgeEnabled << 1,
			BottomEdgeEnabled = TopEdgeEnabled << 1,
			HorizontalCenterEnabled = BottomEdgeEnabled << 1,
			VerticalCenterEnabled = HorizontalCenterEnabled << 1,
		};
		static constexpr EdgeFlags edge_to_flag(Edge edge) { return static_cast<EdgeFlags>(1 << math::to_integral(edge)); }
		static constexpr Edge flag_to_edge(EdgeFlags flag)
		{
			switch(flag) {
			case EdgeFlags::Left:
				return Edge::Left;
			case EdgeFlags::Right:
				return Edge::Right;
			case EdgeFlags::Top:
				return Edge::Top;
			case EdgeFlags::Bottom:
				return Edge::Bottom;
			case EdgeFlags::HorizontalCenter:
				return Edge::HorizontalCenter;
			case EdgeFlags::VerticalCenter:
				return Edge::VerticalCenter;
			}
			return {};
		}
		float left = 0.f;
		float right = 0.f;
		float top = 0.f;
		float bottom = 0.f;

		int32_t pxOffsetLeft = 0;
		int32_t pxOffsetRight = 0;
		int32_t pxOffsetTop = 0;
		int32_t pxOffsetBottom = 0;

		// Center offsets
		int32_t pxOffsetHorizontalCenter = 0;
		int32_t pxOffsetVerticalCenter = 0;

		bool IsEdgeEnabled(Edge edge) const;
		void SetEdgeEnabled(Edge edge, bool enabled = true);
		StateFlags stateFlags = StateFlags::None;
	};
	using namespace math::scoped_enum::bitwise;
}

export {
	REGISTER_ENUM_FLAGS(pragma::gui::Anchor::StateFlags)
	REGISTER_ENUM_FLAGS(pragma::gui::Anchor::EdgeFlags)
};
