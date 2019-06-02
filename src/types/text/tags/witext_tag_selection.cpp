/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext_tags.hpp"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include <util_formatted_text_anchor_point.hpp>
#include <util_formatted_text.hpp>
#include <util_formatted_text_line.hpp>

WITextTagSelection::WITextTagSelection(WIText &text,util::text::TextOffset startOffset,util::text::TextOffset endOffset)
	: WITextDecorator{text}
{
	auto relStart = GetAbsOffset(startOffset);
	auto relEnd = GetAbsOffset(endOffset);
	if(relStart.has_value() == false || relEnd.has_value() == false)
		return;
	auto &formattedText = m_text.GetFormattedTextObject();
	m_startAnchorPoint = formattedText.CreateAnchorPoint(relStart->first,relStart->second);
	m_endAnchorPoint = formattedText.CreateAnchorPoint(relEnd->first,relEnd->second);
}
std::optional<std::pair<util::text::LineIndex,util::text::CharOffset>> WITextTagSelection::GetAbsOffset(util::text::TextOffset offset) const
{
	auto &formattedText = m_text.GetFormattedTextObject();
	auto absOffset = formattedText.GetUnformattedTextOffset(offset);
	if(absOffset.has_value() == false)
		return {};
	return formattedText.GetRelativeCharOffset(*absOffset);
}
util::text::AnchorPoint *WITextTagSelection::GetStartAnchorPoint() {return m_startAnchorPoint.Get();}
util::text::AnchorPoint *WITextTagSelection::GetEndAnchorPoint() {return m_endAnchorPoint.Get();}
void WITextTagSelection::CalcBounds(Vector2i &inOutPos,Vector2i &inOutSize) {inOutPos.y += 2;}
void WITextTagSelection::SetStartOffset(util::text::TextOffset offset)
{
	auto &formattedText = m_text.GetFormattedTextObject();
	auto absOffset = formattedText.GetUnformattedTextOffset(offset);
	if(m_startAnchorPoint.IsValid() && absOffset.has_value())
		m_startAnchorPoint->ShiftToOffset(*absOffset);
}
void WITextTagSelection::SetEndOffset(util::text::TextOffset offset)
{
	auto &formattedText = m_text.GetFormattedTextObject();
	auto absOffset = formattedText.GetUnformattedTextOffset(offset);
	if(m_endAnchorPoint.IsValid() && absOffset.has_value())
		m_endAnchorPoint->ShiftToOffset(*absOffset);
}
void WITextTagSelection::InitializeOverlay(WIBase &overlay)
{
	overlay.SetZPos(5);
	auto *pRect = WGUI::GetInstance().Create<WIRect>(&overlay);
	pRect->SetAutoAlignToParent(true);
	pRect->SetColor(0.75f,0.75f,0.75f);
}
bool WITextTagSelection::IsValid() const {return m_startAnchorPoint.IsValid() && m_endAnchorPoint.IsValid();}
void WITextTagSelection::Apply()
{
	WITextDecorator::Apply();
	CreateOverlayElements();
}
