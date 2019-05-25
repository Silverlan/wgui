/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witextentrybase.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include <prosper_context.hpp>

LINK_WGUI_TO_CLASS(WITextEntryBase,WITextEntryBase);

#pragma optimize("",off)
WITextEntryBase::WITextEntryBase()
	: WIBase(),
	m_posCaret(0),
	m_tBlink(0),m_selectStart(-1),m_selectEnd(-1),
	m_maxLength(-1)
{
	RegisterCallback<void>("OnTextEntered");
	RegisterCallback<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged");
}

void WITextEntryBase::OnTextChanged(const std::string &oldText,const std::string &text)
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
	CallCallbacks<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged",oldText,text);
}

void WITextEntryBase::SetInputHidden(bool b)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::HideInput))
		return;
	umath::set_flag(m_stateFlags,StateFlags::HideInput,b);
	UpdateText(GetText());
}

WIText *WITextEntryBase::GetTextElement() {return static_cast<WIText*>(m_hText.get());}

void WITextEntryBase::UpdateText(const std::string &text)
{
	if(!m_hText.IsValid())
		return;
	auto *pText = m_hText.get<WIText>();
	if(umath::is_flag_set(m_stateFlags,StateFlags::HideInput) == false)
	{
		m_text.clear();
		pText->SetText(text);
		//pText->SizeToContents();
		return;
	}
	m_text = text;
	auto cpy = m_text;
	for(auto it=cpy.begin();it!=cpy.end();++it)
		*it = '*';
	pText->SetText(cpy);
	//pText->SizeToContents();
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
	pText->AddCallback("OnTextChanged",FunctionCallback<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>::Create(std::bind([](WIHandle hTeBase,std::reference_wrapper<const std::string> oldText,std::reference_wrapper<const std::string> text) {
		if(!hTeBase.IsValid())
			return;
		WITextEntryBase *te = hTeBase.get<WITextEntryBase>();
		te->OnTextChanged(oldText,text);
	},this->GetHandle(),std::placeholders::_1,std::placeholders::_2)));

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
	m_maxLength = length;
	if(m_maxLength < 0)
		return;
	std::string text = GetText();
	if(text.length() > length)
	{
		text = text.substr(0,length);
		SetText(text);
	}
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

std::string WITextEntryBase::GetText()
{
	if(!m_hText.IsValid())
		return "";
	if(umath::is_flag_set(m_stateFlags,StateFlags::HideInput))
		return m_text;
	return m_hText.get<WIText>()->GetText();
}

void WITextEntryBase::SetText(std::string text)
{
	if(!m_hText.IsValid())
		return;
	if(m_maxLength >= 0)
		text = text.substr(0,m_maxLength);
	UpdateText(text);
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
	auto &text = pText->GetVisibleText();
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
			int w,h;
			if(!IsMultiLine())
				y = static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(pCaret->GetHeight()) *0.5f);
			for(unsigned int i=0;i<lines.size();i++)
			{
				auto &lineInfo = lines[i];
				auto &line = lineInfo.line;
				int lenLine = static_cast<int>(lineInfo.GetNumberOfCharacters());
				if(pos > lenLine)
					y += pText->GetLineHeight();
				else
				{
					FontManager::GetTextSize(ustring::substr(line,0,pos),0u,pText->GetFont(),&w,&h);
					x += w;
					break;
				}
				pos -= lineInfo.GetNumberOfCharacters();
			}
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
		}
	}
}

int WITextEntryBase::GetCharPos(int,int)
{
	if(m_hText.IsValid())
	{
		WIText *pText = m_hText.get<WIText>();
		int x,y;
		GetMousePos(&x,&y);
		x -= pText->GetX(); // Account for text offset to the left / right
		int lHeight = pText->GetLineHeight();
		auto &lines = pText->GetLines();
		int line = y /lHeight;
		size_t pos = 0;
		if(line < lines.size())
		{
			for(int i=0;i<line;i++)
				pos += (lines)[i].GetNumberOfCharacters();
			const auto &l = lines[line].line;
			auto offsetInLine = 0u;
			for(unsigned int i=0;i<l.length();i++)
			{
				int width,height;
				auto charWidth = FontManager::GetTextSize(l[i],offsetInLine,pText->GetFont(),&width,&height);
				if(x < width *0.5)
					break;
				++pos;
				x -= width;
				offsetInLine += charWidth;
			}
			return pos;
		}
		return -1;
	}
	return -1;
}

int WITextEntryBase::GetCharPos()
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
		auto &line = lines[i].line;
		pos -= static_cast<int>(lines[i].GetNumberOfCharacters());
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
	if(start == -1 || end == -1)
	{
		for(unsigned int i=0;i<static_cast<unsigned int>(m_selection.size());i++)
			m_selection[i]->Remove();
		m_selection.clear();
		return;
	}
	GetSelectionBounds(&start,&end);
	int lineStart = GetLineFromPos(start);
	int lineEnd = GetLineFromPos(end);
	int numLinesSelected = lineEnd -lineStart +1;
	for(int i=static_cast<int>(m_selection.size()) -1;i>=numLinesSelected;i--)
	{
		m_selection[i]->Remove();
		m_selection.pop_back();
	}
	for(int i=static_cast<int>(m_selection.size());i<numLinesSelected;i++)
	{
		WIRect *pRect = WGUI::GetInstance().Create<WIRect>(this);
		pRect->SetColor(0.75f,0.75f,0.75f);
		pRect->SetZPos(1);
		m_selection.push_back(pRect->GetHandle());
	}
	if(!m_hText.IsValid())
		return;
	WIText *pText = m_hText.get<WIText>();
	int xTextOffset = pText->GetX();
	auto &lines = pText->GetLines();
	int numLines = static_cast<int>(lines.size());
	int height = pText->GetLineHeight();
	//std::cout<<"Selected Lines: "<<m_selection.size()<<"("<<lineStart<<" to "<<lineEnd<<")"<<std::endl;
	//std::cout<<"Total Pos Start: "<<start<<std::endl;
	//std::cout<<"Total Pos End: "<<end<<std::endl;
	for(int i=0;i<lineStart;i++)
	{
		auto &line = lines[i].line;
		int l = static_cast<int>(lines[i].GetNumberOfCharacters());
		//std::cout<<"Subtracting line length "<<l<<std::endl;
		start -= l;
		end -= l;
	}
	if(start < 0 || end < 0)
	{
		for(unsigned int i=0;i<m_selection.size();i++)
			m_selection[i]->Remove();
		m_selection.clear();
	}
	//std::cout<<"Pos Start: "<<start<<std::endl;
	//std::cout<<"Pos End: "<<end<<std::endl;
	auto ySelectionOffset = 0;
	if(!IsMultiLine())
		ySelectionOffset = static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(height) *0.5f);
	for(int i=0;i<m_selection.size();i++)
	{
		WIHandle &hRect = m_selection[i];
		int iLine = lineStart +i;
		if(hRect.IsValid() && iLine < numLines)
		{
			auto &lineInfo = lines.at(iLine);
			auto &line = lineInfo.line;
			int xOffset = 0;
			auto charWidth = 0u;
			if(start > 0)
				charWidth = FontManager::GetTextSize(ustring::substr(line,0,start),0u,pText->GetFont(),&xOffset);
			int xSize = 0;
			auto l = end -start;
			if(l > 0)
				FontManager::GetTextSize(ustring::substr(line,start,l),charWidth,pText->GetFont(),&xSize);
			WIRect *pRect = hRect.get<WIRect>();
			//std::cout<<"-----"<<std::endl;
			//std::cout<<"Pos: "<<xOffset<<","<<height *iLine<<std::endl;
			//std::cout<<"Size: "<<xSize<<","<<height<<std::endl;
			//std::cout<<"-----"<<std::endl;
			pRect->SetPos(xOffset +xTextOffset,ySelectionOffset +height *iLine);
			pRect->SetSize(xSize,height);
			end -= static_cast<int>(lineInfo.GetNumberOfCharacters());
		}
		start = 0;
	}
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
	int st,en;
	if(m_selectStart < m_selectEnd)
	{
		st = m_selectStart;
		en = m_selectEnd;
	}
	else
	{
		st = m_selectEnd;
		en = m_selectStart;
	}
	*start = st;
	*end = en;
}

void WITextEntryBase::ClearSelection()
{
	SetSelectionBounds(-1,-1);
}

void WITextEntryBase::RemoveSelection()
{
	if(m_selectStart == -1 || m_selectEnd == -1)
		return;
	int st,en;
	GetSelectionBounds(&st,&en);
	std::string text = GetText();
	text = ustring::substr(text,0,st) +ustring::substr(text,en,text.length() -en);
	SetText(text);
	ClearSelection();
}

int WITextEntryBase::GetLineInfo(int pos,const std::string_view **line,int *lpos)
{
	if(!m_hText.IsValid())
		return -1;
	WIText *pText = m_hText.get<WIText>();
	auto &lines = pText->GetLines();
	for(unsigned int i=0;i<lines.size();i++)
	{
		auto &lineInfo = lines[i];
		auto &cline = lineInfo.line;
		int len = static_cast<int>(lineInfo.GetNumberOfVisibleCharacters());
		if(pos <= len)
		{
			*line = &cline;
			*lpos = pos;
			return i;
		}
		pos -= lineInfo.GetNumberOfCharacters();
	}
	return -1;
}

bool WITextEntryBase::RemoveSelectedText()
{
	if(m_selectStart == -1 || m_selectEnd == -1 || m_selectStart == m_selectEnd)
		return false;
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return false;
	auto &text = pText->GetText();
	int st,en;
	GetSelectionBounds(&st,&en);
	auto &visToRawIndices = pText->GetVisibleTextIndicesToRawIndices();
	st = visToRawIndices.at(st);
	en = visToRawIndices.at(en -1u);
	std::string ntext = ustring::substr(text,0,st);
	if(m_selectEnd < text.length())
		ntext += ustring::substr(text,en +1u,text.length() -en);
	SetText(ntext);
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
						auto &visToRawIndices = pText->GetVisibleTextIndicesToRawIndices();
						if(key == GLFW::Key::Backspace)
						{
							int pos = GetCaretPos();
							if(pos > 0)
							{
								auto prevPos = visToRawIndices.at(pos -1u);
								std::string ntext = text.substr(0,prevPos);
								if(pos < text.length())
									ntext += text.substr(visToRawIndices.at(pos),text.length() -visToRawIndices.at(pos));
								SetText(ntext);
								SetCaretPos(prevPos);
							}
						}
						else
						{
							int pos = GetCaretPos();
							if(pos < text.length())
							{
								std::string ntext = text.substr(0,visToRawIndices.at(pos)) +text.substr(visToRawIndices.at(pos) +1);
								SetText(ntext);
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
					const std::string_view *line;
					int lpos;
					int iline = GetLineInfo(pos,&line,&lpos);
					if(iline != -1)
					{
						int lenOfSubStringUpToCaret;
						FontManager::GetTextSize(ustring::substr(*line,0,lpos),0u,pText->GetFont(),&lenOfSubStringUpToCaret);
						auto *lnext = pText->GetLine(iline +(key == GLFW::Key::Down ? 1 : -1));
						if(lnext != NULL)
						{
							int posNew = 0;
							for(auto i=0;i<iline +(key == GLFW::Key::Down ? 1 : -1);i++)
								posNew += static_cast<int>(lines[i].GetNumberOfCharacters());
							int lenLine = static_cast<int>(lnext->length());
							auto wCurLen = 0u;
							auto wPrevLen = 0u;
							auto offsetInLine = 0u;
							for(int i=0;i<lenLine;i++)
							{
								int wCurChar;
								auto charWidth = FontManager::GetTextSize((*lnext)[i],offsetInLine,pText->GetFont(),&wCurChar);
								wPrevLen = wCurLen;
								wCurLen += wCurChar;
								if(wCurLen >= lenOfSubStringUpToCaret || i == lenLine -1)
								{
									posNew += i;
									if(umath::abs<int32_t>(wCurLen -lenOfSubStringUpToCaret) -umath::abs<int32_t>(wPrevLen -lenOfSubStringUpToCaret) < (wCurChar /2))
										++posNew;
									break;
								}
								offsetInLine += charWidth;
							}
							SetCaretPos(posNew);
							if((mods &GLFW::Modifier::Shift) == GLFW::Modifier::Shift)
							{
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
					RemoveSelection();
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
								auto &text = pText->GetVisibleText();
								int st,en;
								GetSelectionBounds(&st,&en);
								std::string sub = text.substr(st,en -st);
								window.SetClipboardString(sub);
							}
						}
						if(key == GLFW::Key::X && IsEditable())
							RemoveSelection();
					}
				}
				break;
			}
		case GLFW::Key::A:
			{
				if((mods &GLFW::Modifier::Control) == GLFW::Modifier::Control)
				{
					std::string text = GetText();
					if(text.length() == 0)
						ClearSelection();
					else
						SetSelectionBounds(0,static_cast<int>(text.length()));
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
	auto &text = pText->GetVisibleText();
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

void WITextEntryBase::InsertText(std::string instext,int pos)
{
	auto *pText = GetTextElement();
	if(pText == nullptr)
		return;
	auto &text = pText->GetText();
	std::string ntext = text.substr(0,pos) +instext;
	if(pos < text.length())
		ntext += text.substr(pos,text.length() -pos);
	SetText(ntext);
	if(pText->GetAutoBreakMode() == WIText::AutoBreak::NONE)
		pText->SizeToContents();
	SetCaretPos(GetCaretPos() +static_cast<int>(instext.length()));
}

void WITextEntryBase::InsertText(std::string text)
{
	RemoveSelectedText();
	InsertText(text,GetCaretPos());
}

bool WITextEntryBase::IsNumeric() const {return umath::is_flag_set(m_stateFlags,StateFlags::Numeric);}
void WITextEntryBase::SetNumeric(bool bNumeric)
{
	umath::set_flag(m_stateFlags,StateFlags::Numeric,bNumeric);
	std::string text = GetText();
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
#pragma optimize("",on)
