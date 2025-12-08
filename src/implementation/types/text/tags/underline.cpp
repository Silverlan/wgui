// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :text_tags;

void pragma::gui::WITextTagUnderline::InitializeOverlay(types::WIBase &overlay)
{
	auto *pUnderline = WGUI::GetInstance().Create<types::WIRect>(&overlay);
	auto hOverlay = overlay.GetHandle();
	overlay.AddCallback("SetSize", FunctionCallback<void>::Create([this, pUnderline, hOverlay]() {
		if(hOverlay.IsValid() == false)
			return;
		pUnderline->SetPos(Vector2i {0, hOverlay.get()->GetHeight() - 1});
		pUnderline->SetSize(Vector2i {hOverlay.get()->GetWidth(), 1});
		pUnderline->SetColor(m_underlineColor);
	}));
}
void pragma::gui::WITextTagUnderline::CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize) { inOutSize.y += 2; }
void pragma::gui::WITextTagUnderline::Apply()
{
	WITextDecorator::Apply();
	auto startOffset = m_tag.GetOpeningTagComponent()->GetStartAnchorPoint()->GetTextCharOffset();
	m_underlineColor = m_text.GetCharColor(startOffset); // Use color of first character as color for underline

	CreateOverlayElements();
}
