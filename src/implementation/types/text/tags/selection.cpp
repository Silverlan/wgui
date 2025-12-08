// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :text_tags;

pragma::gui::WITextTagSelection::WITextTagSelection(types::WIText &text, util::text::TextOffset startOffset, util::text::TextOffset endOffset) : WITextDecorator {text}
{
	auto relStart = GetAbsOffset(startOffset);
	auto relEnd = GetAbsOffset(endOffset);
	if(relStart.has_value() == false || relEnd.has_value() == false)
		return;
	auto &formattedText = m_text.GetFormattedTextObject();
	m_startAnchorPoint = formattedText.CreateAnchorPoint(relStart->first, relStart->second);
	m_endAnchorPoint = formattedText.CreateAnchorPoint(relEnd->first, relEnd->second);
}
std::optional<std::pair<util::text::LineIndex, util::text::CharOffset>> pragma::gui::WITextTagSelection::GetAbsOffset(util::text::TextOffset offset) const
{
	auto &formattedText = m_text.GetFormattedTextObject();
	auto absOffset = formattedText.GetUnformattedTextOffset(offset);
	if(absOffset.has_value() == false)
		return {};
	return formattedText.GetRelativeCharOffset(*absOffset);
}
util::text::AnchorPoint *pragma::gui::WITextTagSelection::GetStartAnchorPoint() { return m_startAnchorPoint.Get(); }
util::text::AnchorPoint *pragma::gui::WITextTagSelection::GetEndAnchorPoint() { return m_endAnchorPoint.Get(); }
void pragma::gui::WITextTagSelection::CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize) { inOutPos.y += 2; }
void pragma::gui::WITextTagSelection::SetStartOffset(util::text::TextOffset offset)
{
	auto &formattedText = m_text.GetFormattedTextObject();
	auto absOffset = formattedText.GetUnformattedTextOffset(offset);
	if(m_startAnchorPoint.IsValid() && absOffset.has_value())
		m_startAnchorPoint->ShiftToOffset(*absOffset);
}
void pragma::gui::WITextTagSelection::SetEndOffset(util::text::TextOffset offset)
{
	auto &formattedText = m_text.GetFormattedTextObject();
	auto absOffset = formattedText.GetUnformattedTextOffset(offset);
	if(m_endAnchorPoint.IsValid() && absOffset.has_value())
		m_endAnchorPoint->ShiftToOffset(*absOffset);
}
void pragma::gui::WITextTagSelection::InitializeOverlay(types::WIBase &overlay)
{
	overlay.SetZPos(5);
	auto *pRect = WGUI::GetInstance().Create<types::WIRect>(&overlay);
	pRect->SetAutoAlignToParent(true);
	pRect->SetColor(0.75f, 0.75f, 0.75f);
}
bool pragma::gui::WITextTagSelection::IsValid() const { return m_startAnchorPoint.IsValid() && m_endAnchorPoint.IsValid(); }
void pragma::gui::WITextTagSelection::Apply()
{
	WITextDecorator::Apply();
	CreateOverlayElements();
}
