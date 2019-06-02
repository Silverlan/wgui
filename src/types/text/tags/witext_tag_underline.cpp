/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext_tags.hpp"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include <util_formatted_text_tag.hpp>

void WITextTagUnderline::InitializeOverlay(WIBase &overlay)
{
	auto *pUnderline = WGUI::GetInstance().Create<WIRect>(&overlay);
	auto hOverlay = overlay.GetHandle();
	overlay.AddCallback("SetSize",FunctionCallback<void>::Create([this,pUnderline,hOverlay]() {
		if(hOverlay.IsValid() == false)
			return;
		pUnderline->SetPos(Vector2i{0,hOverlay.get()->GetHeight() -1});
		pUnderline->SetSize(Vector2i{hOverlay.get()->GetWidth(),1});
		pUnderline->SetColor(m_underlineColor);
	}));
}
void WITextTagUnderline::CalcBounds(Vector2i &inOutPos,Vector2i &inOutSize) {inOutSize.y += 2;}
void WITextTagUnderline::Apply()
{
	WITextDecorator::Apply();
	auto startOffset = m_tag.GetOpeningTagComponent()->GetStartAnchorPoint()->GetTextCharOffset();
	m_underlineColor = m_text.GetCharColor(startOffset); // Use color of first character as color for underline

	CreateOverlayElements();
}
