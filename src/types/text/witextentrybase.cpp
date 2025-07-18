// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "stdafx_wgui.h"
#include "wgui/types/witextentrybase.h"
#include "wgui/types/witext.h"
#include "wgui/types/witext_iterator.hpp"
#include "wgui/types/witext_tags.hpp"
#include "wgui/types/wirect.h"
#include <util_formatted_text.hpp>
#include <util_formatted_text_line.hpp>
#include <prosper_context.hpp>
#include <prosper_window.hpp>
#include <exprtk.hpp>
#include <codecvt>

import pragma.string.unicode;

LINK_WGUI_TO_CLASS(WITextEntryBase, WITextEntryBase);
WITextEntryBase::WITextEntryBase() : WIBase(), m_posCaret(0), m_tBlink(0), m_selectStart(-1), m_selectEnd(-1), m_maxLength(-1)
{
	RegisterCallback<void>("OnTextEntered");
	RegisterCallback<void>("OnContentsChanged");
	RegisterCallback<void, std::reference_wrapper<const pragma::string::Utf8String>, bool>("OnTextChanged");
}

void WITextEntryBase::OnTextContentsChanged()
{
	ClearSelection();

	if(m_hText.IsValid() == false)
		return;
	auto &elText = *static_cast<WIText *>(m_hText.get());
	if(elText.GetAutoBreakMode() != WIText::AutoBreak::NONE)
		SetHeight(m_hText->GetHeight());
	else if(elText.GetWidth() <= GetWidth())
		elText.SetLeft(0);
	else if(elText.GetRight() < GetWidth()) {
		auto w = GetWidth();
		if(w > 0u)
			--w;
		elText.SetRight(w);
	}
	CallCallbacks<void>("OnContentsChanged");
}

void WITextEntryBase::SetInputHidden(bool b)
{
	if(m_hText.IsValid() == false)
		return;
	static_cast<WIText *>(m_hText.get())->HideText(b);
}
bool WITextEntryBase::IsInputHidden() const { return m_hText.IsValid() ? static_cast<const WIText *>(m_hText.get())->IsTextHidden() : false; }

WIText *WITextEntryBase::GetTextElement() { return static_cast<WIText *>(m_hText.get()); }

void WITextEntryBase::OnTextChanged(const pragma::string::Utf8String &text, bool changedByUser)
{
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	if(pText->GetAutoBreakMode() == WIText::AutoBreak::NONE)
		pText->SizeToContents();
	CallCallbacks<void, std::reference_wrapper<const pragma::string::Utf8String>, bool>("OnTextChanged", text, changedByUser);
}

void WITextEntryBase::OnTextChanged(bool changedByUser)
{
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	OnTextChanged(pText->GetText(), changedByUser);
}

void WITextEntryBase::UpdateTextPosition()
{
	/*if(!m_hText.IsValid())
		return;
	auto *pText = m_hText.get<WIText>();
	auto *font = pText->GetFont();
	if(font == nullptr)
		return;
	pText->SetY(static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(font->GetSize()) *0.5f));*/
}

void WITextEntryBase::Initialize()
{
	m_hText = CreateChild<WIText>();
	WIText *pText = static_cast<WIText *>(m_hText.get());
	pText->SetColor(0, 0, 0, 1);
	pText->SetZPos(2);
	pText->SetAutoSizeToText(true);

	auto hTe = GetHandle();
	pText->AddCallback("OnFontChanged", FunctionCallback<>::Create([hTe]() mutable {
		if(!hTe.IsValid())
			return;
		static_cast<WITextEntryBase *>(hTe.get())->UpdateTextPosition();
	}));
	pText->AddCallback("OnContentsChanged", FunctionCallback<void>::Create([hTe]() mutable {
		if(!hTe.IsValid())
			return;
		WITextEntryBase *te = static_cast<WITextEntryBase *>(hTe.get());
		te->OnTextContentsChanged();
	}));
	pText->AddCallback("OnTextChanged",
	  FunctionCallback<void, std::reference_wrapper<const pragma::string::Utf8String>>::Create(std::bind(
	    [](WIHandle hTeBase, std::reference_wrapper<const pragma::string::Utf8String> text) {
		    if(!hTeBase.IsValid())
			    return;
		    WITextEntryBase *te = static_cast<WITextEntryBase *>(hTeBase.get());
		    te->OnTextChanged(false);
	    },
	    this->GetHandle(), std::placeholders::_1)));

	m_hCaret = CreateChild<WIRect>();
	WIRect *pRect = static_cast<WIRect *>(m_hCaret.get());
	pRect->SetColor(0, 0, 0, 1);
	pRect->SetVisible(false);
	pRect->SetZPos(2);

	SetKeyboardInputEnabled(true);
	SetMouseInputEnabled(true);
}

void WITextEntryBase::SetMaxLength(int length)
{
	auto *pText = GetTextElement();
	m_maxLength = length;
	if(m_maxLength < 0 || !pText)
		return;
	auto &text = pText->GetText();
	if(text.length() <= length)
		return;
	pText->GetFormattedTextObject().RemoveText(length, util::text::END_OF_TEXT);
	OnTextChanged(false);
}
int WITextEntryBase::GetMaxLength() const { return m_maxLength; }

void WITextEntryBase::SetEntryFieldElement(WIBase *el) { m_hEntryFieldElement = el ? el->GetHandle() : WIHandle {}; }

WITextEntryBase::~WITextEntryBase() { KillFocus(); }

void WITextEntryBase::OnEnter()
{
	if(IsNumeric()) {
		using T = float;
		typedef exprtk::symbol_table<T> symbol_table_t;
		typedef exprtk::expression<T> expression_t;
		typedef exprtk::parser<T> parser_t;

		const std::string expression_string = GetText();

		symbol_table_t symbol_table;
		symbol_table.add_function("deg", +[](float v) -> float { return umath::rad_to_deg(v); });
		symbol_table.add_function("rad", +[](float v) -> float { return umath::deg_to_rad(v); });

		expression_t expression;
		expression.register_symbol_table(symbol_table);

		parser_t parser;
		if(parser.compile(expression_string, expression))
			SetText(std::to_string(expression.value()));
	}

	CallCallbacks<void>("OnTextEntered");
}

void WITextEntryBase::SetSize(int x, int y)
{
	WIBase::SetSize(x, y);
	if(m_hText.IsValid()) {
		WIText *pText = static_cast<WIText *>(m_hText.get());
		if(pText->ShouldAutoSizeToText() == false)
			pText->SetSize(x, y);
		else
			pText->SetWidth(x);
		if(m_hCaret.IsValid()) {
			auto *font = pText->GetFont();
			if(font != NULL)
				static_cast<WIRect *>(m_hCaret.get())->SetSize(2, font->GetSize());
		}
		UpdateTextPosition();
		SetCaretPos(m_posCaret);
	}
}

void WITextEntryBase::RequestFocus()
{
	if(!IsSelectable())
		return;
	WIBase::RequestFocus();
}

void WITextEntryBase::OnFocusGained()
{
	WIBase::OnFocusGained();
	if(m_hCaret.IsValid()) {
		WIRect *pRect = static_cast<WIRect *>(m_hCaret.get());
		pRect->SetVisible(true);
		m_tBlink = pragma::platform::get_time();
	}
	EnableThinking();
	auto *elIme = m_hEntryFieldElement.valid() ? m_hEntryFieldElement.get() : this;
	elIme->SetIMETarget();
}

void WITextEntryBase::OnFocusKilled()
{
	WIBase::OnFocusKilled();
	if(m_hCaret.IsValid()) {
		WIRect *pRect = static_cast<WIRect *>(m_hCaret.get());
		pRect->SetVisible(false);
	}
	DisableThinking();
	ClearIMETarget();
}

pragma::string::Utf8StringView WITextEntryBase::GetText() const
{
	if(!m_hText.IsValid())
		return {};
	return static_cast<const WIText *>(m_hText.get())->GetText();
}

void WITextEntryBase::SetText(const pragma::string::Utf8StringArg &text)
{
	if(!m_hText.IsValid())
		return;
	auto view = *text;
	if(m_maxLength >= 0)
		view = view.substr(0, m_maxLength);
	auto *pText = GetTextElement();
	if(pText)
		pText->SetText(view);
	SetCaretPos(GetCaretPos());
}

bool WITextEntryBase::IsSelectable() const { return umath::is_flag_set(m_stateFlags, StateFlags::Selectable); }
void WITextEntryBase::SetSelectable(bool bSelectable) { umath::set_flag(m_stateFlags, StateFlags::Selectable, bSelectable); }

bool WITextEntryBase::IsMultiLine() const { return umath::is_flag_set(m_stateFlags, StateFlags::MultiLine); }

void WITextEntryBase::SetMultiLine(bool bMultiLine) { umath::set_flag(m_stateFlags, StateFlags::MultiLine, bMultiLine); }

int WITextEntryBase::GetCaretPos() const { return m_posCaret; }
void WITextEntryBase::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	WIBase::Think(drawCmd);
	if(HasFocus()) {
		if(m_hCaret.IsValid()) {
			WIRect *pCaret = static_cast<WIRect *>(m_hCaret.get());
			auto t = pragma::platform::get_time();
			auto tDelta = static_cast<uint32_t>((t - m_tBlink) * 1.8);
			if(int(tDelta) % 2 == 0)
				pCaret->SetVisible(true);
			else
				pCaret->SetVisible(false);
		}
		auto &context = WGUI::GetInstance().GetContext();
		auto *window = GetRootWindow();
		if(window && (*window)->GetMouseButtonState(pragma::platform::MouseButton::Left) == pragma::platform::KeyState::Press && m_bWasDoubleClick == false) {
			if(m_selectStart != -1) {
				int pos = GetCharPos();
				if(pos != -1) {
					SetCaretPos(pos);
					SetSelectionEnd(pos);
				}
			}
		}
	}
}

void WITextEntryBase::SetColor(float r, float g, float b, float a)
{
	WIBase::SetColor(r, g, b, a);
	m_hText->SetColor(r, g, b, a);
}

WIRect *WITextEntryBase::GetCaretElement() { return static_cast<WIRect *>(m_hCaret.get()); }

void WITextEntryBase::SetCaretPos(int pos)
{
	m_tBlink = pragma::platform::get_time();
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	auto &text = pText->GetFormattedText();
	pos = umath::clamp(pos, 0, static_cast<int32_t>(text.length()));
	m_posCaret = pos;
	if(m_hCaret.IsValid()) {
		WIRect *pCaret = static_cast<WIRect *>(m_hCaret.get());
		if(m_hText.IsValid()) {
			WIText *pText = static_cast<WIText *>(m_hText.get());
			auto &lines = pText->GetLines();
			int x = 0;
			int y = 0;
			int w;
			if(!IsMultiLine())
				y = static_cast<int>(static_cast<float>(GetHeight()) * 0.5f - static_cast<float>(pCaret->GetHeight()) * 0.5f);

			auto lineHeight = pText->GetLineHeight();
			TextLineIterator lineIt {*pText};
			auto itEnd = lineIt.end();
			for(auto it = lineIt.begin(); it != itEnd;) {
				auto itNext = it;
				++itNext;

				// Check if pos is in bounds of line
				auto &lineInfo = *it;
				if(itNext == itEnd || (lineInfo.charCountSubLine > 0 && pos >= lineInfo.GetAbsCharStartOffset() && pos < lineInfo.GetAbsCharStartOffset() + lineInfo.charCountSubLine)) {
					int32_t w = 0;
					auto relPos = pos - lineInfo.GetAbsCharStartOffset();
					auto startOffset = lineInfo.relCharStartOffset;
					pragma::string::Utf8StringView view {lineInfo.line->GetFormattedLine().GetText()};
					auto text = view.substr(startOffset, relPos);
					std::string strHidden;
					if(IsInputHidden()) {
						strHidden = std::string(text.length(), '*');
						text = strHidden;
					}
					FontManager::GetTextSize(text, startOffset, pText->GetFont(), &w, nullptr);
					x = w;
					y = lineInfo.absSubLineIndex * lineHeight;
					break;
				}
				it = itNext;
			}
			if(IsMultiLine())
				y += 2; // TODO: Why is this required?
			pCaret->SetPos(x, y);
			/*
			w = GetWidth();
			if((x +pCaret->GetWidth()) > (w -pText->GetX()))
			{
				int offset = x -w +pCaret->GetWidth();
				pText->SetX(-offset);
			}
			else if(x < -pText->GetX())
			{
				int offset = x;
				pText->SetX(-offset);
			}
			x += pText->GetX();
			pCaret->SetPos(x,y);
			*/
		}
	}
}

int WITextEntryBase::GetCharPos(int x, int y) const
{
	if(m_hText.IsValid()) {
		const WIText *pText = static_cast<const WIText *>(m_hText.get());
		x -= pText->GetX(); // Account for text offset to the left / right
		int lHeight = pText->GetLineHeight();
		auto &lines = pText->GetLines();
		int line = y / lHeight;

		TextLineIterator lineIt {const_cast<WIText &>(*pText)};
		auto itLine = std::find_if(lineIt.begin(), lineIt.end(), [line](const TextLineIteratorBase::Info &lineInfo) { return lineInfo.absSubLineIndex == line; });
		if(itLine != lineIt.end()) {
			auto &lineInfo = *itLine;

			if(IsInputHidden()) {
				auto len = lineInfo.line->GetUnformattedLine().GetLength();
				auto c = '*';
				int32_t width = 0;
				int32_t height = 0;
				FontManager::GetTextSize(c, 0, pText->GetFont(), &width, &height);
				auto charPos = x / static_cast<float>(width);
				auto charIdx = umath::floor(charPos);
				charPos -= charIdx;
				if(charPos >= 0.5f)
					++charIdx;
				return charIdx;
			}

			CharIterator charIt {const_cast<WIText &>(*pText), lineInfo, true /* updatePixelWidth */};
			auto itChar = std::find_if(charIt.begin(), charIt.end(), [x](const CharIteratorBase::Info &charInfo) { return x < charInfo.pxOffset + charInfo.pxWidth * 0.5f; });
			if(itChar != charIt.end()) {
				auto &charInfo = *itChar;
				return charInfo.charOffsetRelToText;
			}
			auto charPos = lineInfo.GetAbsCharStartOffset() + lineInfo.charCountSubLine;
			//if(lineInfo.isLastSubLine == false)
			--charPos;
			return charPos;
		}

		/*
		size_t pos = 0;
		if(line < lines.size())
		{
			for(int i=0;i<line;i++)
			{
				auto &lineInfo = lines.at(i);
				if(lineInfo.wpLine.expired())
					continue;
				pos += lineInfo.wpLine.lock()->GetAbsFormattedLength();
			}
			const auto &lineInfo = lines.at(line);
			auto line = lineInfo.wpLine.lock();
			auto &strLine = line->GetFormattedLine().GetText();
			auto offsetInLine = 0u;
			for(unsigned int i=0;i<strLine.length();i++)
			{
				int width,height;
				auto charWidth = FontManager::GetTextSize(strLine.at(i),offsetInLine,pText->GetFont(),&width,&height);
				if(x < width *0.5)
					break;
				++pos;
				x -= width;
				offsetInLine += charWidth;
			}
			return pos;
		}*/
		return -1;
	}
	return -1;
}

int WITextEntryBase::GetCharPos() const
{
	int x, y;
	GetMousePos(&x, &y);
	return GetCharPos(x, y);
}

//#include <iostream>
util::EventReply WITextEntryBase::MouseCallback(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods)
{
	m_bWasDoubleClick = false;
	auto shiftClick = state == pragma::platform::KeyState::Press && umath::is_flag_set(mods, pragma::platform::Modifier::Shift);
	if((state == pragma::platform::KeyState::Release && m_selectEnd == -1) || shiftClick) {
		if(shiftClick && m_selectStart == -1)
			SetSelectionStart(GetCaretPos());
		if(m_selectStart != -1) {
			int pos = GetCharPos();
			if(pos == -1 || pos == m_selectStart)
				ClearSelection();
			else
				SetSelectionEnd(pos);
		}
	}
	else if(state == pragma::platform::KeyState::Press) {
		RequestFocus();
		if(m_hText.IsValid()) {
			int pos = GetCharPos();
			if(pos != -1) {
				//std::cout<<"Pos: "<<pos<<std::endl;
				SetCaretPos(pos);
				SetSelectionStart(pos);
			}
		}
	}
	WIBase::MouseCallback(button, state, mods); // Make sure double click is called last!
	return util::EventReply::Handled;
}

int WITextEntryBase::GetLineFromPos(int pos)
{
	if(!m_hText.IsValid())
		return 0;
	WIText *pText = static_cast<WIText *>(m_hText.get());
	auto &lines = pText->GetLines();
	int numLines = static_cast<int>(lines.size());
	for(int i = 0; i < numLines; i++) {
		auto &line = lines[i];
		if(line.wpLine.expired())
			continue;
		pos -= static_cast<int>(lines[i].wpLine.lock()->GetAbsFormattedLength());
		if(pos <= 0)
			return i;
	}
	if(numLines > 0)
		numLines--;
	return numLines;
}

void WITextEntryBase::SetSelectionStart(int pos)
{
	int start = m_selectStart;
	m_selectStart = pos;
	if(pos != start)
		UpdateSelection();
}

void WITextEntryBase::SetSelectionEnd(int pos)
{
	int end = m_selectEnd;
	m_selectEnd = pos;
	if(pos != end)
		UpdateSelection();
}

void WITextEntryBase::UpdateSelection()
{
	int start = m_selectStart;
	int end = m_selectEnd;
	GetSelectionBounds(&start, &end);
	if(m_selectStart == -1 || m_selectEnd == -1 || start == end) {
		if(m_selectionDecorator) {
			auto *pText = GetTextElement();
			if(pText)
				pText->RemoveDecorator(*m_selectionDecorator);
			m_selectionDecorator = nullptr;
		}
		return;
	}

	if(m_selectionDecorator == nullptr)
		m_selectionDecorator = GetTextElement()->AddDecorator<WITextTagSelection>(start, end);
	else {
		auto &tagSelection = static_cast<WITextTagSelection &>(*m_selectionDecorator.get());
		tagSelection.SetStartOffset(start);
		tagSelection.SetEndOffset(end);
	}
	m_selectionDecorator->SetDirty();
	GetTextElement()->UpdateTags();
}

void WITextEntryBase::SetSelectionBounds(int start, int end)
{
	int startLast = m_selectStart;
	int endLast = m_selectEnd;
	m_selectStart = start;
	m_selectEnd = end;
	if(start != startLast || end != endLast)
		UpdateSelection();
}

void WITextEntryBase::GetSelectionBounds(int *start, int *end) const
{
	*start = m_selectStart;
	*end = m_selectEnd;
	if(*end < *start)
		umath::swap(*start, *end);
}

void WITextEntryBase::ClearSelection() { SetSelectionBounds(-1, -1); }

std::pair<util::text::LineIndex, util::text::LineIndex> WITextEntryBase::GetLineInfo(int pos, pragma::string::Utf8StringView &outLine, int *lpos) const
{
	if(!m_hText.IsValid())
		return {util::text::INVALID_LINE_INDEX, util::text::INVALID_LINE_INDEX};
	const WIText *pText = static_cast<const WIText *>(m_hText.get());

	TextLineIterator lineIt {const_cast<WIText &>(*pText)};
	for(auto &lineInfo : lineIt) {
		if(lineInfo.line == nullptr)
			continue;
		auto &strLine = lineInfo.line->GetFormattedLine().GetText();
		int len = lineInfo.charCountSubLine;
		if(pos < len) {
			outLine = pragma::string::Utf8StringView {strLine}.substr(lineInfo.relCharStartOffset, len);
			*lpos = pos;
			return {lineInfo.lineIndex, lineInfo.relSubLineIndex};
		}
		pos -= len;
	}
	return {util::text::INVALID_LINE_INDEX, util::text::INVALID_LINE_INDEX};
}

bool WITextEntryBase::RemoveSelectedText()
{
	if(m_selectStart == -1 || m_selectEnd == -1 || m_selectStart == m_selectEnd)
		return false;
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return false;
	auto &formattedText = pText->GetFormattedTextObject();
	auto &text = pText->GetText();
	int st, en;
	GetSelectionBounds(&st, &en);
	auto stOffset = formattedText.GetUnformattedTextOffset(st);
	auto enOffset = formattedText.GetUnformattedTextOffset(en - 1u);
	if(stOffset.has_value() == false || enOffset.has_value() == false)
		return false;
	st = *stOffset;
	en = *enOffset;

	pText->GetFormattedTextObject().RemoveText(st, en - st + 1);
	OnTextChanged(true);

	SetCaretPos(st);
	ClearSelection();
	return true;
}

util::EventReply WITextEntryBase::KeyboardCallback(pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods)
{
	if(WIBase::KeyboardCallback(key, scanCode, state, mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(state == pragma::platform::KeyState::Press || state == pragma::platform::KeyState::Repeat) {
		switch(key) {
		case pragma::platform::Key::Backspace:
		case pragma::platform::Key::Delete:
			{
				if(IsEditable() == false)
					break;
				auto *pText = GetTextElement();
				if(pText) {
					auto &text = pText->GetText();
					if(!text.empty()) {
						if(RemoveSelectedText())
							break;
						if(key == pragma::platform::Key::Backspace) {
							int pos = GetCaretPos();
							if(pos > 0) {
								auto &formattedText = pText->GetFormattedTextObject();
								auto prevPos = formattedText.GetUnformattedTextOffset(pos - 1);
								if(prevPos.has_value()) {
									pText->RemoveText(*prevPos, 1);
									OnTextChanged(true);
									SetCaretPos(*prevPos);
								}
							}
						}
						else {
							int pos = GetCaretPos();
							if(pos < text.length()) {
								pText->RemoveText(pos, 1);
								OnTextChanged(true);
							}
						}
					}
				}
				break;
			}
		case pragma::platform::Key::Left:
		case pragma::platform::Key::Right:
			{
				int pos = GetCaretPos();
				SetCaretPos(pos + ((key == pragma::platform::Key::Right) ? 1 : -1));
				if((mods & pragma::platform::Modifier::Shift) == pragma::platform::Modifier::Shift) {
					int posNew = GetCaretPos();
					if(pos != posNew) {
						if(m_selectStart == -1)
							SetSelectionStart(pos);
						SetSelectionEnd(posNew);
						if(m_selectStart == m_selectEnd)
							ClearSelection();
					}
				}
				else
					ClearSelection();
				//std::cout<<"Selection bounds: ("<<m_selectStart<<")("<<m_selectEnd<<")"<<std::endl;
				break;
			}
		case pragma::platform::Key::Up:
		case pragma::platform::Key::Down:
			{
				if(m_hText.IsValid()) {
					WIText *pText = static_cast<WIText *>(m_hText.get());
					auto &lines = pText->GetLines();
					int pos = GetCaretPos();
					pragma::string::Utf8StringView line;
					int lpos;
					auto curLine = GetLineInfo(pos, line, &lpos);
					if(curLine.first != util::text::INVALID_LINE_INDEX) {
						int lenOfSubStringUpToCaret;
						FontManager::GetTextSize(line.substr(0, lpos), 0u, pText->GetFont(), &lenOfSubStringUpToCaret);

						TextLineIterator lineIt {*pText, curLine.first, curLine.second};
						auto it = lineIt.begin();
						if(key == pragma::platform::Key::Down)
							++it;
						else
							--it;
						if(it != lineIt.end()) {
							auto &newLineInfo = *it;

							auto &formattedText = pText->GetFormattedTextObject();
							auto *pLine = formattedText.GetLine(newLineInfo.lineIndex);
							if(pLine) {
								CharIterator charIt {*pText, newLineInfo, true /* updatePixelWidth */};
								auto newPos = util::text::LAST_CHAR;
								for(auto &charInfo : charIt) {
									auto endOffset = charInfo.pxOffset + charInfo.pxWidth * 0.5;
									if(endOffset < lenOfSubStringUpToCaret || (key == pragma::platform::Key::Up && endOffset == lenOfSubStringUpToCaret))
										continue;
									auto formattedPos = formattedText.GetFormattedTextOffset(pLine->GetStartOffset());
									if(formattedPos.has_value())
										newPos = *formattedPos + charInfo.charOffsetRelToLine;
									break;
								}
								if(newPos == util::text::LAST_CHAR) {
									auto formattedPos = formattedText.GetFormattedTextOffset(pLine->GetStartOffset());
									if(formattedPos.has_value())
										newPos = *formattedPos + pLine->GetFormattedLength();
								}

								SetCaretPos(newPos);
								if((mods & pragma::platform::Modifier::Shift) == pragma::platform::Modifier::Shift) {
									if(pos != newPos) {
										if(m_selectStart == -1)
											SetSelectionStart(pos);
										SetSelectionEnd(newPos);
										if(m_selectStart == m_selectEnd)
											ClearSelection();
									}
								}
								else
									ClearSelection();
							}
						}
					}
				}
				break;
			}
		case pragma::platform::Key::Enter:
			if(!IsMultiLine() || IsEditable() == false) {
				if(HasFocus()) {
					KillFocus();
					OnEnter();
				}
			}
			else
				CharCallback('\n');
			break;
		case pragma::platform::Key::V:
			{
				if(IsEditable() == false)
					break;
				if((mods & pragma::platform::Modifier::Control) == pragma::platform::Modifier::Control) {
					RemoveSelectedText();
					auto &context = WGUI::GetInstance().GetContext();
					auto *window = GetRootWindow();
					if(window)
						InsertText((*window)->GetClipboardString());
				}
				break;
			}
		case pragma::platform::Key::C:
		case pragma::platform::Key::X:
			{
				if((mods & pragma::platform::Modifier::Control) == pragma::platform::Modifier::Control) {
					if(m_selectStart != -1 && m_selectEnd != -1) {
						auto &context = WGUI::GetInstance().GetContext();
						auto *window = GetRootWindow();
						auto *pText = GetTextElement();
						if(window) {
							if(pText == nullptr || pText->IsTextHidden())
								(*window)->SetClipboardString("");
							else {
								if(pText) {
									auto &text = pText->GetFormattedText();
									int st, en;
									GetSelectionBounds(&st, &en);
									auto sub = text.substr(st, en - st);
									(*window)->SetClipboardString(sub.cpp_str());
								}
							}
						}
						if(key == pragma::platform::Key::X && IsEditable())
							RemoveSelectedText();
					}
				}
				break;
			}
		case pragma::platform::Key::A:
			{
				if((mods & pragma::platform::Modifier::Control) == pragma::platform::Modifier::Control) {
					auto *pText = GetTextElement();
					if(pText) {
						auto &formattedTextObject = pText->GetFormattedTextObject();
						auto &formattedText = formattedTextObject.GetFormattedText();
						if(formattedText.empty())
							ClearSelection();
						else
							SetSelectionBounds(0, formattedText.length());
					}
				}
				break;
			}
		case pragma::platform::Key::Tab:
			return CharCallback('\t', mods);
		default:
			break;
		}
	}
	return util::EventReply::Handled;
}

static std::string utf8char_to_str(unsigned int c)
{
	wchar_t unicodeCharacter = static_cast<wchar_t>(c);
	std::wstring wstr;
	wstr.push_back(unicodeCharacter);
	return ustring::wstring_to_string(wstr);
}
util::EventReply WITextEntryBase::CharCallback(unsigned int c, pragma::platform::Modifier mods)
{
	//std::cout<<"CharCallback: "<<c<<std::endl;
	if(IsEditable() == false)
		return util::EventReply::Unhandled;
	if(/*!IsNumeric() ||*/ true || (c >= 48 && c <= 57) || (IsMultiLine() && c == '\n')) {
		auto *pText = GetTextElement();
		auto shouldInsert = true;
		if(pText && m_maxLength >= 0) {
			auto &text = pText->GetText();
			if(text.length() >= m_maxLength)
				shouldInsert = false;
		}
		if(shouldInsert)
			InsertText(utf8char_to_str(c));
	}
	return util::EventReply::Handled;
}

util::EventReply WITextEntryBase::OnDoubleClick()
{
	if(WIBase::OnDoubleClick() == util::EventReply::Handled)
		return util::EventReply::Handled;
	m_bWasDoubleClick = true;
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return util::EventReply::Unhandled;
	auto &text = pText->GetFormattedText();
	auto caretPos = GetCaretPos();
	if(caretPos >= text.size())
		return util::EventReply::Unhandled;
	const auto fIsLetterOrNumber = [](uint8_t c) { return (c >= 48 && c <= 57) || (c >= 65 && c <= 90) || c == 95 || (c >= 97 && c <= 122) || (c >= 128 && c <= 165) || (c >= 181 && c <= 183) || (c >= 208 && c <= 212) || (c >= 224 && c <= 237); };
	auto pivotPos = caretPos;
	auto startPos = pivotPos;
	if(startPos > 0) {
		auto it = text.begin() + (startPos - 1);
		while(startPos > 0 && fIsLetterOrNumber(*it)) {
			--startPos;
			--it;
		}
	}
	auto endPos = pivotPos;
	if(endPos < text.size()) {
		auto it = text.begin() + endPos;
		while(endPos < text.size() && fIsLetterOrNumber(*it)) {
			++endPos;
			++it;
		}
	}
	SetSelectionBounds(startPos, endPos);
	SetCaretPos(endPos);
	return util::EventReply::Handled;
}

void WITextEntryBase::InsertText(const pragma::string::Utf8StringArg &instext, int pos)
{
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	auto &formattedText = pText->GetFormattedTextObject();
	auto relOffset = formattedText.GetRelativeCharOffset(pos);
	if(relOffset.has_value())
		pText->InsertText(*instext, relOffset->first, relOffset->second);
	else
		pText->AppendText(*instext);

	OnTextChanged(true);
	SetCaretPos(GetCaretPos() + static_cast<int>(instext->length()));
}

void WITextEntryBase::InsertText(const pragma::string::Utf8StringArg &text)
{
	RemoveSelectedText();
	InsertText(text, GetCaretPos());
}

bool WITextEntryBase::IsNumeric() const { return umath::is_flag_set(m_stateFlags, StateFlags::Numeric); }
void WITextEntryBase::SetNumeric(bool bNumeric)
{
	umath::set_flag(m_stateFlags, StateFlags::Numeric, bNumeric);
	auto text = GetText();
	std::string ntext = "";
	for(auto c : text) {
		if(c >= 48 && c <= 57)
			ntext += c;
	}
	SetText(ntext);
}

bool WITextEntryBase::IsEditable() const { return umath::is_flag_set(m_stateFlags, StateFlags::Editable); }
void WITextEntryBase::SetEditable(bool bEditable)
{
	umath::set_flag(m_stateFlags, StateFlags::Editable, bEditable);
	SetSelectable(bEditable);
	if(HasFocus())
		KillFocus();
}
