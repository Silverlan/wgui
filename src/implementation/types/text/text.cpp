// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;




module pragma.gui;

import :types.text;

decltype(WIText::s_textBuffer) WIText::s_textBuffer = nullptr;
WIText::WIText() : WIBase(), m_font(nullptr), m_breakHeight(0), m_wTexture(0), m_hTexture(0), m_autoBreak(AutoBreak::NONE), m_renderTarget(nullptr)
{
	SetColor(Color(0, 0, 0, 255));

	m_text = util::text::FormattedText::Create();
	util::text::FormattedText::Callbacks callbacks {};
	const auto fShiftBufferLines = [this](util::text::LineIndex lineIndex, int32_t shiftOffset) {
		for(auto i = lineIndex; i < m_lineInfos.size(); ++i) {
			auto &lineInfo = m_lineInfos.at(i);
			auto numBuffers = lineInfo.GetBufferCount();
			for(auto i = decltype(numBuffers) {0u}; i < numBuffers; ++i)
				lineInfo.GetBufferInfo(i)->absLineIndex += shiftOffset;
		}
	};
	callbacks.onLineAdded = [this, fShiftBufferLines](util::text::FormattedTextLine &line) {
		auto lineIdx = line.GetIndex();
		auto &lineInfo = *m_lineInfos.insert(m_lineInfos.begin() + lineIdx, LineInfo {});
		lineInfo.wpLine = line.shared_from_this();
		ScheduleRenderUpdate();
		PerformTextPostProcessing();

		fShiftBufferLines(lineIdx + 1, 1);
	};
	callbacks.onLineRemoved = [this, fShiftBufferLines](util::text::FormattedTextLine &line) {
		auto lineIdx = line.GetIndex();
		m_lineInfos.erase(m_lineInfos.begin() + lineIdx);
		SetFlag(Flags::ApplySubTextTags);
		PerformTextPostProcessing();

		fShiftBufferLines(lineIdx, -1);
	};
	callbacks.onLineChanged = [this](util::text::FormattedTextLine &line) {
		m_lineInfos.at(line.GetIndex()).bufferUpdateRequired = true;
		ScheduleRenderUpdate();
		PerformTextPostProcessing();
	};
	callbacks.onTextCleared = [this]() {
		m_lineInfos.clear();
		PerformTextPostProcessing();
	};
	callbacks.onTagAdded = [this](util::text::TextTag &tag) {
		if(tag.IsValid() == false)
			return;
		auto &openingTag = *tag.GetOpeningTagComponent();
		auto &tagName = openingTag.GetTagName();
		std::shared_ptr<WITextDecorator> pDecorator = nullptr;
		if(tagName == "color" || tagName == "c")
			pDecorator = AddDecorator<WITextTagColor>(tag);
		else if(tagName == "link" || tagName == "l")
			pDecorator = AddDecorator<WITextTagLink>(tag);
		else if(tagName == "underline" || tagName == "u")
			pDecorator = AddDecorator<WITextTagUnderline>(tag);
		else if(tagName == "tooltip" || tagName == "t")
			pDecorator = AddDecorator<WITextTagTooltip>(tag);
		else if(tagName == "template") {
			// TODO: Not yet implemented!
			// Allow insertion of custom tags after the fact
		}

		auto &label = openingTag.GetLabel();
		if(pDecorator == nullptr || label.empty())
			return;
		auto itLabel = m_labelToDecorators.find(label);
		if(itLabel == m_labelToDecorators.end())
			itLabel = m_labelToDecorators.insert(std::make_pair<std::string, std::vector<std::weak_ptr<WITextTag>>>(std::string {label}, {})).first;
		itLabel->second.push_back(std::static_pointer_cast<WITextTag>(pDecorator));
	};
	callbacks.onTagRemoved = [this](util::text::TextTag &tag) {
		auto it = std::find_if(m_tagInfos.begin(), m_tagInfos.end(), [&tag](const std::shared_ptr<WITextDecorator> &tagOther) { return tagOther->IsTag() && &static_cast<WITextTag &>(*tagOther).GetTag() == &tag; });
		if(it != m_tagInfos.end())
			m_tagInfos.erase(it);

		auto *pOpeningTagComponent = tag.GetOpeningTagComponent();
		if(pOpeningTagComponent == nullptr)
			return;

		auto &label = pOpeningTagComponent->GetLabel();
		if(label.empty())
			return;
		auto itLabel = m_labelToDecorators.find(label);
		if(itLabel == m_labelToDecorators.end())
			return;
		auto itTag = std::find_if(itLabel->second.begin(), itLabel->second.end(), [&tag](const std::weak_ptr<WITextTag> &wpTag) {
			if(wpTag.expired())
				return false;
			auto pTag = wpTag.lock();
			return pTag->IsValid() && &pTag->GetTag() == &tag;
		});
		if(itTag == itLabel->second.end())
			return;
		itLabel->second.erase(itTag);
	};
	callbacks.onTagsCleared = [this]() { m_tagInfos.clear(); };
	m_text->SetCallbacks(callbacks);
	SetTagsEnabled(false);

	auto &shaderManager = WGUI::GetInstance().GetContext().GetShaderManager();
	m_shader = shaderManager.GetShader("wguitext");

	RegisterCallback<void, std::reference_wrapper<const pragma::string::Utf8String>>("OnTextChanged");
	RegisterCallback<void, const FontInfo *>("OnFontChanged");
	RegisterCallback<void, std::reference_wrapper<const std::shared_ptr<prosper::RenderTarget>>>("OnTextRendered");
	RegisterCallback<void>("OnContentsChanged");
	RegisterCallbackWithOptionalReturn<util::EventReply, std::string>("HandleLinkTagAction");
}

WIText::~WIText()
{
	auto &context = WGUI::GetInstance().GetContext();
	context.KeepResourceAliveUntilPresentationComplete(m_renderTarget);
	context.KeepResourceAliveUntilPresentationComplete(m_shadowRenderTarget);
	context.KeepResourceAliveUntilPresentationComplete(m_shadowBlurSet);

	DestroyShadow();
	DestroyBlur();
}

void WIText::SetAutoSizeToText(bool bAutoSize) { m_bAutoSizeToText = bAutoSize; }
bool WIText::ShouldAutoSizeToText() const { return m_bAutoSizeToText; }
void WIText::UpdateTags() { SetFlag(Flags::ApplySubTextTags); }

std::string WIText::GetDebugInfo() const { return "Text: " + GetText().cpp_str(); }

std::pair<Vector2i, Vector2i> WIText::GetCharacterPixelBounds(util::text::LineIndex lineIdx, util::text::CharOffset charOffset) const
{
	if(lineIdx >= m_lineInfos.size())
		return {{0, 0}, {0, 0}};
	auto &lineInfo = m_lineInfos.at(lineIdx);
	auto &line = *m_text->GetLine(lineIdx);
	auto &strLine = line.GetFormattedLine().GetText();
	if(strLine.empty())
		return {{0, 0}, {0, 0}};
	if(charOffset >= strLine.size())
		charOffset = strLine.size() - 1;
	auto strViewLine = pragma::string::Utf8StringView {strLine};

	std::string strHidden;
	if(IsTextHidden()) {
		strHidden = std::string(strViewLine.length(), '*');
		strViewLine = strHidden;
	}

	auto subLineIndex = lineInfo.subLineIndexOffset;
	util::text::CharOffset offset = 0;
	util::text::CharOffset lineStartOffset = 0;
	for(auto numChars : lineInfo.subLines) {
		auto endOffset = offset + numChars;
		if(charOffset >= endOffset) {
			++subLineIndex;
			lineStartOffset = endOffset;
		}
		else
			break;
		offset = endOffset;
	}

	Vector2i startOffset {};
	startOffset.y = subLineIndex * GetLineHeight();
	FontManager::GetTextSize(strViewLine.substr(lineStartOffset, charOffset - lineStartOffset), 0u, GetFont(), &startOffset.x);

	auto endOffset = startOffset;
	endOffset.y += GetLineHeight();
	FontManager::GetTextSize(strViewLine.at(charOffset), charOffset, GetFont(), &endOffset.x);
	endOffset.x += startOffset.x;
	return {startOffset, endOffset};
}

void WIText::Initialize()
{
	WIBase::Initialize();
	m_baseEl = CreateChild<WITextBase>();
	auto *el = static_cast<WITextBase *>(m_baseEl.get());
	if(el != nullptr) {
		el->SetZPos(10);
		el->SetTextElement(*this);
		el->SetAutoAlignToParent(true);
		el->GetColorProperty()->Link(*GetColorProperty());
	}

	SetText("");
	SetFont(FontManager::GetDefaultFont().get());
	InitializeTextBuffer(WGUI::GetInstance().GetContext());
}

WITextBase *WIText::GetBaseElement() { return static_cast<WITextBase *>(m_baseEl.get()); }

void WIText::SetSize(int x, int y)
{
	auto oldWidth = GetWidth();
	WIBase::SetSize(x, y);
	if(x != oldWidth && m_autoBreak != AutoBreak::NONE)
		SizeToContents();
}

const FontInfo *WIText::GetFont() const { return m_font.get(); }
const std::vector<WIText::LineInfo> &WIText::GetLines() const { return const_cast<WIText *>(this)->GetLines(); }
std::vector<WIText::LineInfo> &WIText::GetLines() { return m_lineInfos; }
util::text::FormattedTextLine *WIText::GetLine(util::text::LineIndex lineIdx) { return m_text->GetLine(lineIdx); }
uint32_t WIText::GetLineCount() const { return m_text->GetLineCount(); }
uint32_t WIText::GetTextWidth() const
{
	auto *font = GetFont();
	uint32_t width = 0;
	for(auto &lineInfo : m_lineInfos) {
		if(lineInfo.subLines.empty()) {
			int32_t wChar = 0;
			int32_t wLine = 0;
			auto line = lineInfo.wpLine.lock();
			FontManager::GetTextSize(line->Substr(0, line->GetLength()), 0, font, &wLine);
			width = std::max(static_cast<int32_t>(width), wLine);
			continue;
		}
		size_t offset = 0;
		for(auto subLineLen : lineInfo.subLines) {
			auto sub = lineInfo.wpLine.lock()->Substr(offset, subLineLen);

			int32_t wChar = 0;
			int32_t wLine = 0;
			FontManager::GetTextSize(sub, 0, font, &wLine);
			width = std::max(static_cast<int32_t>(width), wLine);

			offset += subLineLen;
		}
	}
	return width;
}
uint32_t WIText::GetTotalLineCount() const
{
	if(m_lineInfos.empty())
		return 0;
	auto &lastLine = m_lineInfos.back();
	return lastLine.subLineIndexOffset + (lastLine.subLines.empty() ? 1 : lastLine.subLines.size());
}
int WIText::GetLineHeight() const { return m_font->GetSize() + m_breakHeight; }
int WIText::GetBreakHeight() { return m_breakHeight; }
void WIText::SetBreakHeight(int breakHeight) { m_breakHeight = breakHeight; }
const util::text::FormattedText &WIText::GetFormattedTextObject() const { return const_cast<WIText *>(this)->GetFormattedTextObject(); }
util::text::FormattedText &WIText::GetFormattedTextObject() { return *m_text; }
const pragma::string::Utf8String &WIText::GetText() const { return m_text->GetUnformattedText(); }
const pragma::string::Utf8String &WIText::GetFormattedText() const { return m_text->GetFormattedText(); }
void WIText::SetTabSpaceCount(uint32_t numberOfSpaces) { m_tabSpaceCount = numberOfSpaces; }
uint32_t WIText::GetTabSpaceCount() const { return m_tabSpaceCount; }
void WIText::SetFont(const std::string_view &font) { SetFont(FontManager::GetFont(font.data()).get()); }
void WIText::SetFont(const FontInfo *font, bool reload)
{
	if(m_font.get() == font && !reload)
		return;
	m_font = (font != nullptr) ? font->shared_from_this() : nullptr;
	if(m_font)
		FontManager::InitializeFontGlyphs(GetText(), *m_font);
	SetDirty();
	CallCallbacks<void, const FontInfo *>("OnFontChanged", font);
}
void WIText::ReloadFont()
{
	SetFont(m_font.get(), true);
	SetText(GetText());
}

void WIText::SetCacheEnabled(bool bEnabled)
{
	SetFlag(Flags::Cache, bEnabled);
	if(bEnabled == true || m_renderTarget == nullptr)
		return;
	WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_renderTarget);
	m_renderTarget = nullptr;
}
bool WIText::IsCacheEnabled() const { return umath::is_flag_set(m_flags, Flags::Cache); }

int WIText::GetTextHeight()
{
	if(m_font == nullptr)
		return 0;
	auto h = m_font->GetSize();
	auto numLines = GetTotalLineCount();
	return numLines * h + (((numLines > 0) ? (numLines - 1) : 0) * m_breakHeight);
}

Vector2i WIText::CalcTextSize() const
{
	Vector2i sz;
	const_cast<WIText *>(this)->GetTextSize(&sz.x, &sz.y);
	return sz;
}

void WIText::GetTextSize(int *w, int *h, const pragma::string::Utf8StringView *inText, const FontInfo *pfont)
{
	auto *font = pfont;
	if(!font)
		font = m_font.get();
	if(font == nullptr) {
		*w = 0;
		*h = 0;
		return;
	}
	int wText = 0;
	if(inText == nullptr) {
		if(pfont)
			throw std::invalid_argument {"Custom font argument must not be supplied if no text argument was given!"};
		for(auto &line : m_text->GetLines()) {
			auto &formattedLine = line->GetFormattedLine();
			int w;
			FontManager::GetTextSize(formattedLine.GetText(), 0u, m_font.get(), &w);
			if(w > wText)
				wText = w;
		}
		auto lineCount = GetTotalLineCount();
		int hText = GetLineHeight(); //m_font->GetMaxGlyphSize();//m_font->GetSize();
		*w = wText;
		// We add 3 pixels to prevent characters from being cut off in some cases
		*h = hText * lineCount + 3;
		return;
	}
	FontManager::GetTextSize(*inText, 0u, font, w);
	*h = (m_font ? m_font->GetMaxGlyphSize() : 0) + 1;
}

void WIText::SetFlag(Flags flag, bool enabled)
{
	umath::set_flag(m_flags, flag, enabled);
	if(umath::is_flag_set(m_flags, Flags::ApplySubTextTags | Flags::RenderTextScheduled | Flags::FullUpdateScheduled))
		EnableThinking();
}

void WIText::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	WIBase::Think(drawCmd);
	UpdateRenderTexture(drawCmd);
	if(umath::is_flag_set(m_flags, Flags::ApplySubTextTags | Flags::RenderTextScheduled | Flags::FullUpdateScheduled) == false)
		DisableThinking();
}

void WIText::SetDirty() { SetFlag(Flags::TextDirty); }
bool WIText::IsDirty() const { return umath::is_flag_set(m_flags, Flags::TextDirty); }

void WIText::SizeToContents(bool x, bool y)
{
	int w, h;
	GetTextSize(&w, &h);
	if(w == GetWidth() && h == GetHeight())
		return;
	if(GetAutoBreakMode() != WIText::AutoBreak::NONE) {
		ScheduleRenderUpdate();
		//if(IsTextUpdateRequired(GetText()))
		//	UpdateSubLines();
		return;
	}
	if(IsShadowEnabled() == true) {
		auto &shadowOffset = *GetShadowOffset();
		w += shadowOffset.x;
		h += shadowOffset.y;
	}
	if(x && y)
		SetSize(w, h);
	else if(x)
		SetWidth(w);
	else if(y)
		SetHeight(h);
}

WIText::AutoBreak WIText::GetAutoBreakMode() const { return m_autoBreak; }

void WIText::SetAutoBreakMode(AutoBreak b)
{
	if(b == m_autoBreak)
		return;
	m_autoBreak = b;
	SetAutoSizeToText(b != AutoBreak::NONE);
	SizeToContents();
}

void WIText::AppendText(const pragma::string::Utf8StringArg &text) { m_text->AppendText(text); }
bool WIText::InsertText(const pragma::string::Utf8StringArg &text, util::text::LineIndex lineIdx, util::text::CharOffset charOffset) { return m_text->InsertText(text, lineIdx, charOffset); }
void WIText::AppendLine(const pragma::string::Utf8StringArg &line) { m_text->AppendLine(line); }
void WIText::PopFrontLine() { m_text->PopFrontLine(); }
void WIText::PopBackLine() { m_text->PopBackLine(); }
void WIText::RemoveLine(util::text::LineIndex lineIdx) { m_text->RemoveLine(lineIdx); }
bool WIText::RemoveText(util::text::LineIndex lineIdx, util::text::CharOffset charOffset, util::text::TextLength len) { return m_text->RemoveText(lineIdx, charOffset, len); }
bool WIText::RemoveText(util::text::TextOffset offset, util::text::TextLength len) { return m_text->RemoveText(offset, len); }
bool WIText::MoveText(util::text::LineIndex lineIdx, util::text::CharOffset startOffset, util::text::TextLength len, util::text::LineIndex targetLineIdx, util::text::CharOffset targetCharOffset) { return m_text->MoveText(lineIdx, startOffset, len, targetLineIdx, targetCharOffset); }
pragma::string::Utf8StringView WIText::Substr(util::text::TextOffset startOffset, util::text::TextLength len) const { return m_text->Substr(startOffset, len); }
void WIText::Clear() { return m_text->Clear(); }
