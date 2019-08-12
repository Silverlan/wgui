/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witextentrybase.h"
#include "wgui/types/witext.h"
#include "wgui/types/witext_iterator.hpp"
#include "wgui/types/witext_tags.hpp"
#include "wgui/types/wirect.h"
#include <util_formatted_text.hpp>
#include <util_formatted_text_line.hpp>
#include <prosper_context.hpp>

LINK_WGUI_TO_CLASS(WITextEntryBase,WITextEntryBase);

WITextEntryBase::WITextEntryBase()
	: WIBase(),
	m_posCaret(0),
	m_tBlink(0),m_selectStart(-1),m_selectEnd(-1),
	m_maxLength(-1)
{
	RegisterCallback<void>("OnTextEntered");
	RegisterCallback<void>("OnContentsChanged");
	RegisterCallback<void,std::reference_wrapper<const std::string>,bool>("OnTextChanged");
}

void WITextEntryBase::OnTextContentsChanged()
{
	ClearSelection();

	if(m_hText.IsValid() == false)
		return;
	auto &elText = *static_cast<WIText*>(m_hText.get());
	if(elText.GetAutoBreakMode() != WIText::AutoBreak::NONE)
		SetHeight(m_hText->GetHeight());
	else if(elText.GetWidth() <= GetWidth())
		elText.SetLeft(0);
	else if(elText.GetRight() < GetWidth())
	{
		auto w = GetWidth();
		if(w > 0u)
			--w;
		elText.SetRight(w);
	}
	CallCallbacks<void>("OnContentsChanged");
}

void WITextEntryBase::SetInputHidden(bool b)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::HideInput) == b)
		return;
	umath::set_flag(m_stateFlags,StateFlags::HideInput,b);
	if(b)
		UpdateHiddenText();
	else
		SetText(GetText());
}

void WITextEntryBase::UpdateHiddenText()
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::HideInput) == false)
		return;
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	// TODO
	std::string hiddenText('*',m_realText.length());
	SetText(hiddenText);
}

WIText *WITextEntryBase::GetTextElement() {return static_cast<WIText*>(m_hText.get());}

void WITextEntryBase::OnTextChanged(const std::string &text,bool changedByUser)
{
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	if(pText->GetAutoBreakMode() == WIText::AutoBreak::NONE)
		pText->SizeToContents();
	CallCallbacks<void,std::reference_wrapper<const std::string>,bool>("OnTextChanged",text,changedByUser);
}

void WITextEntryBase::OnTextChanged(bool changedByUser)
{
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	OnTextChanged(pText->GetText(),changedByUser);
}

void WITextEntryBase::SizeToContents()
{
	// if(m_hText.IsValid())
	// 	m_hText->SizeToContents();
	WIBase::SizeToContents();
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
	WIText *pText = m_hText.get<WIText>();
	pText->SetColor(0,0,0,1);
	pText->SetZPos(2);
	pText->SetAutoSizeToText(true);

	auto hTe = GetHandle();
	pText->AddCallback("OnFontChanged",FunctionCallback<>::Create([hTe]() {
		if(!hTe.IsValid())
			return;
		hTe.get<WITextEntryBase>()->UpdateTextPosition();
	}));
	pText->AddCallback("OnContentsChanged",FunctionCallback<void>::Create([hTe]() {
		if(!hTe.IsValid())
			return;
		WITextEntryBase *te = hTe.get<WITextEntryBase>();
		te->OnTextContentsChanged();
	}));
	pText->AddCallback("OnTextChanged",FunctionCallback<void,std::reference_wrapper<const std::string>>::Create(std::bind([](WIHandle hTeBase,std::reference_wrapper<const std::string> text) {
		if(!hTeBase.IsValid())
			return;
		WITextEntryBase *te = hTeBase.get<WITextEntryBase>();
		te->OnTextChanged(false);
	},this->GetHandle(),std::placeholders::_1)));

	m_hCaret = CreateChild<WIRect>();
	WIRect *pRect = m_hCaret.get<WIRect>();
	pRect->SetColor(0,0,0,1);
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
	pText->GetFormattedTextObject().RemoveText(length,util::text::END_OF_TEXT);
	UpdateHiddenText();
	OnTextChanged(false);
}
int WITextEntryBase::GetMaxLength() {return m_maxLength;}

WITextEntryBase::~WITextEntryBase()
{
	KillFocus();
}

void WITextEntryBase::OnEnter()
{
	CallCallbacks<void>("OnTextEntered");
}

void WITextEntryBase::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	if(m_hText.IsValid())
	{
		WIText *pText = m_hText.get<WIText>();
		if(pText->ShouldAutoSizeToText() == false)
			pText->SetSize(x,y);
		else
			pText->SetWidth(x);
		if(m_hCaret.IsValid())
		{
			auto *font = pText->GetFont();
			if(font != NULL)
				m_hCaret.get<WIRect>()->SetSize(2,font->GetSize());
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
	if(m_hCaret.IsValid())
	{
		WIRect *pRect = m_hCaret.get<WIRect>();
		pRect->SetVisible(true);
		m_tBlink = GLFW::get_time();
	}
}

void WITextEntryBase::OnFocusKilled()
{
	WIBase::OnFocusKilled();
	if(m_hCaret.IsValid())
	{
		WIRect *pRect = m_hCaret.get<WIRect>();
		pRect->SetVisible(false);
	}
}

std::string_view WITextEntryBase::GetText() const
{
	if(!m_hText.IsValid())
		return {};
	if(umath::is_flag_set(m_stateFlags,StateFlags::HideInput))
		return m_realText;
	return static_cast<WIText*>(m_hText.get())->GetText();
}

void WITextEntryBase::SetText(std::string_view text)
{
	if(!m_hText.IsValid())
		return;
	if(m_maxLength >= 0)
		text = text.substr(0,m_maxLength);
	auto *pText = GetTextElement();
	if(pText)
		pText->SetText(text);
	SetCaretPos(GetCaretPos());
}

bool WITextEntryBase::IsSelectable() const {return umath::is_flag_set(m_stateFlags,StateFlags::Selectable);}
void WITextEntryBase::SetSelectable(bool bSelectable) {umath::set_flag(m_stateFlags,StateFlags::Selectable,bSelectable);}

bool WITextEntryBase::IsMultiLine() const {return umath::is_flag_set(m_stateFlags,StateFlags::MultiLine);}

void WITextEntryBase::SetMultiLine(bool bMultiLine) {umath::set_flag(m_stateFlags,StateFlags::MultiLine,bMultiLine);}

int WITextEntryBase::GetCaretPos() const {return m_posCaret;}
void WITextEntryBase::Think()
{
	WIBase::Think();
	if(HasFocus())
	{
		if(m_hCaret.IsValid())
		{
			WIRect *pCaret = m_hCaret.get<WIRect>();
			auto t = GLFW::get_time();
			auto tDelta = static_cast<uint32_t>((t -m_tBlink) *1.8);
			if(int(tDelta) % 2 == 0)
				pCaret->SetVisible(true);
			else
				pCaret->SetVisible(false);
		}
		auto &context = WGUI::GetInstance().GetContext();
		auto &window = context.GetWindow();
		if(window.GetMouseButtonState(GLFW::MouseButton::Left) == GLFW::KeyState::Press && m_bWasDoubleClick == false)
		{
			if(m_selectStart != -1)
			{
				int pos = GetCharPos();
				if(pos != -1)
				{
					SetCaretPos(pos);
					SetSelectionEnd(pos);
				}
			}
		}
	}
}

void WITextEntryBase::SetColor(float r,float g,float b,float a)
{
	WIBase::SetColor(r,g,b,a);
	m_hText->SetColor(r,g,b,a);
}

WIRect *WITextEntryBase::GetCaretElement() {return static_cast<WIRect*>(m_hCaret.get());}

void WITextEntryBase::SetCaretPos(int pos)
{
	m_tBlink = GLFW::get_time();
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	auto &text = pText->GetFormattedText();
	pos = umath::clamp(pos,0,static_cast<int32_t>(text.length()));
	m_posCaret = pos;
	if(m_hCaret.IsValid())
	{
		WIRect *pCaret = m_hCaret.get<WIRect>();
		if(m_hText.IsValid())
		{
			WIText *pText = m_hText.get<WIText>();
			auto &lines = pText->GetLines();
			int x = 0;
			int y = 0;
			int w;
			if(!IsMultiLine())
				y = static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(pCaret->GetHeight()) *0.5f);

			auto lineHeight = pText->GetLineHeight();
			TextLineIterator lineIt{*pText};
			auto itEnd = lineIt.end();
			for(auto it=lineIt.begin();it!=itEnd;)
			{
				auto itNext = it;
				++itNext;

				// Check if pos is in bounds of line
				auto &lineInfo = *it;
				if(itNext == itEnd || (lineInfo.charCountSubLine > 0 && pos >= lineInfo.GetAbsCharStartOffset() && pos < lineInfo.GetAbsCharStartOffset() +lineInfo.charCountSubLine))
				{
					int32_t w = 0;
					auto relPos = pos -lineInfo.GetAbsCharStartOffset();
					auto startOffset = lineInfo.relCharStartOffset;
					FontManager::GetTextSize(std::string_view{lineInfo.line->GetFormattedLine().GetText()}.substr(startOffset,relPos),startOffset,pText->GetFont(),&w,nullptr);
					x = w;
					y = lineInfo.absSubLineIndex *lineHeight;
					break;
				}
				it = itNext;
			}
			if(IsMultiLine())
				y += 2; // TODO: Why is this required?
			pCaret->SetPos(x,y);
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

int WITextEntryBase::GetCharPos(int x,int y) const
{
	if(m_hText.IsValid())
	{
		WIText *pText = m_hText.get<WIText>();
		x -= pText->GetX(); // Account for text offset to the left / right
		int lHeight = pText->GetLineHeight();
		auto &lines = pText->GetLines();
		int line = y /lHeight;

		TextLineIterator lineIt{*pText};
		auto itLine = std::find_if(lineIt.begin(),lineIt.end(),[line](const TextLineIteratorBase::Info &lineInfo) {
			return lineInfo.absSubLineIndex == line;
		});
		if(itLine != lineIt.end())
		{
			auto &lineInfo = *itLine;

			CharIterator charIt{*pText,lineInfo,true /* updatePixelWidth */};
			auto itChar = std::find_if(charIt.begin(),charIt.end(),[x](const CharIteratorBase::Info &charInfo) {
				return x < charInfo.pxOffset +charInfo.pxWidth *0.5f;
			});
			if(itChar != charIt.end())
			{
				auto &charInfo = *itChar;
				return charInfo.charOffsetRelToText;
			}
			auto charPos = lineInfo.GetAbsCharStartOffset() +lineInfo.charCountSubLine;
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
	int x,y;
	GetMousePos(&x,&y);
	return GetCharPos(x,y);
}

//#include <iostream>
util::EventReply WITextEntryBase::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	m_bWasDoubleClick = false;
	auto shiftClick = state == GLFW::KeyState::Press && umath::is_flag_set(mods,GLFW::Modifier::Shift);
	if((state == GLFW::KeyState::Release && m_selectEnd == -1) || shiftClick)
	{
		if(shiftClick && m_selectStart == -1)
			SetSelectionStart(GetCaretPos());
		if(m_selectStart != -1)
		{
			int pos = GetCharPos();
			if(pos == -1 || pos == m_selectStart)
				ClearSelection();
			else
				SetSelectionEnd(pos);
		}
	}
	else if(state == GLFW::KeyState::Press)
	{
		RequestFocus();
		if(m_hText.IsValid())
		{
			int pos = GetCharPos();
			if(pos != -1)
			{
				//std::cout<<"Pos: "<<pos<<std::endl;
				SetCaretPos(pos);
				SetSelectionStart(pos);
			}
		}
	}
	WIBase::MouseCallback(button,state,mods); // Make sure double click is called last!
	return util::EventReply::Handled;
}

int WITextEntryBase::GetLineFromPos(int pos)
{
	if(!m_hText.IsValid())
		return 0;
	WIText *pText = m_hText.get<WIText>();
	auto &lines = pText->GetLines();
	int numLines = static_cast<int>(lines.size());
	for(int i=0;i<numLines;i++)
	{
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
	GetSelectionBounds(&start,&end);
	if(m_selectStart == -1 || m_selectEnd == -1 || start == end)
	{
		if(m_selectionDecorator)
		{
			auto *pText = GetTextElement();
			if(pText)
				pText->RemoveDecorator(*m_selectionDecorator);
			m_selectionDecorator = nullptr;
		}
		return;
	}

	if(m_selectionDecorator == nullptr)
		m_selectionDecorator = GetTextElement()->AddDecorator<WITextTagSelection>(start,end);
	else
	{
		auto &tagSelection = static_cast<WITextTagSelection&>(*m_selectionDecorator.get());
		tagSelection.SetStartOffset(start);
		tagSelection.SetEndOffset(end);
	}
	m_selectionDecorator->SetDirty();
	GetTextElement()->UpdateTags();
}

void WITextEntryBase::SetSelectionBounds(int start,int end)
{
	int startLast = m_selectStart;
	int endLast = m_selectEnd;
	m_selectStart = start;
	m_selectEnd = end;
	if(start != startLast || end != endLast)
		UpdateSelection();
}

void WITextEntryBase::GetSelectionBounds(int *start,int *end)
{
	*start = m_selectStart;
	*end = m_selectEnd;
	if(*end < *start)
		umath::swap(*start,*end);
}

void WITextEntryBase::ClearSelection()
{
	SetSelectionBounds(-1,-1);
}

std::pair<util::text::LineIndex,util::text::LineIndex> WITextEntryBase::GetLineInfo(int pos,std::string_view &outLine,int *lpos) const
{
	if(!m_hText.IsValid())
		return {util::text::INVALID_LINE_INDEX,util::text::INVALID_LINE_INDEX};
	WIText *pText = m_hText.get<WIText>();

	TextLineIterator lineIt{*pText};
	for(auto &lineInfo : lineIt)
	{
		if(lineInfo.line == nullptr)
			continue;
		auto &strLine = lineInfo.line->GetFormattedLine().GetText();
		int len = lineInfo.charCountSubLine;
		if(pos < len)
		{
			outLine = std::string_view{strLine}.substr(lineInfo.relCharStartOffset,len);
			*lpos = pos;
			return {lineInfo.lineIndex,lineInfo.relSubLineIndex};
		}
		pos -= len;
	}
	return {util::text::INVALID_LINE_INDEX,util::text::INVALID_LINE_INDEX};
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
	int st,en;
	GetSelectionBounds(&st,&en);
	auto stOffset = formattedText.GetUnformattedTextOffset(st);
	auto enOffset = formattedText.GetUnformattedTextOffset(en -1u);
	if(stOffset.has_value() == false || enOffset.has_value() == false)
		return false;
	st = *stOffset;
	en = *enOffset;

	pText->GetFormattedTextObject().RemoveText(st,en -st +1);
	UpdateHiddenText();
	OnTextChanged(true);

	SetCaretPos(st);
	ClearSelection();
	return true;
}

util::EventReply WITextEntryBase::KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	if(WIBase::KeyboardCallback(key,scanCode,state,mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(state == GLFW::KeyState::Press || state == GLFW::KeyState::Repeat)
	{
		switch(key)
		{
		case GLFW::Key::Backspace:
		case GLFW::Key::Delete:
			{
				if(IsEditable() == false)
					break;
				auto *pText = GetTextElement();
				if(pText)
				{
					auto &text = pText->GetText();
					if(!text.empty())
					{
						if(RemoveSelectedText())
							break;
						if(key == GLFW::Key::Backspace)
						{
							int pos = GetCaretPos();
							if(pos > 0)
							{
								auto &formattedText = pText->GetFormattedTextObject();
								auto prevPos = formattedText.GetUnformattedTextOffset(pos -1);
								if(prevPos.has_value())
								{
									pText->RemoveText(*prevPos,1);
									OnTextChanged(true);
									SetCaretPos(*prevPos);
								}
							}
						}
						else
						{
							int pos = GetCaretPos();
							if(pos < text.length())
							{
								pText->RemoveText(pos,1);
								OnTextChanged(true);
							}
						}
					}
				}
				break;
			}
		case GLFW::Key::Left:
		case GLFW::Key::Right:
			{
				int pos = GetCaretPos();
				SetCaretPos(pos +((key == GLFW::Key::Right) ? 1 : -1));
				if((mods &GLFW::Modifier::Shift) == GLFW::Modifier::Shift)
				{
					int posNew = GetCaretPos();
					if(pos != posNew)
					{
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
		case GLFW::Key::Up:
		case GLFW::Key::Down:
			{
				if(m_hText.IsValid())
				{
					WIText *pText = m_hText.get<WIText>();
					auto &lines = pText->GetLines();
					int pos = GetCaretPos();
					std::string_view line;
					int lpos;
					auto curLine = GetLineInfo(pos,line,&lpos);
					if(curLine.first != util::text::INVALID_LINE_INDEX)
					{
						int lenOfSubStringUpToCaret;
						FontManager::GetTextSize(ustring::substr(line,0,lpos),0u,pText->GetFont(),&lenOfSubStringUpToCaret);

						TextLineIterator lineIt {*pText,curLine.first,curLine.second};
						auto it = lineIt.begin();
						if(key == GLFW::Key::Down)
							++it;
						else
							--it;
						if(it != lineIt.end())
						{
							auto &newLineInfo = *it;
							
							auto &formattedText = pText->GetFormattedTextObject();
							auto *pLine = formattedText.GetLine(newLineInfo.lineIndex);
							if(pLine)
							{
								CharIterator charIt {*pText,newLineInfo,true /* updatePixelWidth */};
								auto newPos = util::text::LAST_CHAR;
								for(auto &charInfo : charIt)
								{
									auto endOffset = charInfo.pxOffset +charInfo.pxWidth *0.5;
									if(endOffset < lenOfSubStringUpToCaret || (key == GLFW::Key::Up && endOffset == lenOfSubStringUpToCaret))
										continue;
									auto formattedPos = formattedText.GetFormattedTextOffset(pLine->GetStartOffset());
									if(formattedPos.has_value())
										newPos = *formattedPos +charInfo.charOffsetRelToLine;
									break;
								}
								if(newPos == util::text::LAST_CHAR)
								{
									auto formattedPos = formattedText.GetFormattedTextOffset(pLine->GetStartOffset());
									if(formattedPos.has_value())
										newPos = *formattedPos +pLine->GetFormattedLength();
								}

								SetCaretPos(newPos);
								if((mods &GLFW::Modifier::Shift) == GLFW::Modifier::Shift)
								{
									if(pos != newPos)
									{
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
		case GLFW::Key::Enter:
			if(!IsMultiLine() || IsEditable() == false)
			{
				if(HasFocus())
				{
					KillFocus();
					OnEnter();
				}
			}
			else
				CharCallback('\n');
			break;
		case GLFW::Key::V:
			{
				if(IsEditable() == false)
					break;
				if((mods &GLFW::Modifier::Control) == GLFW::Modifier::Control)
				{
					RemoveSelectedText();
					auto &context = WGUI::GetInstance().GetContext();
					auto &window = context.GetWindow();
					InsertText(window.GetClipboardString());
				}
				break;
			}
		case GLFW::Key::C:
		case GLFW::Key::X:
			{
				if((mods &GLFW::Modifier::Control) == GLFW::Modifier::Control)
				{
					if(m_selectStart != -1 && m_selectEnd != -1)
					{
						auto &context = WGUI::GetInstance().GetContext();
						auto &window = context.GetWindow();
						if(umath::is_flag_set(m_stateFlags,StateFlags::HideInput))
							window.SetClipboardString("");
						else
						{
							auto *pText = GetTextElement();
							if(pText)
							{
								auto &text = pText->GetFormattedText();
								int st,en;
								GetSelectionBounds(&st,&en);
								std::string sub = text.substr(st,en -st);
								window.SetClipboardString(sub);
							}
						}
						if(key == GLFW::Key::X && IsEditable())
							RemoveSelectedText();
					}
				}
				break;
			}
		case GLFW::Key::A:
			{
				if((mods &GLFW::Modifier::Control) == GLFW::Modifier::Control)
				{
					auto *pText = GetTextElement();
					if(pText)
					{
						auto &formattedTextObject = pText->GetFormattedTextObject();
						auto &formattedText = formattedTextObject.GetFormattedText();
						if(formattedText.empty())
							ClearSelection();
						else
							SetSelectionBounds(0,formattedText.length());
					}
				}
				break;
			}
		case GLFW::Key::Tab:
			return CharCallback('\t',mods);
		default:
			break;
		}
	}
	return util::EventReply::Handled;
}

util::EventReply WITextEntryBase::CharCallback(unsigned int c,GLFW::Modifier mods)
{
	//std::cout<<"CharCallback: "<<c<<std::endl;
	if(IsEditable() == false)
		return util::EventReply::Unhandled;
	if(!IsNumeric() || (c >= 48 && c <= 57) || (IsMultiLine() && c == '\n'))
	{
		std::string s = "";
		s += char(c);
		InsertText(s);
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
	const auto fIsLetterOrNumber = [](uint8_t c) {
		return (c >= 48 && c <= 57) || (c >= 65 && c <= 90) || c == 95 || (c >= 97 && c <= 122) || (c >= 128 && c <= 165) || (c >= 181 && c <= 183) || (c >= 208 && c <= 212) || (c >= 224 && c <= 237);
	};
	auto pivotPos = caretPos;
	auto startPos = pivotPos;
	while(startPos > 0 && fIsLetterOrNumber(text.at(startPos -1u)))
		--startPos;
	auto endPos = pivotPos;
	while(endPos < text.size() && fIsLetterOrNumber(text.at(endPos)))
		++endPos;
	SetSelectionBounds(startPos,endPos);
	SetCaretPos(endPos);
	return util::EventReply::Handled;
}

void WITextEntryBase::InsertText(const std::string_view &instext,int pos)
{
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	auto &formattedText = pText->GetFormattedTextObject();
	auto relOffset = formattedText.GetRelativeCharOffset(pos);
	if(relOffset.has_value())
		pText->InsertText(instext,relOffset->first,relOffset->second);
	else
		pText->AppendText(instext);
	
	UpdateHiddenText();
	OnTextChanged(true);
	SetCaretPos(GetCaretPos() +static_cast<int>(instext.length()));
}

void WITextEntryBase::InsertText(const std::string_view &text)
{
	RemoveSelectedText();
	InsertText(text,GetCaretPos());
}

bool WITextEntryBase::IsNumeric() const {return umath::is_flag_set(m_stateFlags,StateFlags::Numeric);}
void WITextEntryBase::SetNumeric(bool bNumeric)
{
	umath::set_flag(m_stateFlags,StateFlags::Numeric,bNumeric);
	auto text = GetText();
	std::string ntext = "";
	for(unsigned int i=0;i<text.size();i++)
	{
		if(text[i] >= 48 && text[i] <= 57)
			ntext += text[i];
	}
	SetText(ntext);
}

bool WITextEntryBase::IsEditable() const {return umath::is_flag_set(m_stateFlags,StateFlags::Editable);}
void WITextEntryBase::SetEditable(bool bEditable)
{
	umath::set_flag(m_stateFlags,StateFlags::Editable,bEditable);
	SetSelectable(bEditable);
	if(HasFocus())
		KillFocus();
}
