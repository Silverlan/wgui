/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include <prosper_context.hpp>
#include <sharedutils/property/util_property_color.hpp>
#include <sharedutils/util.h>

LINK_WGUI_TO_CLASS(WIText,WIText);

#pragma optimize("",off)
void WIText::set_link_handler(const std::function<void(const std::string&)> &linkHandler)
{
	s_linkHandler = linkHandler;
}
decltype(WIText::s_textBuffer) WIText::s_textBuffer = nullptr;
decltype(WIText::s_colorBuffer) WIText::s_colorBuffer = nullptr;
decltype(WIText::s_linkHandler) WIText::s_linkHandler = nullptr;
WIText::WIText()
	: WIBase(),m_font(nullptr),m_breakHeight(0),m_wTexture(0),m_hTexture(0),
	m_autoBreak(AutoBreak::NONE),m_renderTarget(nullptr),
	m_text(util::StringProperty::Create())
{
	SetColor(Color(0,0,0,255));

	SetText("");
	SetFont(FontManager::GetDefaultFont().get());

	auto &shaderManager = WGUI::GetInstance().GetContext().GetShaderManager();
	m_shader = shaderManager.GetShader("wguitext");

	RegisterCallback<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged");
	RegisterCallback<void,const FontInfo*>("OnFontChanged");
	RegisterCallback<void,std::reference_wrapper<const std::shared_ptr<prosper::RenderTarget>>>("OnTextRendered");

	InitializeTextBuffer(WGUI::GetInstance().GetContext());
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

void WIText::SetAutoSizeToText(bool bAutoSize) {m_bAutoSizeToText = bAutoSize;}
bool WIText::ShouldAutoSizeToText() const {return m_bAutoSizeToText;}

std::string WIText::GetDebugInfo() const {return "Text: " +GetText();}

std::pair<Vector2i,Vector2i> WIText::GetCharacterPixelBounds(uint32_t charOffset) const
{
	auto itLine = std::find_if(m_lines.begin(),m_lines.end(),[charOffset](const LineInfo &lineInfo) {
		return lineInfo.IsEmpty() == false && charOffset >= lineInfo.visTextRange.offset && charOffset <= lineInfo.visTextRange.GetEndOffset();
	});
	if(itLine == m_lines.end())
		return {{-1,-1},{-1,-1}};
	auto &lineInfo = *itLine;
	auto lineOffset = itLine -m_lines.begin();
	Vector2i startOffset {};
	startOffset.y = lineOffset *GetLineHeight();
	charOffset -= lineInfo.visTextRange.offset;
	FontManager::GetTextSize(ustring::substr(lineInfo.line,0,charOffset),0u,GetFont(),&startOffset.x);

	auto endOffset = startOffset;
	endOffset.y += GetLineHeight();
	FontManager::GetTextSize(lineInfo.line.at(charOffset),charOffset,GetFont(),&endOffset.x);
	endOffset.x += startOffset.x;
	return {startOffset,endOffset};
}

 void WIText::Initialize()
{
	WIBase::Initialize();
	m_baseText = CreateChild<WITexturedRect>();
	WITexturedRect *rect = m_baseText.get<WITexturedRect>();
	if(rect != NULL)
	{
		rect->SetAlphaOnly(true);
		rect->SetZPos(1);
		rect->GetColorProperty()->Link(*GetColorProperty());
	}
}

void WIText::SetSize(int x,int y)
{
	auto oldWidth = GetWidth();
	WIBase::SetSize(x,y);
	if(x != oldWidth && m_autoBreak != AutoBreak::NONE)
		SizeToContents();
}

const FontInfo *WIText::GetFont() const {return m_font.get();}
const std::vector<WIText::LineInfo> &WIText::GetLines() const {return m_lines;}
std::string_view *WIText::GetLine(int line)
{
	if(line < 0 || line >= m_lines.size())
		return NULL;
	return &m_lines[line].line;
}
uint32_t WIText::GetLineCount() const {return m_lineCount;}
uint32_t WIText::GetTotalLineCount() const {return m_lines.size();}
int WIText::GetLineHeight() const {return m_font->GetSize() +m_breakHeight;}
int WIText::GetBreakHeight() {return m_breakHeight;}
void WIText::SetBreakHeight(int breakHeight) {m_breakHeight = breakHeight;}
const util::PStringProperty &WIText::GetTextProperty() const {return m_text;}
const std::string &WIText::GetText() const {return *m_text;}
const std::string &WIText::GetVisibleText() const {return m_visibleText;}
const std::vector<uint32_t> &WIText::GetRawTextIndicesToVisibleIndices() const {return m_rawTextIndexToVisibleTextIndex;}
const std::vector<uint32_t> &WIText::GetVisibleTextIndicesToRawIndices() const {return m_visibleTextIndexToRawTextIndex;}
void WIText::SetTabSpaceCount(uint32_t numberOfSpaces) {m_tabSpaceCount = numberOfSpaces;}
uint32_t WIText::GetTabSpaceCount() const {return m_tabSpaceCount;}
void WIText::SetFont(const std::string &font) {SetFont(FontManager::GetFont(font.c_str()).get());}
void WIText::SetFont(const FontInfo *font)
{
	if(m_font.get() == font)
		return;
	m_font = (font != nullptr) ? font->shared_from_this() : nullptr;
	SetDirty();
	SetText(*m_text);
	CallCallbacks<void,const FontInfo*>("OnFontChanged",font);
}

void WIText::SetCacheEnabled(bool bEnabled)
{
	umath::set_flag(m_flags,Flags::Cache,bEnabled);
	if(bEnabled == true || m_renderTarget == nullptr)
		return;
	WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_renderTarget);
	m_renderTarget = nullptr;
}
bool WIText::IsCacheEnabled() const {return umath::is_flag_set(m_flags,Flags::Cache);}

int WIText::GetTextHeight()
{
	if(m_font == nullptr)
		return 0;
	auto h = m_font->GetSize();
	auto numLines = static_cast<int>(m_lines.size());
	return numLines *h +(((numLines > 0) ? (numLines -1) : 0) *m_breakHeight);
}

void WIText::GetTextSize(int *w,int *h,const std::string_view *inText)
{
	if(m_font == nullptr)
	{
		*w = 0;
		*h = 0;
		return;
	}
	int wText = 0;
	if(inText == nullptr)
	{
		for(unsigned int i=0;i<m_lines.size();i++)
		{
			auto &line = m_lines[i];
			int w;
			FontManager::GetTextSize(line.line,0u,m_font.get(),&w);
			if(w > wText)
				wText = w;
		}
		int hText = GetLineHeight();//m_font->GetMaxGlyphSize();//m_font->GetSize();
		*w = wText;
		*h = hText *static_cast<int>(m_lines.size()) +1;
		return;
	}
	FontManager::GetTextSize(*inText,0u,m_font.get(),w);
	*h = m_font->GetMaxGlyphSize() +1;
}

uint32_t WIText::GetStringLength(const std::string_view &sv) const
{
	auto l = 0u;
	for(auto i=decltype(sv.length()){0u};i<sv.length();)
	{
		auto &c = sv.at(i);
		auto endIndex = ParseCodeTag(sv,i);
		if(endIndex.has_value())
		{
			i = *endIndex;
			continue;
		}
		++l;
		++i;
	}
	return l;
}

void WIText::Think()
{
	WIBase::Think();
	UpdateRenderTexture();
}

void WIText::Update()
{
	WIBase::Update();
	
}

void WIText::SetDirty() {m_flags |= Flags::TextDirty;}
bool WIText::IsDirty() const {return umath::is_flag_set(m_flags,Flags::TextDirty);}

void WIText::SizeToContents()
{
	if(GetAutoBreakMode() != WIText::AutoBreak::NONE)
	{
		if(IsTextUpdateRequired(GetText()))
		{
			SetDirty();
			SetText(GetText());
		}
		return;
	}
	int w,h;
	GetTextSize(&w,&h);
	if(IsShadowEnabled() == true)
	{
		auto &shadowOffset = *GetShadowOffset();
		w += shadowOffset.x;
		h += shadowOffset.y;
	}
	SetSize(w,h);
}

WIText::AutoBreak WIText::GetAutoBreakMode() const {return m_autoBreak;}

void WIText::SetAutoBreakMode(AutoBreak b)
{
	if(b == m_autoBreak)
		return;
	m_autoBreak = b;
	SetAutoSizeToText(b != AutoBreak::NONE);
	SizeToContents();
}
#pragma optimize("",on)
