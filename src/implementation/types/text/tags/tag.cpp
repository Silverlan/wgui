// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :text_tags;

pragma::gui::WITextDecorator::WITextDecorator(types::WIText &text) : m_text {text} {}
pragma::gui::WITextDecorator::~WITextDecorator() { Clear(); }
void pragma::gui::WITextDecorator::Initialize() {}
void pragma::gui::WITextDecorator::Apply() { SetDirty(false); }
const pragma::string::AnchorPoint *pragma::gui::WITextDecorator::GetStartAnchorPoint() const { return const_cast<WITextDecorator *>(this)->GetStartAnchorPoint(); }
const pragma::string::AnchorPoint *pragma::gui::WITextDecorator::GetEndAnchorPoint() const { return const_cast<WITextDecorator *>(this)->GetEndAnchorPoint(); }
pragma::string::TextOffset pragma::gui::WITextDecorator::GetStartTextCharOffset() const
{
	auto *pStartAnchorPoint = GetStartAnchorPoint();
	if(pStartAnchorPoint == nullptr)
		return string::END_OF_TEXT;
	return pStartAnchorPoint->GetTextCharOffset();
}
pragma::string::TextOffset pragma::gui::WITextDecorator::GetEndTextCharOffset() const
{
	auto *pEndAnchorPoint = GetEndAnchorPoint();
	if(pEndAnchorPoint == nullptr)
		return string::END_OF_TEXT;
	return pEndAnchorPoint->GetTextCharOffset();
}

void pragma::gui::WITextDecorator::Clear()
{
	for(auto &hOverlay : m_overlays) {
		if(hOverlay.IsValid())
			hOverlay->Remove();
	}
	m_overlays.clear();
}
bool pragma::gui::WITextDecorator::IsValid() const { return GetStartAnchorPoint() != nullptr; }
void pragma::gui::WITextDecorator::SetDirty(bool dirty) { m_bDirty = dirty; }
bool pragma::gui::WITextDecorator::IsDirty() const { return m_bDirty; }
bool pragma::gui::WITextDecorator::IsTag() const { return false; }
void pragma::gui::WITextDecorator::InitializeOverlay(types::WIBase &overlay) {}

void pragma::gui::WITextDecorator::CreateOverlayElement(string::LineIndex lineIndex, string::TextOffset startOffset, string::TextOffset endOffset, std::vector<WIHandle> &cachedOverlays)
{
	auto startBounds = m_text.GetCharacterPixelBounds(lineIndex, startOffset);
	auto endBounds = m_text.GetCharacterPixelBounds(lineIndex, endOffset);

	WIHandle hOverlay = {};
	while(hOverlay.IsValid() == false && cachedOverlays.empty() == false) {
		hOverlay = cachedOverlays.front();
		cachedOverlays.erase(cachedOverlays.begin());
	}
	if(hOverlay.IsValid() == false) {
		auto *p = WGUI::GetInstance().Create<types::WIBase>(&m_text);
		p->SetZPos(3);
		// auto *pBg = WGUI::GetInstance().Create<WIRect>(p);
		// pBg->SetColor(Color{255,0,0,128});
		// pBg->SetAutoAlignToParent(true);
		InitializeOverlay(*p);
		hOverlay = p->GetHandle();
	}
	auto *p = hOverlay.get();
	auto pos = startBounds.first;
	auto size = endBounds.second - startBounds.first;
	CalcBounds(pos, size);
	p->SetSize(size);
	p->SetPos(pos);
	m_overlays.push_back(hOverlay);
}

void pragma::gui::WITextDecorator::CalcBounds(Vector2i &inOutPos, Vector2i &inOutSize) {}

void pragma::gui::WITextDecorator::CreateOverlayElements()
{
	string::LineIndex startLineIdx, endLineIdx;
	string::TextOffset absStartCharOffset, absEndCharOffset;
	GetTagRange(startLineIdx, endLineIdx, absStartCharOffset, absEndCharOffset);

	auto overlays = m_overlays;
	m_overlays.clear();

	TextLineIterator lineIt {m_text, startLineIdx};
	for(auto &lineInfo : lineIt) {
		if(lineInfo.lineIndex > endLineIdx)
			break;
		string::CharOffset startOffsetRelToLine, endOffsetRelToLine;
		auto subLineStartOffset = lineInfo.relCharStartOffset;
		auto subLineEndOffset = subLineStartOffset + lineInfo.charCountSubLine - 1;
		auto numChars = GetTagRange(*lineInfo.line, subLineStartOffset, subLineEndOffset, startOffsetRelToLine, endOffsetRelToLine);
		if(numChars == -1)
			break;
		if(numChars == -2)
			continue;
		CreateOverlayElement(lineInfo.lineIndex, startOffsetRelToLine, endOffsetRelToLine, overlays);
	}
	for(auto &hOverlay : overlays) {
		if(hOverlay.IsValid() == false)
			continue;
		hOverlay->Remove();
	}
}
int32_t pragma::gui::WITextDecorator::GetTagRange(const string::FormattedTextLine &line, string::CharOffset minOffsetInLine, string::CharOffset maxOffsetInLine, string::CharOffset &outTagStartOffsetInLine, string::CharOffset &outTagEndOffsetInLine) const
{
	string::LineIndex tagStartLineIdx, tagEndLineIdx;
	string::TextOffset tagStartOffsetRelToText, tagEndOffsetRelToText;
	GetTagRange(tagStartLineIdx, tagEndLineIdx, tagStartOffsetRelToText, tagEndOffsetRelToText);

	auto lineStartOffset = line.GetStartOffset();
	auto lineEndOffset = lineStartOffset + line.GetLength();
	if(tagEndOffsetRelToText < lineStartOffset)
		return -1;
	if(tagStartOffsetRelToText >= lineEndOffset)
		return -2;
	string::CharOffset tagStartOffsetRelToLine = std::max<int32_t>(tagStartOffsetRelToText - lineStartOffset, 0);
	string::CharOffset tagEndOffsetRelToLine = tagEndOffsetRelToText - lineStartOffset;

	// Offsets are relative to unformatted text, but we need them relative to the formatted text
	tagStartOffsetRelToLine = line.GetFormattedCharOffset(tagStartOffsetRelToLine);
	tagEndOffsetRelToLine = line.GetFormattedCharOffset(tagEndOffsetRelToLine);

	if(tagEndOffsetRelToLine < minOffsetInLine)
		return -1; // Tag ends before the minimum offset
	if(tagStartOffsetRelToLine > maxOffsetInLine)
		return -2; // Tag starts after the maximum offset

	// Clamp offsets to max range
	tagStartOffsetRelToLine = std::max(tagStartOffsetRelToLine, minOffsetInLine);
	tagEndOffsetRelToLine = std::min(tagEndOffsetRelToLine, maxOffsetInLine);

	// Number of characters within the range affected by the tag
	auto numCharactersAffected = tagEndOffsetRelToLine - tagStartOffsetRelToLine + 1;

	// Output
	outTagStartOffsetInLine = tagStartOffsetRelToLine;
	outTagEndOffsetInLine = tagEndOffsetRelToLine;

	return numCharactersAffected;
}
void pragma::gui::WITextDecorator::GetTagRange(string::LineIndex &startLine, string::LineIndex &endLine, string::TextOffset &startOffset, string::TextOffset &endOffset) const
{
	auto *startAnchorPoint = GetStartAnchorPoint();
	if(startAnchorPoint == nullptr || startAnchorPoint->IsValid() == false) {
		startOffset = string::END_OF_TEXT;
		endOffset = string::END_OF_TEXT;
		return;
	}
	startLine = startAnchorPoint->GetLineIndex();
	endLine = m_text.GetLineCount() - 1;

	startOffset = GetStartTextCharOffset();
	endOffset = string::END_OF_TEXT;

	auto *endAnchorPoint = GetEndAnchorPoint();
	if(endAnchorPoint && endAnchorPoint->IsValid()) {
		endLine = endAnchorPoint->GetLineIndex();
		endOffset = endAnchorPoint->GetTextCharOffset() - 1;
	}
}

int32_t pragma::gui::WITextDecorator::GetTagRange(const string::FormattedTextLine &line, string::CharOffset &startOffset, string::CharOffset &endOffset) const
{
	string::LineIndex startLineIdx, endLineIdx;
	string::TextOffset absStartCharOffset, absEndCharOffset;
	GetTagRange(startLineIdx, endLineIdx, absStartCharOffset, absEndCharOffset);

	auto lineStartOffset = line.GetStartOffset();
	auto lineEndOffset = lineStartOffset + line.GetAbsLength() - 1;

	if(absEndCharOffset < lineStartOffset)
		return -1;
	if(absStartCharOffset > lineEndOffset)
		return -2;

	startOffset = (absStartCharOffset > lineStartOffset) ? (absStartCharOffset - lineStartOffset) : 0;
	if(absEndCharOffset == string::END_OF_TEXT)
		endOffset = line.GetLength();
	else
		endOffset = (absEndCharOffset < lineEndOffset) ? (absEndCharOffset - lineStartOffset) : (lineEndOffset - lineStartOffset);
	startOffset = line.GetFormattedCharOffset(startOffset);
	endOffset = line.GetFormattedCharOffset(endOffset);
	return endOffset - startOffset + 1; // Number of characters affected by tag within this line
}

/////////////////////

pragma::gui::WITextTag::WITextTag(types::WIText &text, string::TextTag &tag) : WITextDecorator {text}, m_tag {tag} {}
bool pragma::gui::WITextTag::IsValid() const
{
	if(m_tag.IsValid() == false)
		return false;
	auto innerRange = m_tag.GetInnerRange();
	return innerRange.has_value();
}
bool pragma::gui::WITextTag::IsTag() const { return true; }
const pragma::string::TextTag &pragma::gui::WITextTag::GetTag() const { return const_cast<WITextTag *>(this)->GetTag(); }
pragma::string::TextTag &pragma::gui::WITextTag::GetTag() { return m_tag; }
const std::vector<pragma::gui::WITextTagArgument> &pragma::gui::WITextTag::GetArguments() const { return const_cast<WITextTag *>(this)->GetArguments(); }
std::vector<pragma::gui::WITextTagArgument> &pragma::gui::WITextTag::GetArguments() { return m_args; }
pragma::string::AnchorPoint *pragma::gui::WITextTag::GetStartAnchorPoint()
{
	auto *pOpeningComponent = m_tag.GetOpeningTagComponent();
	if(pOpeningComponent == nullptr || pOpeningComponent->IsValid() == false)
		return nullptr;
	return pOpeningComponent->GetEndAnchorPoint();
}
pragma::string::AnchorPoint *pragma::gui::WITextTag::GetEndAnchorPoint()
{
	auto *pClosingComponent = m_tag.GetClosingTagComponent();
	if(pClosingComponent == nullptr || pClosingComponent->IsValid() == false)
		return nullptr;
	return pClosingComponent->GetStartAnchorPoint();
}
pragma::string::TextOffset pragma::gui::WITextTag::GetStartTextCharOffset() const
{
	auto offset = WITextDecorator::GetStartTextCharOffset();
	if(offset == string::END_OF_TEXT)
		return offset;
	return offset + 1;
}
pragma::string::TextOffset pragma::gui::WITextTag::GetEndTextCharOffset() const
{
	auto offset = WITextDecorator::GetEndTextCharOffset();
	if(offset == string::END_OF_TEXT)
		return offset;
	return offset - 1;
}
