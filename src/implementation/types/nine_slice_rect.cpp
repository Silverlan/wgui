// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.nine_slice_rect;

pragma::gui::types::WI9SliceRect::WI9SliceRect() : WIBaseSegmentedRect<WITexturedRect> {} {}
void pragma::gui::types::WI9SliceRect::Initialize()
{
	WIBaseSegmentedRect<WITexturedRect>::Initialize();
	SetShader(*WGUI::GetInstance().GetShader<shaders::ShaderType::TexturedNineSlice>());
}

void pragma::gui::types::WI9SliceRect::BindShader(shaders::ShaderTextured &shader, prosper::ShaderBindState &bindState, DrawState &drawState)
{
	Vector2 uiElementSize {GetWidth(), GetHeight()};
	static_cast<shaders::ShaderTexturedNineSlice &>(shader).RecordSetNineSliceSettings(bindState, uiElementSize, m_textureSize, m_borderSizes);
}

void pragma::gui::types::WI9SliceRect::OnMaterialChanged()
{
	WIBaseSegmentedRect<WITexturedRect>::OnMaterialChanged();
	auto *mat = GetMaterial();
	if(mat)
		UpdateMaterial(*mat);
	uint32_t w = 0;
	uint32_t h = 0;
	GetImageSize(w, h);
	m_textureSize = {w, h};
	m_borderSizes = {m_nineSlice.left, m_nineSlice.right, m_nineSlice.top, m_nineSlice.bottom};
}

pragma::material::Material *pragma::gui::types::WI9SliceRect::GetSegmentMaterial() { return GetMaterial(); }
