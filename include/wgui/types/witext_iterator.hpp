/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXT_ITERATOR_HPP__
#define __WITEXT_ITERATOR_HPP__

#include "wgui/wguidefinitions.h"
#include <util_formatted_text_line.hpp>

class WIText;
class DLLWGUI TextLineIteratorBase
{
public:
	struct DLLWGUI Info
	{
		util::text::FormattedTextLine *line = nullptr;
		util::text::LineIndex lineIndex = 0;
		util::text::LineIndex relSubLineIndex = 0;
		util::text::LineIndex absSubLineIndex = 0;
		bool isLastLine = false;
		bool isLastSubLine = false;

		util::text::TextLength charCountLine = 0;
		util::text::TextLength charCountSubLine = 0;

		util::text::CharOffset relCharStartOffset = 0;
		util::text::TextOffset absLineStartOffset = 0;
		util::text::TextOffset GetAbsCharStartOffset() const {return absLineStartOffset +relCharStartOffset;}
	};

    using iterator_category = std::forward_iterator_tag;
    using value_type = Info;
    using difference_type = value_type;
    using pointer = value_type*;
    using reference = value_type&;
	
	static const auto INVALID_LINE = std::numeric_limits<util::text::LineIndex>::max();
	static const Info INVALID_LINE_INFO;

	TextLineIteratorBase(WIText &text,util::text::LineIndex lineIndex,util::text::LineIndex subLineIndex=0,bool iterateSubLines=true);
	TextLineIteratorBase(const TextLineIteratorBase &other)=default;
	TextLineIteratorBase &operator=(const TextLineIteratorBase &other)=default;

	const value_type &operator++();
	const value_type &operator++(int);
	const value_type &operator--();
	const value_type &operator--(int);
	const value_type &operator*() const;

	bool operator==(const TextLineIteratorBase &other) const;
	bool operator!=(const TextLineIteratorBase &other) const;
private:
	WIText &GetText() const;
	void UpdateLine();
	WIText *m_text = nullptr;
	Info m_info = {};
	bool m_bIterateSubLines = true;
};

class DLLWGUI TextLineIterator
{
public:
	TextLineIterator(WIText &text,util::text::LineIndex startLineIndex=0,util::text::LineIndex subLineIndex=0,bool iterateSubLines=true);
	TextLineIteratorBase begin() const;
	TextLineIteratorBase end() const;
private:
	WIText &m_text;
	bool m_bIterateSubLines = true;
	util::text::LineIndex m_startLineIndex = 0;
	util::text::LineIndex m_startSubLineIndex = 0;
};

//////////////////////

class DLLWGUI CharIteratorBase
{
public:
	struct DLLWGUI Info
	{
		util::text::LineIndex lineIndex = 0;
		util::text::LineIndex subLineIndex = 0;
		util::text::CharOffset charOffsetRelToSubLine = 0;
		util::text::CharOffset charOffsetRelToLine = 0;
		util::text::CharOffset charOffsetRelToText = 0;

		uint32_t pxOffset = 0;
		uint32_t pxWidth = 0;
	};
	enum class Flags : uint32_t
	{
		None = 0,
		BreakAtEndOfSubLine = 1,
		UpdatePixelWidth = BreakAtEndOfSubLine<<1
	};

    using iterator_category = std::forward_iterator_tag;
    using value_type = Info;
    using difference_type = value_type;
    using pointer = value_type*;
    using reference = value_type&;

	static const Info INVALID_INFO;

	CharIteratorBase(WIText &text,util::text::LineIndex lineIndex,util::text::LineIndex subLineIndex,util::text::TextOffset absLineStartOffset,util::text::CharOffset charOffset,Flags flags);

	const value_type &operator++();
	const value_type &operator++(int);
	const value_type &operator*() const;

	bool operator==(const CharIteratorBase &other) const;
	bool operator!=(const CharIteratorBase &other) const;
private:
	void UpdatePixelWidth();
	WIText &GetText() const;
	WIText *m_text = nullptr;
	Info m_info = {};
	Flags m_flags = Flags::BreakAtEndOfSubLine;
};
REGISTER_BASIC_BITWISE_OPERATORS(CharIteratorBase::Flags)

class DLLWGUI CharIterator
{
public:
	CharIterator(
		WIText &text,util::text::LineIndex lineIndex,util::text::LineIndex subLineIndex,
		util::text::TextOffset absLineStartOffset,util::text::CharOffset charOffset=0,
		bool updatePixelWidth=false,bool breakAtEndOfSubLine=true
	);
	CharIterator(WIText &text,const TextLineIteratorBase::Info &lineInfo,bool updatePixelWidth=false,bool breakAtEndOfSubLine=true);
	CharIteratorBase begin() const;
	CharIteratorBase end() const;
private:
	WIText &m_text;
	util::text::LineIndex m_lineIndex = 0;
	util::text::LineIndex m_subLineIndex = 0;
	util::text::TextOffset m_absLineStartOffset = 0;
	util::text::CharOffset m_charOffset = 0;
	CharIteratorBase::Flags m_flags = CharIteratorBase::Flags::BreakAtEndOfSubLine;
};

#endif
