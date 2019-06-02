/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include "wgui/types/witext_tags.hpp"
#include <util_formatted_text.hpp>
#include <unordered_map>

void WIText::UpdateSubLines()
{
	if(m_autoBreak == AutoBreak::NONE)
		return;
	auto &lines = m_text->GetLines();
	util::text::ShiftOffset lineShift = 0;
	uint32_t curTagIdx = 0;
	for(auto i=decltype(lines.size()){0u};i<lines.size();++i)
	{
		if(BreakLineByWidth(i,lineShift) == false)
			continue;
		// Number of sub-lines have changed, we have to mark the tags associated with this line
		// as dirty
		auto &wpLine = m_lineInfos.at(i).wpLine;
		if(curTagIdx >= m_tagInfos.size() || wpLine.expired())
			continue;
		auto pLine = wpLine.lock();
		auto lineEndOffset = pLine->GetAbsEndOffset();
		m_flags |= Flags::ApplySubTextTags;
		for(auto tagIdx=curTagIdx;tagIdx<m_tagInfos.size();++tagIdx)
		{
			auto &tag = m_tagInfos.at(tagIdx);
			if(tag->GetStartTextCharOffset() > lineEndOffset)
			{
				curTagIdx = tagIdx;
				break;
			}
			tag->SetDirty();
		}
	}
	PerformTextPostProcessing();
	CallCallbacks<void>("OnContentsChanged");
}
bool WIText::BreakLineByWidth(uint32_t lineIndex,util::text::ShiftOffset &lineShift)
{
	auto w = GetWidth();
	if(m_autoBreak == AutoBreak::NONE || w == 0 || lineIndex >= m_lineInfos.size() || m_lineInfos.at(lineIndex).wpLine.expired())
		return false;
	auto *font = GetFont();
	auto &lineInfo = m_lineInfos.at(lineIndex);
	auto line = lineInfo.wpLine.lock();
	auto &strLine = line->GetFormattedLine().GetText();
	auto strView = std::string_view{strLine};
	auto len = line->GetAbsFormattedLength() -1;
	auto i = line->GetAbsFormattedLength();
	auto breakMode = m_autoBreak;
	if(lineInfo.subLines.size() <= 1)
	{
		// If we previously only had one line, the chance that we have to split the line is
		// low. Perform a quick check to confirm and exit early if possible.
		int wText = 0;
		FontManager::GetTextSize(strLine,0u,font,&wText);
		if(wText <= w)
		{
			// We're not updating the line buffers, so we have to change the
			// line indices of the existing buffers
			for(auto &bufInfo : lineInfo.buffers)
				bufInfo.absLineIndex += lineShift;
			return false;
		}
	}
	auto oldSubLines = lineInfo.subLines;
	auto numSubLines = lineInfo.subLines.empty() ? 1 : lineInfo.subLines.size();
	lineInfo.subLines.clear();
	const auto fBreakLine = [this,w,&strView,&lineInfo,font](util::text::CharOffset charOffset) -> std::optional<util::text::CharOffset> {
		auto startOffset = charOffset;
		int32_t wText = 0;
		auto lastSpaceCharOffset = util::text::LAST_CHAR;
		while(charOffset < strView.length())
		{
			int32_t wChar = 0;
			auto c = strView.at(charOffset);
			FontManager::GetTextSize(c,0u,font,&wChar);
			if(wText +wChar > w)
			{
				// Break
				if(charOffset == 0)
					return {}; // Don't break; Character will be out-of-bounds but there's nothing we can do
				if(GetAutoBreakMode() == AutoBreak::WHITESPACE && lastSpaceCharOffset != util::text::LAST_CHAR)
					charOffset = lastSpaceCharOffset +1;
				lineInfo.subLines.push_back(charOffset -startOffset);
				return charOffset;
			}
			if(c == ' ' || c == '\f' || c == '\v' || c == '\t')
				lastSpaceCharOffset = charOffset;
			wText += wChar;
			++charOffset;
		}
		if(charOffset > startOffset)
			lineInfo.subLines.push_back(charOffset -startOffset);
		return {};
	};
	std::optional<util::text::CharOffset> startOffset = 0;
	while(startOffset.has_value())
		startOffset = fBreakLine(*startOffset);
	
	auto newNumSubLines = lineInfo.subLines.size();
	auto subLinesHaveChanged = newNumSubLines != numSubLines || lineInfo.subLines != oldSubLines;
	if(subLinesHaveChanged)
		lineInfo.bufferUpdateRequired = true;
	else
	{
		// Sublines haven't changed, no need to update the buffers.
		// The line indices have to be updated manually, however.
		for(auto &bufInfo : lineInfo.buffers)
			bufInfo.absLineIndex += lineShift;
	}

	lineShift += static_cast<util::text::ShiftOffset>(newNumSubLines) -static_cast<util::text::ShiftOffset>(numSubLines);
	return subLinesHaveChanged;
}

void WIText::SetText(const std::string_view &text)
{
	if(IsDirty() == false && *m_text == text)
		return;
	umath::set_flag(m_flags,Flags::TextDirty,false);
	umath::set_flag(m_flags,Flags::ApplySubTextTags);
	ScheduleRenderUpdate(true);
	std::string oldText = *m_text;
	auto textCpy = text;

	m_text->Clear();
	m_text->SetText(text);

	std::string newText = *m_text;
	CallCallbacks<void,std::reference_wrapper<const std::string>>("OnTextChanged",std::reference_wrapper<const std::string>(newText));
	CallCallbacks<void>("OnContentsChanged");
}
void WIText::PerformTextPostProcessing()
{
	for(auto lineIndex=decltype(m_lineInfos.size()){0u};lineIndex<m_lineInfos.size();++lineIndex)
	{
		auto &lineInfo = m_lineInfos.at(lineIndex);
		if(lineIndex == 0)
			lineInfo.subLineIndexOffset = 0;
		else
		{
			auto &prevLine = m_lineInfos.at(lineIndex -1);
			lineInfo.subLineIndexOffset = prevLine.subLineIndexOffset +(prevLine.subLines.empty() ? 1 : prevLine.subLines.size());
		}
	}
	AutoSizeToText();
}
void WIText::AutoSizeToText()
{
	if(ShouldAutoSizeToText() == false)
		return;
	// Calling SizeToContents here would result in infinite recursion
	int32_t w,h;
	if(GetAutoBreakMode() == WIText::AutoBreak::NONE)
		GetTextSize(&w,&h);
	else
		w = GetWidth();
	WIBase::SetSize(w,GetTextHeight());
}
