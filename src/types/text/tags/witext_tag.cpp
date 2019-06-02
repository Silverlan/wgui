/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include "wgui/types/witext_tags.hpp"
#include "wgui/types/witext_iterator.hpp"
#include <sharedutils/util_shared_handle.hpp>
#include <util_formatted_text.hpp>
#include <util_formatted_text_tag.hpp>

WITextDecorator::WITextDecorator(WIText &text)
	: m_text{text}
{}
WITextDecorator::~WITextDecorator()
{
	Clear();
}
void WITextDecorator::Initialize() {}
void WITextDecorator::Apply() {SetDirty(false);}
const util::text::AnchorPoint *WITextDecorator::GetStartAnchorPoint() const {return const_cast<WITextDecorator*>(this)->GetStartAnchorPoint();}
const util::text::AnchorPoint *WITextDecorator::GetEndAnchorPoint() const {return const_cast<WITextDecorator*>(this)->GetEndAnchorPoint();}
util::text::TextOffset WITextDecorator::GetStartTextCharOffset() const
{
	auto *pStartAnchorPoint = GetStartAnchorPoint();
	if(pStartAnchorPoint == nullptr)
		return util::text::END_OF_TEXT;
	return pStartAnchorPoint->GetTextCharOffset();
}
util::text::TextOffset WITextDecorator::GetEndTextCharOffset() const
{
	auto *pEndAnchorPoint = GetEndAnchorPoint();
	if(pEndAnchorPoint == nullptr)
		return util::text::END_OF_TEXT;
	return pEndAnchorPoint->GetTextCharOffset();
}

void WITextDecorator::Clear()
{
	for(auto &hOverlay : m_overlays)
	{
		if(hOverlay.IsValid())
			hOverlay->Remove();
	}
	m_overlays.clear();
}
bool WITextDecorator::IsValid() const {return GetStartAnchorPoint() != nullptr;}
void WITextDecorator::SetDirty(bool dirty) {m_bDirty = dirty;}
bool WITextDecorator::IsDirty() const {return m_bDirty;}
bool WITextDecorator::IsTag() const {return false;}
void WITextDecorator::InitializeOverlay(WIBase &overlay) {}

void WITextDecorator::CreateOverlayElement(util::text::LineIndex lineIndex,util::text::TextOffset startOffset,util::text::TextOffset endOffset,std::vector<WIHandle> &cachedOverlays)
{
	auto startBounds = m_text.GetCharacterPixelBounds(lineIndex,startOffset);
	auto endBounds = m_text.GetCharacterPixelBounds(lineIndex,endOffset);

	WIHandle hOverlay = {};
	while(hOverlay.IsValid() == false && cachedOverlays.empty() == false)
	{
		hOverlay = cachedOverlays.front();
		cachedOverlays.erase(cachedOverlays.begin());
	}
	if(hOverlay.IsValid() == false)
	{
		auto *p = WGUI::GetInstance().Create<WIBase>(&m_text);
		p->SetZPos(3);
		// auto *pBg = WGUI::GetInstance().Create<WIRect>(p);
		// pBg->SetColor(Color{255,0,0,128});
		// pBg->SetAutoAlignToParent(true);
		InitializeOverlay(*p);
		hOverlay = p->GetHandle();
	}
	auto *p = hOverlay.get();
	auto pos = startBounds.first;
	auto size = endBounds.second -startBounds.first;
	CalcBounds(pos,size);
	p->SetSize(size);
	p->SetPos(pos);
	m_overlays.push_back(hOverlay);
}

void WITextDecorator::CalcBounds(Vector2i &inOutPos,Vector2i &inOutSize) {}

void WITextDecorator::CreateOverlayElements()
{
	util::text::LineIndex startLineIdx,endLineIdx;
	util::text::TextOffset absStartCharOffset,absEndCharOffset;
	GetTagRange(startLineIdx,endLineIdx,absStartCharOffset,absEndCharOffset);

	auto overlays = m_overlays;
	m_overlays.clear();

	TextLineIterator lineIt {m_text,startLineIdx};
	for(auto &lineInfo : lineIt)
	{
		if(lineInfo.lineIndex > endLineIdx)
			break;
		util::text::CharOffset startOffsetRelToLine,endOffsetRelToLine;
		auto subLineStartOffset = lineInfo.relCharStartOffset;
		auto subLineEndOffset = subLineStartOffset +lineInfo.charCountSubLine -1;
		auto numChars = GetTagRange(*lineInfo.line,subLineStartOffset,subLineEndOffset,startOffsetRelToLine,endOffsetRelToLine);
		if(numChars == -1)
			break;
		if(numChars == -2)
			continue;
		CreateOverlayElement(lineInfo.lineIndex,startOffsetRelToLine,endOffsetRelToLine,overlays);
	}
	for(auto &hOverlay : overlays)
	{
		if(hOverlay.IsValid() == false)
			continue;
		hOverlay->Remove();
	}
}
int32_t WITextDecorator::GetTagRange(
	const util::text::FormattedTextLine &line,util::text::CharOffset minOffsetInLine,util::text::CharOffset maxOffsetInLine,
	util::text::CharOffset &outTagStartOffsetInLine,util::text::CharOffset &outTagEndOffsetInLine
) const
{
	util::text::LineIndex tagStartLineIdx,tagEndLineIdx;
	util::text::TextOffset tagStartOffsetRelToText,tagEndOffsetRelToText;
	GetTagRange(tagStartLineIdx,tagEndLineIdx,tagStartOffsetRelToText,tagEndOffsetRelToText);

	auto lineStartOffset = line.GetStartOffset();
	auto lineEndOffset = lineStartOffset +line.GetLength();
	if(tagEndOffsetRelToText < lineStartOffset)
		return -1;
	if(tagStartOffsetRelToText >= lineEndOffset)
		return -2;
	util::text::CharOffset tagStartOffsetRelToLine = std::max<int32_t>(tagStartOffsetRelToText -lineStartOffset,0);
	util::text::CharOffset tagEndOffsetRelToLine = tagEndOffsetRelToText -lineStartOffset;

	// Offsets are relative to unformatted text, but we need them relative to the formatted text
	tagStartOffsetRelToLine = line.GetFormattedCharOffset(tagStartOffsetRelToLine);
	tagEndOffsetRelToLine = line.GetFormattedCharOffset(tagEndOffsetRelToLine);

	if(tagEndOffsetRelToLine < minOffsetInLine)
		return -1; // Tag ends before the minimum offset
	if(tagStartOffsetRelToLine > maxOffsetInLine)
		return -2; // Tag starts after the maximum offset

	// Clamp offsets to max range
	tagStartOffsetRelToLine = std::max(tagStartOffsetRelToLine,minOffsetInLine);
	tagEndOffsetRelToLine = std::min(tagEndOffsetRelToLine,maxOffsetInLine);

	// Number of characters within the range affected by the tag
	auto numCharactersAffected = tagEndOffsetRelToLine -tagStartOffsetRelToLine +1;

	// Output
	outTagStartOffsetInLine = tagStartOffsetRelToLine;
	outTagEndOffsetInLine = tagEndOffsetRelToLine;

	return numCharactersAffected;
}
void WITextDecorator::GetTagRange(
	util::text::LineIndex &startLine,util::text::LineIndex &endLine,
	util::text::TextOffset &startOffset,util::text::TextOffset &endOffset
) const
{
	auto *startAnchorPoint = GetStartAnchorPoint();
	if(startAnchorPoint == nullptr || startAnchorPoint->IsValid() == false)
	{
		startOffset = util::text::END_OF_TEXT;
		endOffset = util::text::END_OF_TEXT;
		return;
	}
	startLine = startAnchorPoint->GetLineIndex();
	endLine = m_text.GetLineCount() -1;

	startOffset = GetStartTextCharOffset();
	endOffset = util::text::END_OF_TEXT;

	auto *endAnchorPoint = GetEndAnchorPoint();
	if(endAnchorPoint && endAnchorPoint->IsValid())
	{
		endLine = endAnchorPoint->GetLineIndex();
		endOffset = endAnchorPoint->GetTextCharOffset() -1;
	}
}

int32_t WITextDecorator::GetTagRange(
	const util::text::FormattedTextLine &line,
	util::text::CharOffset &startOffset,util::text::CharOffset &endOffset
) const
{
	util::text::LineIndex startLineIdx,endLineIdx;
	util::text::TextOffset absStartCharOffset,absEndCharOffset;
	GetTagRange(startLineIdx,endLineIdx,absStartCharOffset,absEndCharOffset);

	auto lineStartOffset = line.GetStartOffset();
	auto lineEndOffset = lineStartOffset +line.GetAbsLength() -1;

	if(absEndCharOffset < lineStartOffset)
		return -1;
	if(absStartCharOffset > lineEndOffset)
		return -2;

	startOffset = (absStartCharOffset > lineStartOffset) ? (absStartCharOffset -lineStartOffset) : 0;
	if(absEndCharOffset == util::text::END_OF_TEXT)
		endOffset = line.GetLength();
	else
		endOffset = (absEndCharOffset < lineEndOffset) ? (absEndCharOffset -lineStartOffset) : (lineEndOffset -lineStartOffset);
	startOffset = line.GetFormattedCharOffset(startOffset);
	endOffset = line.GetFormattedCharOffset(endOffset);
	return endOffset -startOffset +1; // Number of characters affected by tag within this line
}

/////////////////////

WITextTag::WITextTag(WIText &text,util::text::TextTag &tag)
	: WITextDecorator{text},m_tag{tag}
{}
bool WITextTag::IsValid() const
{
	if(m_tag.IsValid() == false)
		return false;
	auto innerRange = m_tag.GetInnerRange();
	return innerRange.has_value();
}
bool WITextTag::IsTag() const {return true;}
const util::text::TextTag &WITextTag::GetTag() const {return const_cast<WITextTag*>(this)->GetTag();}
util::text::TextTag &WITextTag::GetTag() {return m_tag;}
const std::vector<WITextTagArgument> &WITextTag::GetArguments() const {return const_cast<WITextTag*>(this)->GetArguments();}
std::vector<WITextTagArgument> &WITextTag::GetArguments() {return m_args;}
util::text::AnchorPoint *WITextTag::GetStartAnchorPoint()
{
	auto *pOpeningComponent = m_tag.GetOpeningTagComponent();
	if(pOpeningComponent == nullptr || pOpeningComponent->IsValid() == false)
		return nullptr;
	return pOpeningComponent->GetEndAnchorPoint();
}
util::text::AnchorPoint *WITextTag::GetEndAnchorPoint()
{
	auto *pClosingComponent = m_tag.GetClosingTagComponent();
	if(pClosingComponent == nullptr || pClosingComponent->IsValid() == false)
		return nullptr;
	return pClosingComponent->GetStartAnchorPoint();
}
util::text::TextOffset WITextTag::GetStartTextCharOffset() const
{
	auto offset = WITextDecorator::GetStartTextCharOffset();
	if(offset == util::text::END_OF_TEXT)
		return offset;
	return offset +1;
}
util::text::TextOffset WITextTag::GetEndTextCharOffset() const
{
	auto offset = WITextDecorator::GetEndTextCharOffset();
	if(offset == util::text::END_OF_TEXT)
		return offset;
	return offset -1;
}
