/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext_iterator.hpp"
#include "wgui/types/witext.h"
#include <util_formatted_text.hpp>

TextLineIteratorBase::TextLineIteratorBase(WIText &text,util::text::LineIndex lineIndex,util::text::LineIndex subLineIndex,bool iterateSubLines)
	: m_text{&text},m_info{nullptr,lineIndex,0,0},m_bIterateSubLines{iterateSubLines}
{
	UpdateLine();
	auto &lines = text.GetLines();
	if(lineIndex >= lines.size())
		return;
	for(auto i=decltype(lineIndex){0u};i<lineIndex;++i)
	{
		auto &lineInfo = lines.at(i);
		if(lineInfo.wpLine.expired())
			continue;
		auto pLine = lineInfo.wpLine.lock();
		m_info.absLineStartOffset += pLine->GetAbsLength();
	}
	if(subLineIndex > 0 && iterateSubLines == false)
		throw std::logic_error{"Iterator skips sub-line but initial sub-line has been specified!"};
	while(subLineIndex > 0)
	{
		++(*this);
		--subLineIndex;
	}
}

const TextLineIteratorBase::value_type &TextLineIteratorBase::operator++()
{
	auto &lines = GetText().GetLines();
	if(m_info.lineIndex >= lines.size())
		return INVALID_LINE_INFO;
	auto &line = lines.at(m_info.lineIndex);
	auto &subLines = line.subLines;
	if(m_bIterateSubLines)
	{
		++m_info.relSubLineIndex;
		++m_info.absSubLineIndex;
	}
	if(m_bIterateSubLines == false || m_info.relSubLineIndex >= subLines.size())
	{
		++m_info.lineIndex;
		m_info.relSubLineIndex = 0;
		m_info.relCharStartOffset = 0;

		m_info.absLineStartOffset += m_info.charCountLine;
	}
	UpdateLine();
	if(m_info.relSubLineIndex > 0 && (m_info.relSubLineIndex -1) < line.subLines.size())
		m_info.relCharStartOffset += line.subLines.at(m_info.relSubLineIndex -1);
	return m_info;
}
const TextLineIteratorBase::value_type &TextLineIteratorBase::operator++(int) {return operator++();}
const TextLineIteratorBase::value_type &TextLineIteratorBase::operator--()
{
	auto &lines = GetText().GetLines();
	if(m_info.lineIndex >= lines.size() || (m_info.lineIndex == 0 && (m_info.relSubLineIndex == 0 || m_bIterateSubLines == false)))
		return INVALID_LINE_INFO;
	auto &line = lines.at(m_info.lineIndex);
	auto &subLines = line.subLines;
	if(m_bIterateSubLines)
	{
		--m_info.relSubLineIndex;
		--m_info.absSubLineIndex;
	}
	auto decrementLineIndex = m_bIterateSubLines == false || m_info.relSubLineIndex == util::text::INVALID_LINE_INDEX;
	if(decrementLineIndex)
	{
		--m_info.lineIndex;

		auto &newLine = lines.at(m_info.lineIndex);
		if(newLine.subLines.empty() || m_bIterateSubLines == false)
		{
			m_info.relSubLineIndex = 0;
			m_info.relCharStartOffset = 0;
		}
		else
		{
			m_info.relSubLineIndex = newLine.subLines.size() -1;
			m_info.relCharStartOffset = newLine.wpLine.lock()->GetLength() -newLine.subLines.back();
		}
	}
	UpdateLine();
	if(decrementLineIndex)
		m_info.absLineStartOffset -= m_info.charCountLine;
	else if(m_info.relSubLineIndex < line.subLines.size())
		m_info.relCharStartOffset -= line.subLines.at(m_info.relSubLineIndex);
	return m_info;
}
const TextLineIteratorBase::value_type &TextLineIteratorBase::operator--(int) {return operator--();}
const TextLineIteratorBase::value_type &TextLineIteratorBase::operator*() const {return m_info;}

bool TextLineIteratorBase::operator==(const TextLineIteratorBase &other) const
{
	if(m_text != other.m_text)
		return false;
	auto &lines = GetText().GetLines();
	if(m_info.lineIndex >= lines.size() || other.m_info.lineIndex >= lines.size())
		return m_info.lineIndex >= lines.size() && other.m_info.lineIndex >= lines.size();
	return m_info.absSubLineIndex == other.m_info.absSubLineIndex;
}
bool TextLineIteratorBase::operator!=(const TextLineIteratorBase &other) const {return operator==(other) == false;}

WIText &TextLineIteratorBase::GetText() const {return *m_text;}
void TextLineIteratorBase::UpdateLine()
{
	auto &formattedText = GetText().GetFormattedTextObject();
	m_info.line = formattedText.GetLine(m_info.lineIndex);
		
	auto &lines = GetText().GetLines();
	if(m_info.lineIndex == lines.size() -1)
	{
		auto &lineInfo = lines.at(m_info.lineIndex);
		m_info.isLastLine = lineInfo.subLines.empty() || m_bIterateSubLines == false || m_info.relSubLineIndex == lineInfo.subLines.size() -1;
	}

	m_info.isLastSubLine = false;
	if(m_bIterateSubLines && m_info.lineIndex < lines.size())
	{
		auto &lineInfo = lines.at(m_info.lineIndex);
		if(lineInfo.subLines.size() <= 1 || m_info.relSubLineIndex == lineInfo.subLines.size() -1)
			m_info.isLastSubLine = true;
	}

	if(m_bIterateSubLines == false)
	{
		m_info.charCountLine = m_info.charCountSubLine = m_info.line ? m_info.line->GetAbsFormattedLength() : 0;
		return;
	}
	if(m_info.lineIndex >= lines.size())
	{
		m_info.charCountLine = m_info.charCountSubLine = 0;
		return;
	}
	auto &lineInfo = lines.at(m_info.lineIndex);
	if(m_info.relSubLineIndex == 0 && lineInfo.subLines.empty())
	{
		m_info.charCountLine = m_info.charCountSubLine = m_info.line ? m_info.line->GetAbsFormattedLength() : 0;
		return;
	}
	m_info.charCountLine = m_info.line ? m_info.line->GetAbsFormattedLength() : 0;
	if(m_info.relSubLineIndex >= lineInfo.subLines.size())
	{
		m_info.charCountSubLine = 0;
		return;
	}
	m_info.charCountSubLine = lineInfo.subLines.at(m_info.relSubLineIndex);
	if(m_info.isLastSubLine)
		++m_info.charCountSubLine; // Include new-line character
}
decltype(TextLineIteratorBase::INVALID_LINE_INFO) TextLineIteratorBase::INVALID_LINE_INFO = {nullptr,INVALID_LINE,INVALID_LINE,INVALID_LINE};

TextLineIterator::TextLineIterator(WIText &text,util::text::LineIndex startLineIndex,util::text::LineIndex subLineIndex,bool iterateSubLines)
	: m_text{text},m_bIterateSubLines{iterateSubLines},m_startLineIndex{startLineIndex},m_startSubLineIndex{subLineIndex}
{}
TextLineIteratorBase TextLineIterator::begin() const
{
	return TextLineIteratorBase{m_text,m_startLineIndex,m_startSubLineIndex,m_bIterateSubLines};
}
TextLineIteratorBase TextLineIterator::end() const
{
	return TextLineIteratorBase{m_text,static_cast<uint32_t>(m_text.GetLines().size()),util::text::LAST_LINE,m_bIterateSubLines};
}

//////////////////////

CharIteratorBase::CharIteratorBase(WIText &text,util::text::LineIndex lineIndex,util::text::LineIndex subLineIndex,util::text::TextOffset absLineStartOffset,util::text::CharOffset charOffset,Flags flags)
	: m_text{&text},m_info{lineIndex,subLineIndex,0,0,0},m_flags{flags}
{
	m_info.charOffsetRelToText = util::text::LAST_CHAR;
	auto &lines = text.GetLines();
	if(lineIndex >= lines.size() || lines.at(lineIndex).wpLine.expired())
	{
		m_info.lineIndex = TextLineIteratorBase::INVALID_LINE;
		return;
	}
	auto &lineInfo = lines.at(lineIndex);
	auto pLine = lineInfo.wpLine.lock();
	auto absCharOffset = absLineStartOffset +charOffset;
	m_info.charOffsetRelToText = absCharOffset;
	m_info.charOffsetRelToLine = absCharOffset -absLineStartOffset;
	
	auto subLineCharOffset = m_info.charOffsetRelToLine;
	for(auto numChars : lineInfo.subLines)
	{
		if(subLineCharOffset < numChars)
			break;
		subLineCharOffset -= numChars;
	}
	m_info.charOffsetRelToSubLine = subLineCharOffset;

	if(lineInfo.subLines.empty() == true && m_info.subLineIndex > 0)
		m_info.lineIndex = TextLineIteratorBase::INVALID_LINE;
	else
		UpdatePixelWidth();

	if(umath::is_flag_set(flags,Flags::UpdatePixelWidth))
	{
		auto &formattedLine = pLine->GetFormattedLine().GetText();
		if(formattedLine.empty() == false)
		{
			int32_t w;
			auto subLineStartOffset = m_info.charOffsetRelToLine -m_info.charOffsetRelToSubLine;
			if(FontManager::GetTextSize(std::string_view{formattedLine}.substr(subLineStartOffset,m_info.charOffsetRelToSubLine),subLineStartOffset,GetText().GetFont(),&w,nullptr))
				m_info.pxOffset = w;
		}
	}
}

const CharIteratorBase::value_type &CharIteratorBase::operator++()
{
	auto &lines = GetText().GetLines();
	if(m_info.lineIndex >= lines.size())
		return INVALID_INFO;
	auto &line = lines.at(m_info.lineIndex);
	if(line.wpLine.expired())
	{
		// Invalidate
		m_info.lineIndex = TextLineIteratorBase::INVALID_LINE;
		return INVALID_INFO;
	}
	auto pLine = line.wpLine.lock();
	auto len = pLine->GetAbsFormattedLength();
	if(m_info.charOffsetRelToLine >= len)
	{
		// Invalidate
		m_info.lineIndex = TextLineIteratorBase::INVALID_LINE;
		return INVALID_INFO;
	}
	++m_info.charOffsetRelToLine;
	++m_info.charOffsetRelToSubLine;
	++m_info.charOffsetRelToText;
	if(umath::is_flag_set(m_flags,Flags::UpdatePixelWidth))
	{
		m_info.pxOffset += m_info.pxWidth;
		UpdatePixelWidth();
	}
	if(m_info.subLineIndex < line.subLines.size())
	{
		auto numCharsSubLine = line.subLines.at(m_info.subLineIndex);
		if(m_info.charOffsetRelToSubLine >= numCharsSubLine)
		{
			++m_info.subLineIndex;
			m_info.charOffsetRelToSubLine = 0;
			m_info.pxOffset = 0;
			if(umath::is_flag_set(m_flags,Flags::BreakAtEndOfSubLine))
				m_info.lineIndex = TextLineIteratorBase::INVALID_LINE;
			else
				UpdatePixelWidth();
		}
	}
	else if(line.subLines.empty() == false && m_info.subLineIndex > 0)
		m_info.lineIndex = TextLineIteratorBase::INVALID_LINE;
	return m_info;
}
const CharIteratorBase::value_type &CharIteratorBase::operator++(int) {return operator++();}
const CharIteratorBase::value_type &CharIteratorBase::operator*() const {return m_info;}
void CharIteratorBase::UpdatePixelWidth()
{
	m_info.pxWidth = 0;
	
	auto &lines = GetText().GetLines();
	if(m_info.lineIndex >= lines.size())
		return;
	auto &line = lines.at(m_info.lineIndex);
	if(line.wpLine.expired())
		return;
	auto pLine = line.wpLine.lock();
	auto &text = pLine->GetFormattedLine().GetText();
	if(m_info.charOffsetRelToLine < text.length())
	{
		auto c = text.at(m_info.charOffsetRelToLine);
		int32_t w;
		if(FontManager::GetTextSize(c,m_info.charOffsetRelToLine,GetText().GetFont(),&w,nullptr))
			m_info.pxWidth = w;
	}
}

bool CharIteratorBase::operator==(const CharIteratorBase &other) const
{
	if(m_text != other.m_text)
		return false;
	auto &lines = GetText().GetLines();
	if(m_info.lineIndex >= lines.size() || other.m_info.lineIndex >= lines.size())
		return m_info.lineIndex >= lines.size() && other.m_info.lineIndex >= lines.size();
	return m_info.lineIndex == other.m_info.lineIndex && m_info.charOffsetRelToLine == other.m_info.charOffsetRelToLine;
}
bool CharIteratorBase::operator!=(const CharIteratorBase &other) const {return operator==(other) == false;}

WIText &CharIteratorBase::GetText() const {return *m_text;}
decltype(CharIteratorBase::INVALID_INFO) CharIteratorBase::INVALID_INFO = {TextLineIteratorBase::INVALID_LINE,util::text::LAST_CHAR,util::text::LAST_CHAR,util::text::LAST_CHAR};

CharIterator::CharIterator(WIText &text,const TextLineIteratorBase::Info &lineInfo,bool updatePixelWidth,bool breakAtEndOfSubLine)
	: CharIterator{text,lineInfo.lineIndex,lineInfo.relSubLineIndex,lineInfo.absLineStartOffset,lineInfo.relCharStartOffset,updatePixelWidth,breakAtEndOfSubLine}
{}
CharIterator::CharIterator(
	WIText &text,util::text::LineIndex lineIndex,util::text::LineIndex subLineIndex,
	util::text::TextOffset absLineStartOffset,util::text::CharOffset charOffset,
	bool updatePixelWidth,bool breakAtEndOfSubLine
)
	: m_text{text},m_lineIndex{lineIndex},m_subLineIndex{subLineIndex},m_absLineStartOffset{absLineStartOffset},
	m_charOffset{charOffset}
{
	m_flags = CharIteratorBase::Flags::None;
	if(updatePixelWidth)
		m_flags |= CharIteratorBase::Flags::UpdatePixelWidth;
	if(breakAtEndOfSubLine)
		m_flags |= CharIteratorBase::Flags::BreakAtEndOfSubLine;
}
CharIteratorBase CharIterator::begin() const
{
	return CharIteratorBase{m_text,m_lineIndex,m_subLineIndex,m_absLineStartOffset,m_charOffset,m_flags};
}
CharIteratorBase CharIterator::end() const
{
	return CharIteratorBase{m_text,TextLineIteratorBase::INVALID_LINE,TextLineIteratorBase::INVALID_LINE,m_absLineStartOffset,m_charOffset,m_flags};
}
