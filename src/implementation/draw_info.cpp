// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "mathutil/color.h"

#undef DrawState

module pragma.gui;

import :draw_info;
import :draw_state;
import :types.base;

Vector4 wgui::DrawInfo::GetColor(WIBase &el, const wgui::DrawState &drawState) const
{
	Vector4 color;
	if(this->color.has_value())
		color = *this->color;
	else
		color = el.GetColor().ToVector4();
	color.a *= drawState.renderAlpha;
	return color;
}
