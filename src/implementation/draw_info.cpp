// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :draw_info;
import :draw_state;
import :types.base;

#undef DrawState

Vector4 pragma::gui::DrawInfo::GetColor(types::WIBase &el, const DrawState &drawState) const
{
	Vector4 color;
	if(this->color.has_value())
		color = *this->color;
	else
		color = el.GetColor().ToVector4();
	color.a *= drawState.renderAlpha;
	return color;
}
