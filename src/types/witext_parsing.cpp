/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include <unordered_map>

#pragma optimize("",off)
uint32_t WIText::ParseLine(const std::string_view &line)
{
	auto lenText = (*m_text)->length() +line.length();
	m_rawTextIndexToVisibleTextIndex.resize(lenText,0u);
	m_visibleTextIndexToRawTextIndex.resize(lenText,0u);

	auto lenLine = line.length();
	auto offsetIntoLine = 0u;
	auto offsetIntoText = (*m_text)->length();
	auto offsetIntoVisibleText = m_visibleText.length();
	m_visibleText.reserve(m_visibleText.size() +lenLine);
	(*m_text)->reserve((*m_text)->size() +lenLine);

	auto subTagsOffset = m_subTextTags.size();
	
	auto numLinesPrev = m_lines.size();
	if(m_lines.empty() == false && m_lines.back().newLine == false)
	{
		--numLinesPrev; // Continue at last line
		m_lines.back().bufferUpdateRequired = true;
	}
	else
		m_lines.push_back({});
	m_lines.back().textRange.offset = offsetIntoText;
	auto *curLine = &m_lines.back();
	while(offsetIntoLine < lenLine)
	{
		auto c = line.at(offsetIntoLine);
		switch(c)
		{
			case '\n':
			{
				m_rawTextIndexToVisibleTextIndex.at(offsetIntoText) = offsetIntoVisibleText;
				m_visibleTextIndexToRawTextIndex.at(offsetIntoVisibleText) = offsetIntoText;
				curLine->textRange.length = offsetIntoText -curLine->textRange.offset;
				curLine->newLine = true;
				if(curLine->visTextRange.length == 0)
				{
					curLine->visTextRange.offset = offsetIntoVisibleText;
					curLine->visTextRange.length = 0u;
				}
				m_lines.push_back({});
				curLine = &m_lines.back();
				curLine->textRange.offset = offsetIntoText;

				m_visibleText += c;
				++offsetIntoLine;
				break;
			}
			case '[':
			{
				TagInfo tagInfo;
				bool isClosingTag;
				auto endPos = ParseCodeTag(line,offsetIntoLine,&tagInfo,&isClosingTag);
				if(endPos.has_value())
				{
					auto endPosText = offsetIntoText +(*endPos -offsetIntoLine);
					if(isClosingTag == false)
					{
						tagInfo.rangeOpenTag.offset = offsetIntoText;
						tagInfo.rangeOpenTag.length = *endPos -offsetIntoLine;
						m_subTextTags.push_back(tagInfo);

						//if(curLine->textRange.length == 0)
						//	curLine->textRange.offset = offsetIntoText;
					}
					else
					{
						auto itTag = std::find_if(m_subTextTags.rbegin(),m_subTextTags.rend(),[&tagInfo](const TagInfo &tagInfoOther) {
							return tagInfoOther.rangeClosingTag.length == 0 && tagInfo.tagType == tagInfoOther.tagType;
						});
						if(itTag != m_subTextTags.rend())
						{
							itTag->rangeClosingTag.offset = offsetIntoText;
							itTag->rangeClosingTag.length = *endPos -offsetIntoLine;
						}
						curLine->textRange.length = endPosText -curLine->textRange.offset;
					}

					int32_t visIdx;
					if(isClosingTag)
						visIdx = (offsetIntoVisibleText > 0) ? (static_cast<int32_t>(offsetIntoVisibleText) -1) : 0;
					else
						visIdx = offsetIntoVisibleText;
					for(auto i=offsetIntoText;i<endPosText;++i)
						m_rawTextIndexToVisibleTextIndex.at(i) = visIdx;
					m_visibleTextIndexToRawTextIndex.at(visIdx) = isClosingTag ? (offsetIntoText -1) : endPosText;
					offsetIntoText = endPosText;
					offsetIntoLine = *endPos;
					break;
				}
				// break;
			}
			default:
			{
				m_rawTextIndexToVisibleTextIndex.at(offsetIntoText) = offsetIntoVisibleText;
				m_visibleTextIndexToRawTextIndex.at(offsetIntoVisibleText) = offsetIntoText;
				if(curLine->visTextRange.length == 0)
					curLine->visTextRange.offset = m_visibleText.length();
				curLine->visTextRange.length = m_visibleText.length() -curLine->visTextRange.offset +1;
				curLine->textRange.length = offsetIntoText -curLine->textRange.offset +1;
				++offsetIntoText;
				++offsetIntoLine;
				m_visibleText += c;
				offsetIntoVisibleText = m_visibleText.length();
				break;
			}
		}
	}
	if(m_lines.back().textRange.length == 0 && m_lines.back().newLine == false)
		m_lines.pop_back();
	**m_text += line.substr(0,offsetIntoLine);
	m_rawTextIndexToVisibleTextIndex.resize((*m_text)->size());
	m_visibleTextIndexToRawTextIndex.resize(m_visibleText.size());

	// Remove invalid tags (tags which haven't been closed)
	/*for(auto it=m_subTextTags.begin() +subTagsOffset;it!=m_subTextTags.end();)
	{
		auto &tagInfo = *it;
		if(tagInfo.range.second == INVALID_RANGE_INDEX)
			it = m_subTextTags.erase(it);
		else
			++it;
	}*/

	if(offsetIntoText > 0u && m_lines.empty() == false)
	{
		auto &lineInfo = m_lines.back();
		lineInfo.textRange.offset = umath::min(lineInfo.textRange.offset,static_cast<int32_t>((*m_text)->length()) -1);
		lineInfo.textRange.length = offsetIntoText -lineInfo.textRange.offset;
	}
	
	// Initialize line string views objects
	UpdateLineStringViewObjects(numLinesPrev);

	// Break lines if necessary
	for(auto i=numLinesPrev;i<m_lines.size();++i)
		BreakLineByWidth(i);

	// Schedule new lines for buffer updates
	m_flags |= Flags::RenderTextScheduled | Flags::ApplySubTextTags;

	return offsetIntoLine;
}
bool WIText::ParseLines(const std::string_view &text)
{
	(*m_text)->reserve((*m_text)->length() +text.length());
	m_visibleText.reserve(m_visibleText.length() +text.length());

	auto offset = ParseLine(text);
	while(offset < text.length())
		offset += ParseLine(text.substr(offset));
	return true;
}
void WIText::UpdateLineStringViewObjects(uint32_t offset)
{
	auto *pFont = GetFont();
	for(auto i=offset;i<m_lines.size();++i)
	{
		auto &lineInfo = m_lines.at(i);
		if(m_visibleText.empty())
		{
			if(lineInfo.visTextRange.length == 0)
				lineInfo.visTextRange.offset = 0u;
			lineInfo.line = std::string_view{m_visibleText};
		}
		else
		{
			if(lineInfo.textRange.length == 0 || lineInfo.textRange.GetEndOffset() < lineInfo.textRange.offset)
				lineInfo.line = {};
			else
				lineInfo.line = std::string_view{m_visibleText}.substr(lineInfo.visTextRange.offset,lineInfo.visTextRange.length);
		}
		FontManager::GetTextSize(lineInfo.line,0u,pFont,&lineInfo.widthInPixels,nullptr);
	}
}

void WIText::BreakLineByWidth(uint32_t lineIndex)
{
	auto w = GetWidth();
	if(m_autoBreak == AutoBreak::NONE || w == 0)
		return;
	int wText = 0;
	int hText = 0;
	auto *font = GetFont();
	auto &lineInfo = m_lines.at(lineIndex);
	const auto &line = lineInfo.line;
	auto len = lineInfo.GetNumberOfCharacters() -1;
	auto i = lineInfo.GetNumberOfCharacters();
	auto breakMode = m_autoBreak;
	FontManager::GetTextSize(line,0u,font,&wText,&hText); // COLOR TODO
	while(wText > w && i > 0)
	{
		i--;
		if(breakMode == AutoBreak::ANY || line[i] == ' ' || line[i] == '\f' || line[i] == '\v')
			FontManager::GetTextSize(line.substr(0,i),0u,font,&wText,&hText); // COLOR TODO
		else if(i == 0 && breakMode == AutoBreak::WHITESPACE)
		{
			// Unable to break at whitespace; Try again, but break at ANY character
			i = lineInfo.GetNumberOfCharacters();
			breakMode = AutoBreak::ANY;
		}
	}
	if(i >= len || i <= 0)
		return;
	if(breakMode == AutoBreak::WHITESPACE && line.at(i) == ' ')
		++i; // Add whitespace to first segment
	auto lineCpy = lineInfo;
	lineCpy.visTextRange.length = i;
	lineCpy.textRange.length = m_visibleTextIndexToRawTextIndex.at(lineCpy.visTextRange.GetEndOffset()) -lineCpy.textRange.offset +1;
	lineCpy.line = line.substr(0,i);
	lineCpy.newLine = false;

	auto &infoNext = m_lines.at(lineIndex);
	infoNext.visTextRange.offset = lineCpy.visTextRange.GetEndOffset() +1;
	infoNext.visTextRange.length = line.length() -i;
	infoNext.textRange.offset = m_visibleTextIndexToRawTextIndex.at(infoNext.visTextRange.offset);
	infoNext.textRange.length = m_visibleTextIndexToRawTextIndex.at(infoNext.visTextRange.GetEndOffset()) -infoNext.textRange.offset +1;
	infoNext.line = infoNext.line.substr(i);

	m_lines.insert(m_lines.begin() +lineIndex,lineCpy);
}

bool WIText::IsTextUpdateRequired(const std::string &newText,bool compareText) const
{
	if(compareText == true && newText != **m_text)
		return true;
	if(GetAutoBreakMode() == AutoBreak::NONE)
		return false;
	auto w = GetWidth();
	// TODO: If size is now larger than previously, may have to be updated, too
	auto it = std::find_if(m_lines.begin(),m_lines.end(),[w](const LineInfo &lineInfo) {
		return lineInfo.widthInPixels > w;
	});
	return it != m_lines.end();
}

void WIText::SetText(const std::string &text)
{
	if(IsDirty() == false && text == **m_text)
		return;
	umath::set_flag(m_flags,Flags::TextDirty,false);
	ScheduleRenderUpdate(true);
	std::string oldText = *m_text;
	auto textCpy = text;

	m_lines.clear();
	m_visibleText.clear();
	m_visibleText.reserve(textCpy.size());
	(*m_text)->clear();
	(*m_text)->reserve(textCpy.size());
	m_subTextTags.clear();
	m_rawTextIndexToVisibleTextIndex.clear();
	m_visibleTextIndexToRawTextIndex.clear();

	ParseLines(std::string_view{textCpy});

	PerformTextPostProcessing();
	CallCallbacks<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged",std::reference_wrapper<const std::string>(oldText),std::reference_wrapper<const std::string>(**m_text));
}
void WIText::PerformTextPostProcessing()
{
	if(m_lines.empty())
		m_lineCount = 0;
	else
	{
		m_lineCount = std::count_if(m_lines.begin(),m_lines.end(),[](const LineInfo &lineInfo) {
			return lineInfo.newLine;
		}) +1;
	}
	for(auto it=m_subTextTags.begin();it!=m_subTextTags.end();)
	{
		auto &tagInfo = *it;
		if(tagInfo.rangeClosingTag.length != 0)
		{
			auto isTagEmpty = m_visibleText.empty();
			if(isTagEmpty == false)
			{
				auto firstVisCharOffset = m_rawTextIndexToVisibleTextIndex.at(tagInfo.rangeOpenTag.offset);
				auto firstVisRawCharOffset = m_visibleTextIndexToRawTextIndex.at(firstVisCharOffset);
				if(firstVisRawCharOffset < tagInfo.rangeOpenTag.offset || firstVisRawCharOffset > tagInfo.rangeClosingTag.GetEndOffset())
					isTagEmpty = true;
				else if(tagInfo.GetLength() == (tagInfo.rangeOpenTag.length +tagInfo.rangeClosingTag.length) +1)
				{
					auto c = m_visibleText.at(firstVisCharOffset);
					if(c == '\n')
						isTagEmpty = true; // Tag only contains a new-line, remove it!
				}
			}
			if(isTagEmpty)
			{
				// Tag is empty, remove it
				// TODO: Also remove tag from text
				it = m_subTextTags.erase(it);
				continue;
			}
		}
		++it;
	}
	UpdateLineStringViewObjects(); // Has to be done for ALL lines, since some of them may have become invalid due to m_text change
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
void WIText::InsertLine(uint32_t lineIdx,const std::string_view &line)
{
#if 0
	if(line.empty())
		return;
	auto lineNl = std::string{line} +'\n';
	try
	{
	auto curLineIndex = 0u;
	auto internalLineIndex = 0u;
	for(auto &lineInfo : m_lines)
	{
		if(curLineIndex == lineIdx)
			break;
		if(lineInfo.newLine)
			++curLineIndex;
		++internalLineIndex;
	}
	
	LineInfo *prevLine = nullptr;
	if(internalLineIndex > 0u)
		prevLine = &m_lines.at(internalLineIndex -1u);
	auto itLineInfo = m_lines.insert(m_lines.begin() +internalLineIndex,LineInfo{});
	auto &lineInfo = *itLineInfo;
	if(prevLine != nullptr)
	{
		// TODO: Take tags into account
		lineInfo.textRange.offset = prevLine->textRange.GetEndOffset() +1u;
		lineInfo.visTextRange.first = prevLine->visTextRange.second +1u;
	}
	else
	{
		lineInfo.textRange.offset = 0u;
		lineInfo.visTextRange.first = 0u;
	}
	lineInfo.textRange.length = (lineInfo.textRange.offset +lineNl.length()) -lineInfo.textRange.offset;
	lineInfo.visTextRange.second = lineInfo.visTextRange.first +line.length() -1u;

	lineInfo.newLine = true; // TODO

	*m_text = ustring::substr(**m_text,0u,lineInfo.textRange.offset) +std::string{lineNl} +ustring::substr(**m_text,lineInfo.textRange.GetEndOffset());
	m_visibleText = ustring::substr(m_visibleText,0u,lineInfo.visTextRange.first) +std::string{line} +ustring::substr(m_visibleText,lineInfo.visTextRange.second +1u);

	std::vector<uint32_t> rawToVisIndices {};
	rawToVisIndices.reserve(lineNl.length());
	for(auto i=decltype(lineNl.length()){0u};i<lineNl.length();++i)
		rawToVisIndices.push_back(lineInfo.textRange.offset +i);
	m_rawTextIndexToVisibleTextIndex.insert(m_rawTextIndexToVisibleTextIndex.begin() +lineInfo.textRange.offset,rawToVisIndices.begin(),rawToVisIndices.end());
	
	std::vector<uint32_t> visToRawIndices {};
	visToRawIndices.reserve(line.length());
	for(auto i=decltype(line.length()){0u};i<line.length();++i)
		visToRawIndices.push_back(lineInfo.visTextRange.first +i);
	m_visibleTextIndexToRawTextIndex.insert(m_visibleTextIndexToRawTextIndex.begin() +lineInfo.visTextRange.first,visToRawIndices.begin(),visToRawIndices.end());

	for(auto i=lineInfo.textRange.GetEndOffset() +1u;i<m_rawTextIndexToVisibleTextIndex.size();++i)
		m_rawTextIndexToVisibleTextIndex.at(i) += lineNl.length();
	for(auto i=lineInfo.visTextRange.second +1u;i<m_visibleTextIndexToRawTextIndex.size();++i)
		m_visibleTextIndexToRawTextIndex.at(i) += line.length();


	/*auto itBufInfoEraseStart = m_textBufferInfo.glyphInfoBufferInfos.end();
	auto itBufInfoEraseEnd = m_textBufferInfo.glyphInfoBufferInfos.end();
	for(auto it=m_textBufferInfo.glyphInfoBufferInfos.begin();it!=m_textBufferInfo.glyphInfoBufferInfos.end();++it)
	{
		auto &bufInfo = *it;
		if(bufInfo.range.first > endOffsetLine)
			break;
		if(bufInfo.range.first == startOffset)
			itBufInfoEraseStart = it;
		if(bufInfo.range.second == endOffsetLine)
			itBufInfoEraseEnd = it;
	}
	if(itBufInfoEraseStart != m_textBufferInfo.glyphInfoBufferInfos.end() && itBufInfoEraseEnd != m_textBufferInfo.glyphInfoBufferInfos.end())
		m_textBufferInfo.glyphInfoBufferInfos.erase(itBufInfoEraseStart,itBufInfoEraseEnd +1u);
	\*/




	for(auto it=m_lines.begin() +(internalLineIndex +1u);it!=m_lines.end();++it)
	{
		auto &lineInfo = *it;
		if(lineInfo.textRange.offset >= lineInfo.textRange.offset)
		{
			lineInfo.textRange.offset += lineNl.length();
			lineInfo.textRange.second += lineNl.length();
		}
		if(lineInfo.visTextRange.first >= lineInfo.visTextRange.first)
		{
			lineInfo.visTextRange.first += line.length();
			lineInfo.visTextRange.second += line.length();
		}
	}

	for(auto &lineInfo : m_lines)
		lineInfo.line = std::string_view{m_visibleText}.substr(lineInfo.visTextRange.first,lineInfo.visTextRange.second -lineInfo.visTextRange.first +1u);

	++m_lineCount;

	InitializeTextBuffers(lineIdx);
	WIBase::SetSize(GetWidth(),GetTextHeight());
	}
	catch(...)
	{
		std::cout<<"";
	}

#endif
}
void WIText::AppendLine(const std::string_view &line)
{
	std::string strNlLine;
	auto nlLine = line;
	if(line.empty() || line.back() != '\n')
	{
		strNlLine = std::string{line} +'\n';
		nlLine = strNlLine;
	}
	std::string oldText = *m_text;
	ParseLines(std::string_view{nlLine});

	PerformTextPostProcessing();
	CallCallbacks<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged",std::reference_wrapper<const std::string>(oldText),std::reference_wrapper<const std::string>(**m_text));

	std::stringstream ss;
	if(Validate(ss) == false)
	{
		std::cout<<"Append line validation error:\n"<<ss.str()<<std::endl;
	}
}
void WIText::PrependLine(const std::string_view &line)
{
	InsertLine(0u,line);
}
std::optional<uint32_t> WIText::GetInternalLineIndex(uint32_t lineIndex) const
{
	auto sLineIndex = static_cast<int32_t>(lineIndex);
	auto internalIndex = 0u;
	while(sLineIndex > 0u && internalIndex < m_lines.size())
	{
		auto &lineInfo = m_lines.at(internalIndex);
		if(lineInfo.newLine)
			--sLineIndex;
		++internalIndex;
		continue;
	}
	if(sLineIndex > 0 || m_lines.empty())
		return {};
	return umath::min(internalIndex,static_cast<uint32_t>(m_lines.size() -1));
}
void WIText::RemoveLine(uint32_t inLineIndex)
{
	// Inputs
	auto lineIndex = GetInternalLineIndex(inLineIndex);
	if(lineIndex.has_value() == false)
		return;

	struct
	{
		// Number of characters removed from m_text
		uint32_t numRawCharsRemoved = 0u;
		// Number of characters removed from m_visibleText
		uint32_t numVisCharsRemoved = 0u;

		// The new offset into m_text for the first line after the deleted line
		uint32_t newRawStartOffset = 0u;
		// The new offset into m_visibleText for the first line after the deleted line
		uint32_t newVisStartOffset = 0u;

		// The original start offset into m_text for the deleted line
		uint32_t deletedLineRawStartOffset = 0u;
		// The original end offset into m_text for the deleted line
		uint32_t deletedLineRawEndOffset = 0u;
	} info;
	auto first = true;
	while(true)
	{
		auto &lineInfo = m_lines.at(*lineIndex);
		if(first)
		{
			info.newRawStartOffset = lineInfo.textRange.offset;
			info.newVisStartOffset = lineInfo.visTextRange.offset;
			first = false;
		}
		auto bNewLine = lineInfo.newLine;
		info.numRawCharsRemoved += lineInfo.textRange.length;
		info.numVisCharsRemoved += lineInfo.visTextRange.length;
		m_lines.erase(m_lines.begin() +*lineIndex);
		if(bNewLine)
		{
			++info.numRawCharsRemoved;
			++info.numVisCharsRemoved;
			break;
		}
	}
	if(m_lines.empty())
	{
		// Skip everything else and just clear the text
		SetText("");
		return;
	}
	info.deletedLineRawStartOffset = info.newRawStartOffset;
	info.deletedLineRawEndOffset = info.newRawStartOffset +info.numRawCharsRemoved -1;
	
	struct LostTagInfo
	{
		uint32_t tagIndex;
		std::string tag;
		bool isEndTag;
	};
	// Tags lost through the deletion of the line
	std::vector<LostTagInfo> lostTagInfos {};
	for(auto it=m_subTextTags.begin();it!=m_subTextTags.end();)
	{
		auto tagIndex = static_cast<uint32_t>(it -m_subTextTags.begin());
		auto &tagInfo = *it;
		auto tagStartOffset = tagInfo.rangeOpenTag.offset;
		auto tagEndOffset = tagInfo.rangeClosingTag.GetEndOffset();
		auto startTagInDeletedRow = umath::between<int32_t>(tagStartOffset,info.deletedLineRawStartOffset,info.deletedLineRawEndOffset);
		auto endTagInDeletedRow = umath::between<int32_t>(tagEndOffset,info.deletedLineRawStartOffset,info.deletedLineRawEndOffset);
		if(startTagInDeletedRow && endTagInDeletedRow)
		{
			// Both tags are within deleted row, we can just delete the tag immediately
			it = m_subTextTags.erase(it);
			continue;
		}
		if(startTagInDeletedRow)
		{
			// Start tag is within deleted row
			lostTagInfos.push_back({tagIndex,tagInfo.GetOpenTag(**m_text),false});
		}
		if(endTagInDeletedRow)
		{
			// End tag is within deleted row
			lostTagInfos.push_back({tagIndex,tagInfo.GetClosingTag(**m_text),true});
		}
		++it;
	}

	auto rawTextShiftOffset = static_cast<int32_t>(info.numRawCharsRemoved);

	auto visTextShiftOffset = info.numVisCharsRemoved;
	m_rawTextIndexToVisibleTextIndex.erase(m_rawTextIndexToVisibleTextIndex.begin() +info.newRawStartOffset,m_rawTextIndexToVisibleTextIndex.begin() +info.newRawStartOffset +rawTextShiftOffset);
	m_visibleTextIndexToRawTextIndex.erase(m_visibleTextIndexToRawTextIndex.begin() +info.newVisStartOffset,m_visibleTextIndexToRawTextIndex.begin() +info.newVisStartOffset +visTextShiftOffset);

	// Update new offsets
	for(auto i=info.newRawStartOffset;i<m_rawTextIndexToVisibleTextIndex.size();++i)
	{
		auto &idx = m_rawTextIndexToVisibleTextIndex.at(i);
		idx = umath::max(static_cast<int32_t>(idx) -static_cast<int32_t>(visTextShiftOffset),0);
	}
	for(auto &idx : m_visibleTextIndexToRawTextIndex)
	{
		if(umath::between(idx,info.deletedLineRawStartOffset,info.deletedLineRawEndOffset))
			idx = (info.deletedLineRawStartOffset > 0) ? (info.deletedLineRawStartOffset -1) : 0;
		else if(idx >= info.deletedLineRawEndOffset)
			idx -= rawTextShiftOffset;
	}
	first = true;
	for(auto i=*lineIndex;i<m_lines.size();++i)
	{
		auto &lineInfo = m_lines.at(i);
		if(first)
		{
			lineInfo.textRange.offset = info.newRawStartOffset;
			lineInfo.visTextRange.offset = info.newVisStartOffset;
			first = false;
		}
		else
		{
			lineInfo.textRange.offset = lineInfo.textRange.offset -info.numRawCharsRemoved;
			lineInfo.visTextRange.offset -= info.numVisCharsRemoved;
		}
		
		// Update buffer range offsets
		for(auto &bufInfo : lineInfo.buffers)
		{
			if(bufInfo.range.offset >= info.newRawStartOffset)
				bufInfo.range.offset -= rawTextShiftOffset;
		}
	}

	struct TagOffsetInfo
	{
		uint32_t tagIndex;
		int32_t offset;
		bool isEndTag;
	};
	std::queue<TagOffsetInfo> tagOffsetUpdateQueue = {};
	for(auto it=m_subTextTags.begin();it!=m_subTextTags.end();++it)
	{
		auto tagIndex = it -m_subTextTags.begin();
		auto &tagInfo = *it;
		if(tagInfo.rangeOpenTag.offset >= info.newRawStartOffset)
		{
			if(tagInfo.rangeOpenTag.offset > info.deletedLineRawEndOffset)
				tagOffsetUpdateQueue.push({static_cast<uint32_t>(tagIndex),tagInfo.rangeOpenTag.offset,false});
			tagInfo.rangeOpenTag.offset -= rawTextShiftOffset;
		}
		if(tagInfo.rangeClosingTag.length != 0 && tagInfo.rangeClosingTag.offset >= info.newRawStartOffset)
		{
			if(tagInfo.rangeClosingTag.offset > info.deletedLineRawEndOffset)
				tagOffsetUpdateQueue.push({static_cast<uint32_t>(tagIndex),tagInfo.rangeClosingTag.offset,true});
			tagInfo.rangeClosingTag.offset -= rawTextShiftOffset;
		}
	}

	auto oldText = **m_text;
	(*m_text)->erase((*m_text)->begin() +info.newRawStartOffset,(*m_text)->begin() +info.newRawStartOffset +info.numRawCharsRemoved);
	m_visibleText.erase(m_visibleText.begin() +info.newVisStartOffset,m_visibleText.begin() +info.newVisStartOffset +info.numVisCharsRemoved);

	for(auto i=0u;i<m_visibleTextIndexToRawTextIndex.size();++i)
	{
		if(m_visibleTextIndexToRawTextIndex.at(i) >= (*m_text)->size())
			std::cout<<"ERROR"<<std::endl;
	}

	if(lostTagInfos.empty() == false)
	{
		// Move leftover tags into end of previous line if possible, otherwise into beginning of next line
		auto moveIntoPrevLine = *lineIndex > 0;
		auto lineInsertIndex = moveIntoPrevLine ? (*lineIndex -1) : *lineIndex;
		auto &tgtLine = m_lines.at(lineInsertIndex);
		auto insertPos = moveIntoPrevLine ? (tgtLine.textRange.GetEndOffset() +1) : tgtLine.textRange.offset;
		std::string tagLeftovers = "";
		for(auto &lostTagInfo : lostTagInfos)
		{
			auto &tagInfo = m_subTextTags.at(lostTagInfo.tagIndex);
			auto newOffset = insertPos +tagLeftovers.length();
			if(lostTagInfo.isEndTag == false)
				tagInfo.rangeOpenTag.offset = newOffset;
			else
				tagInfo.rangeClosingTag.offset = newOffset;
			tagLeftovers += lostTagInfo.tag;
		}

		// Update offsets of all other tags affected by the insertion of the leftover tags
		while(tagOffsetUpdateQueue.empty() == false)
		{
			auto &tagOffsetUpdateInfo = tagOffsetUpdateQueue.front();
			auto &tagInfo = m_subTextTags.at(tagOffsetUpdateInfo.tagIndex);
			if(tagOffsetUpdateInfo.isEndTag == false)
				tagInfo.rangeOpenTag.offset += tagLeftovers.length();
			else
				tagInfo.rangeClosingTag.offset += tagLeftovers.length();
			tagOffsetUpdateQueue.pop();
		}

		tgtLine.textRange.length += tagLeftovers.length();

		(*m_text)->insert((*m_text)->begin() +insertPos,tagLeftovers.begin(),tagLeftovers.end());
		
		// Now add the length of the inserted tag string to all ranges after the end offset of the previous line
		std::vector<uint32_t> insertIndices(tagLeftovers.size(),m_rawTextIndexToVisibleTextIndex.at(umath::max(insertPos -1,0)));
		m_rawTextIndexToVisibleTextIndex.insert(m_rawTextIndexToVisibleTextIndex.begin() +insertPos,insertIndices.begin(),insertIndices.end());

		for(auto &idx : m_visibleTextIndexToRawTextIndex)
		{
			if(idx >= insertPos)
				idx += tagLeftovers.size();
		}

	for(auto i=0u;i<m_visibleTextIndexToRawTextIndex.size();++i)
	{
		if(m_visibleTextIndexToRawTextIndex.at(i) >= (*m_text)->size())
			std::cout<<"ERROR"<<std::endl;
	}

		for(auto i=(moveIntoPrevLine ? *lineIndex : (*lineIndex +1));i<m_lines.size();++i)
		{
			auto &lineInfo = m_lines.at(i);
			lineInfo.textRange.offset += tagLeftovers.length();

			// Update buffer range offsets
			for(auto &bufInfo : lineInfo.buffers)
				bufInfo.range.offset += tagLeftovers.length();
		}
	}
	
	// Initialize line string views objects
	UpdateLineStringViewObjects(*lineIndex);
	// TODO: Update width in pixels

	///for(auto &lineInfo : m_lines)
	//	lineInfo.ClearOverlays(); // TODO: Without this color tags don't work properly
	//for(auto &tagInfo : m_subTextTags)
	//	tagInfo.ClearOverlays();
	//for(auto &lineInfo : m_lines)
	//	lineInfo.bufferUpdateRequired = true; // TODO: Without this color tags don't work properly
	//m_flags |= Flags::ApplySubTextTags;

	PerformTextPostProcessing();
	CallCallbacks<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged",std::reference_wrapper<const std::string>(oldText),std::reference_wrapper<const std::string>(**m_text));
}
void WIText::PopFrontLine()
{
	RemoveLine(0u);
	std::stringstream ss;
	if(Validate(ss) == false)
	{
		std::cout<<"Remove line validation error:\n"<<ss.str()<<std::endl;
	}
}
void WIText::PopBackLine()
{
	if(m_lineCount == 0u)
		return;
	RemoveLine(m_lineCount -1u);
}
#ifdef WITEXT_VALIDATION_ENABLED
bool WIText::UnitTest()
{
	enum class Action : uint32_t
	{
		InsertLine = 0u,
		RemoveLine,
		Clear
	};
	auto result = false;
	std::function<void(Action action,const std::string &line,uint32_t position)> fUnitTest = nullptr;
	fUnitTest = [this,&result,&fUnitTest](Action action,const std::string &line="",uint32_t position=std::numeric_limits<uint32_t>::max()) {
		std::stringstream msg;
		if(GetLineCount() > 5 && (action != Action::RemoveLine || position != 0u))
			fUnitTest(Action::RemoveLine,"",0u);
		auto currentText = **m_text;
		std::vector<std::string> currentLines {};
		currentLines.reserve(m_lines.size());
		for(auto &lineInfo : m_lines)
			currentLines.push_back(std::string{lineInfo.line});
		switch(action)
		{
			case Action::InsertLine:
				AppendLine(line);
				break;
			case Action::RemoveLine:
				RemoveLine(position);
				break;
			case Action::Clear:
				SetText("");
				break;
		}
		if(Validate(msg) == true)
			return;
		result = false;
		std::cout<<msg.str()<<std::endl;
		std::cout<<"Validation Error occured with the following text:"<<std::endl;
		std::cout<<currentText<<std::endl;
		std::cout<<"The last action was:"<<std::endl;
		std::string actionName;
		switch(action)
		{
			case Action::InsertLine:
				actionName = "InsertLine";
				break;
			case Action::RemoveLine:
				actionName = "RemoveLine";
				break;
			case Action::Clear:
				actionName = "Clear";
				break;
		}
		std::cout<<"Action: "<<actionName<<std::endl;
		std::cout<<"Action Line: "<<line<<std::endl;
		std::cout<<"Action Line Index: "<<position<<std::endl;
	};
	const auto fUnitTestAppendLine = [&fUnitTest](const std::string &line) {
		fUnitTest(Action::InsertLine,line,std::numeric_limits<uint32_t>::max());
	};
	fUnitTestAppendLine("A");
	fUnitTestAppendLine("B");
	fUnitTestAppendLine("C");
	fUnitTestAppendLine("D");
	fUnitTestAppendLine("E");
	fUnitTestAppendLine("F");
	fUnitTestAppendLine("G");
	fUnitTestAppendLine("H");
	fUnitTest(Action::RemoveLine,"",1u);
	fUnitTest(Action::RemoveLine,"",3u);
	fUnitTest(Action::RemoveLine,"",6u);
	fUnitTest(Action::InsertLine,"[c]",2u);
	fUnitTest(Action::InsertLine,"x[/c]",6u);
	fUnitTest(Action::InsertLine,"[u]",0u);
	fUnitTestAppendLine("[/u]");

	const std::array<std::string,5> randomTags = {
		"[c:ff00]",
		"[c:00ff]",
		"[/c]",
		"[u]",
		"[/u]"
	};

	auto numTestCases = 100'000u;
	for(auto i=decltype(numTestCases){0u};i<numTestCases;++i)
	{
		auto action = (umath::random(0,4) <= 3) ? Action::InsertLine : Action::RemoveLine;
		std::string text = "";
		auto numChars = umath::random(0,30);
		text.reserve(numChars);
		for(auto j=decltype(numChars){0u};j<numChars;++j)
		{
			if(false)//umath::random(0,30) == 0)
			{
				auto &tag = randomTags.at(umath::random(0,randomTags.size() -1));
				text += tag;
			}
			else
			{
				auto c = static_cast<char>(umath::random(32,126));
				text += c;
			}
		}
		auto lineId = umath::random(0,m_lines.size());
		fUnitTest(action,text,lineId);
	}

	return result;
}

bool WIText::Validate(std::stringstream &outMsg) const
{
	if((*m_text)->empty())
		return true;
	outMsg<<"----- Text validation for "<<GetName()<<" -----"<<"\n";
	std::string prefix = "VALIDATION ERROR: ";
	auto result = true;
	auto charOffset = 0u;
	for(auto &lineInfo : m_lines)
	{
		if(lineInfo.textRange.offset < 0 || lineInfo.textRange.GetEndOffset() > static_cast<int32_t>((*m_text)->size()))
		{
			outMsg<<prefix<<"Invalid line offset into text: "<<lineInfo.textRange.offset<<"!"<<"\n";
			result = false;
		}
		if(lineInfo.visTextRange.offset < 0 || lineInfo.visTextRange.GetEndOffset() > static_cast<int32_t>((*m_text)->size()))
		{
			outMsg<<prefix<<"Invalid line offset into visible text: "<<lineInfo.textRange.offset<<"!"<<"\n";
			result = false;
		}
		if(lineInfo.visTextRange.offset != charOffset)
		{
			outMsg<<prefix<<"Unexpected line offset!"<<"\n";
			result = false;
		}
		charOffset += lineInfo.visTextRange.length;
		if(lineInfo.newLine)
			++charOffset;
		for(auto &lineInfoOther : m_lines)
		{
			if(&lineInfoOther == &lineInfo)
				continue;
			if(
				(lineInfo.textRange.offset >= lineInfoOther.textRange.offset &&
				lineInfo.textRange.offset <= lineInfoOther.textRange.GetEndOffset()) ||
				(lineInfo.textRange.GetEndOffset() >= lineInfoOther.textRange.offset &&
				lineInfo.textRange.GetEndOffset() <= lineInfoOther.textRange.GetEndOffset()) ||
				(lineInfo.visTextRange.offset >= lineInfoOther.visTextRange.offset &&
				lineInfo.visTextRange.offset <= lineInfoOther.visTextRange.GetEndOffset()) ||
				(lineInfo.visTextRange.GetEndOffset() >= lineInfoOther.visTextRange.offset &&
				lineInfo.visTextRange.GetEndOffset() <= lineInfoOther.visTextRange.GetEndOffset())
			)
			{
				outMsg<<prefix<<"Overlapping line ranges!"<<"\n";
				result = false;
			}
		}
		outMsg<<"------------------------"<<"\n";
		outMsg<<"Line: "<<ustring::substr(**m_text,lineInfo.textRange.offset,lineInfo.textRange.length)<<"\n";
		outMsg<<"Visible Line: "<<ustring::substr(m_visibleText,lineInfo.visTextRange.offset,lineInfo.visTextRange.length)<<"\n";
		for(auto &bufInfo : lineInfo.buffers)
		{
			if(bufInfo.range.offset < lineInfo.textRange.offset || bufInfo.range.GetEndOffset() > lineInfo.textRange.GetEndOffset())
			{
				outMsg<<prefix<<"Buffer range out of bounds of line!"<<"\n";
				result = false;
			}
		}
	}
	if(m_lines.empty() == false && m_lines.front().textRange.offset != 0)
	{
		outMsg<<prefix<<"First line does not start at offset 0!"<<"\n";
		result = false;
	}

	for(auto &tagInfo : m_subTextTags)
	{
		if(tagInfo.rangeOpenTag.offset < 0 || tagInfo.rangeOpenTag.length < 0 || tagInfo.rangeClosingTag.offset < 0 || tagInfo.rangeClosingTag.length < 0 || tagInfo.rangeClosingTag.GetEndOffset() > static_cast<int32_t>((*m_text)->size()))
		{
			outMsg<<prefix<<"Invalid tag line offset into text!"<<"\n";
			result = false;
			continue;
		}
		auto tagString = tagInfo.GetTagString(**m_text);
		outMsg<<"------------------------"<<"\n";
		outMsg<<"Tag String: "<<tagString<<"\n";
		
		auto firstChar = (*m_text)->at(tagInfo.rangeOpenTag.offset);
		outMsg<<"First Char: "<<firstChar<<"\n";
		if(tagInfo.rangeClosingTag.length == 0)
			outMsg<<"Tag is incomplete!"<<"\n";
		else
		{
			auto lastChar = (*m_text)->at(tagInfo.rangeClosingTag.GetEndOffset());
			outMsg<<"Last Char: "<<lastChar<<"\n";
			if(lastChar != ']')
			{
				outMsg<<prefix<<"Invalid subtext closing tag!"<<"\n";
				result = false;
			}
		}
		if(firstChar != '[')
		{
			outMsg<<prefix<<"Invalid subtext opening tag!"<<"\n";
			result = false;
		}

		outMsg<<"Start Tag: "<<tagInfo.GetOpenTag(**m_text)<<"\n";
		outMsg<<"End Tag: "<<tagInfo.GetClosingTag(**m_text)<<"\n";
	}
	if(m_rawTextIndexToVisibleTextIndex.size() > (*m_text)->size())
	{
		outMsg<<prefix<<"Raw to visible index array has incorrect size!"<<"\n";
		result = false;
	}
	if(m_visibleTextIndexToRawTextIndex.size() > m_visibleText.size())
	{
		outMsg<<prefix<<"Visible to raw index array has incorrect size!"<<"\n";
		result = false;
	}
	for(auto visIdx : m_rawTextIndexToVisibleTextIndex)
	{
		if(visIdx >= m_visibleText.size())
		{
			outMsg<<prefix<<"Raw to visible index value out of range!"<<"\n";
			result = false;
		}
	}
	for(auto rawIdx : m_visibleTextIndexToRawTextIndex)
	{
		if(rawIdx >= (*m_text)->size())
		{
			outMsg<<prefix<<"Raw to visible index value out of range!"<<"\n";
			result = false;
		}
	}
	auto nextOffset = 0u;
	for(auto visIdx : m_rawTextIndexToVisibleTextIndex)
	{
		if(visIdx == nextOffset)
			++nextOffset;
	}
	if(nextOffset != m_visibleText.size())
	{
		outMsg<<prefix<<"Raw to visible index array is missing characters!"<<"\n";
		result = false;
	}
	nextOffset = 0u;
	for(auto rawIdx : m_visibleTextIndexToRawTextIndex)
	{
		if(rawIdx < 0 || rawIdx >= (*m_text)->size())
		{
			outMsg<<prefix<<"Visible to raw index array contains characters out of range!"<<"\n";
			result = false;
		}
	}
	outMsg<<"-----------------------------------------------"<<"\n";
	return result;
}
#endif
#pragma optimize("",on)
