/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witextentrybase.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include <prosper_context.hpp>

LINK_WGUI_TO_CLASS(WITextEntryBase,WITextEntryBase);

WITextEntryBase::WITextEntryBase()
	: WIBase(),
	m_posCaret(0),m_bNumeric(false),m_bMultiLine(false),m_bEditable(true),
	m_tBlink(0),m_selectStart(-1),m_selectEnd(-1),
	m_maxLength(-1),m_bHiddenInput(false)
{
	RegisterCallback<void>("OnTextEntered");
	RegisterCallback<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged");
}

void WITextEntryBase::OnTextChanged(const std::string&,const std::string&)
{

}

void WITextEntryBase::SetInputHidden(bool b)
{
	if(b == m_bHiddenInput)
		return;
	m_bHiddenInput = b;
	UpdateText(GetText());
}

WIText *WITextEntryBase::GetTextElement() {return static_cast<WIText*>(m_hText.get());}

void WITextEntryBase::UpdateText(const std::string &text)
{
	if(!m_hText.IsValid())
		return;
	auto *pText = m_hText.get<WIText>();
	if(!m_bHiddenInput)
	{
		m_text.clear();
		pText->SetText(text);
		pText->SizeToContents();
		return;
	}
	m_text = text;
	auto cpy = m_text;
	for(auto it=cpy.begin();it!=cpy.end();++it)
		*it = '*';
	pText->SetText(cpy);
	pText->SizeToContents();
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
		te->CallCallbacks<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged",oldText,text);
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
	if(!IsEditable())
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
	if(m_bHiddenInput)
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

bool WITextEntryBase::IsMultiLine() {return m_bMultiLine;}

void WITextEntryBase::SetMultiLine(bool bMultiLine)
{
	m_bMultiLine = bMultiLine;
}

int WITextEntryBase::GetCaretPos() {return m_posCaret;}
void WITextEntryBase::Think()
{
	WIBase::Think();
	if(HasFocus())
	{
		if(m_hCaret.IsValid())
		{
			WIRect *pCaret = m_hCaret.get<WIRect>();
			auto t = GLFW::get_time();
			auto tDelta = (t -m_tBlink) *1.8;
			if(int(tDelta) % 2 == 0)
				pCaret->SetVisible(true);
			else
				pCaret->SetVisible(false);
		}
		auto &context = WGUI::GetInstance().GetContext();
		auto &window = context.GetWindow();
		if(window.GetMouseButtonState(GLFW::MouseButton::Left) == GLFW::KeyState::Press)
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

void WITextEntryBase::SetCaretPos(int pos)
{
	m_tBlink = GLFW::get_time();
	std::string text = GetText();
	int l = static_cast<int>(text.length());
	if(pos > l)
		pos = l;
	if(pos < 0)
		pos = 0;
	m_posCaret = pos;
	if(m_hCaret.IsValid())
	{
		WIRect *pCaret = m_hCaret.get<WIRect>();
		if(m_hText.IsValid())
		{
			WIText *pText = m_hText.get<WIText>();
			std::vector<std::string> *lines;
			pText->GetLines(&lines);
			int x = 0;
			int y = 0;
			int w,h;
			if(!IsMultiLine())
				y = static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(pCaret->GetHeight()) *0.5f);
			for(unsigned int i=0;i<lines->size();i++)
			{
				auto &line = (*lines)[i];
				int lenLine = static_cast<int>(line.length());
				if(pos > lenLine)
				{
					y += pText->GetLineHeight();
					pos -= 1;
				}
				else
				{
					FontManager::GetTextSize(line.substr(0,pos).c_str(),pText->GetFont(),&w,&h);
					x += w;
					break;
				}
				pos -= lenLine;
			}
			w = pText->GetWidth();
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
		std::vector<std::string> *lines;
		pText->GetLines(&lines);
		int line = y /lHeight;
		size_t pos = 0;
		if(line < lines->size())
		{
			//std::cout<<"Line: "<<line<<std::endl;
			//std::cout<<"Size: "<<lines->size()<<std::endl;
			for(int i=0;i<line;i++)
				pos += (*lines)[i].length() +1;
			std::string &l = (*lines)[line];
			//std::cout<<"Mouse Pos: "<<x<<","<<y<<std::endl;
			for(unsigned int i=0;i<l.length();i++)
			{
				int width,height;
				FontManager::GetTextSize(l[i],pText->GetFont(),&width,&height);
				if(x > width *0.5)
				{
					pos += 1;
					x -= width;
				}
			}
			//pos += (*lines)[line]->line.length();
			// Move On X
			//std::cout<<"Line: "<<line<<std::endl;
			return static_cast<int>(pos);
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
void WITextEntryBase::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	WIBase::MouseCallback(button,state,mods);
	if(state == GLFW::KeyState::Press)
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
	else if(state == GLFW::KeyState::Release)
	{
		if(m_selectStart != -1)
		{
			int pos = GetCharPos();
			if(pos == -1 || pos == m_selectStart)
				ClearSelection();
			else
				SetSelectionEnd(pos);
		}
	}
}

int WITextEntryBase::GetLineFromPos(int pos)
{
	if(!m_hText.IsValid())
		return 0;
	WIText *pText = m_hText.get<WIText>();
	std::vector<std::string> *lines;
	pText->GetLines(&lines);
	int numLines = static_cast<int>(lines->size());
	for(int i=0;i<numLines;i++)
	{
		auto &line = (*lines)[i];
		pos -= static_cast<int>(line.length());
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
	std::vector<std::string> *lines;
	pText->GetLines(&lines);
	int numLines = static_cast<int>(lines->size());
	int height = pText->GetLineHeight();
	//std::cout<<"Selected Lines: "<<m_selection.size()<<"("<<lineStart<<" to "<<lineEnd<<")"<<std::endl;
	//std::cout<<"Total Pos Start: "<<start<<std::endl;
	//std::cout<<"Total Pos End: "<<end<<std::endl;
	for(int i=0;i<lineStart;i++)
	{
		auto &line = (*lines)[i];
		int l = static_cast<int>(line.length()) +1;
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
			auto &line = (*lines)[iLine];
			int xOffset;
			FontManager::GetTextSize(line.substr(0,start).c_str(),pText->GetFont(),&xOffset);
			int xSize;
			FontManager::GetTextSize(line.substr(start,end -start).c_str(),pText->GetFont(),&xSize);
			WIRect *pRect = hRect.get<WIRect>();
			//std::cout<<"-----"<<std::endl;
			//std::cout<<"Pos: "<<xOffset<<","<<height *iLine<<std::endl;
			//std::cout<<"Size: "<<xSize<<","<<height<<std::endl;
			//std::cout<<"-----"<<std::endl;
			pRect->SetPos(xOffset +xTextOffset,ySelectionOffset +height *iLine);
			pRect->SetSize(xSize,height);
			end -= static_cast<int>(line.length()) +1;
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
	text = text.substr(0,st) +text.substr(en,text.length() -en);
	SetText(text);
	ClearSelection();
}

int WITextEntryBase::GetLineInfo(int pos,std::string **line,int *lpos)
{
	if(!m_hText.IsValid())
		return -1;
	WIText *pText = m_hText.get<WIText>();
	std::vector<std::string> *lines;
	pText->GetLines(&lines);
	for(unsigned int i=0;i<lines->size();i++)
	{
		auto &cline = (*lines)[i];
		int len = static_cast<int>(cline.length());
		if(pos <= len)
		{
			*line = &cline;
			*lpos = pos;
			return i;
		}
		pos -= len +1;
	}
	return -1;
}

void WITextEntryBase::KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	WIBase::KeyboardCallback(key,scanCode,state,mods);
	//std::cout<<"KeyboardCallback: "<<key<<","<<action<<std::endl;
	if(state == GLFW::KeyState::Press)
	{
		switch(key)
		{
		case GLFW::Key::Backspace:
		case GLFW::Key::Delete:
			{
				std::string text = GetText();
				if(!text.empty())
				{
					if(m_selectStart != -1 && m_selectEnd != -1)
					{
						int st,en;
						GetSelectionBounds(&st,&en);
						std::string ntext = text.substr(0,st);
						if(m_selectEnd < text.length())
							ntext += text.substr(en,text.length() -en);
						SetText(ntext);
						SetCaretPos(st);
						ClearSelection();
						break;
					}
					if(key == GLFW::Key::Backspace)
					{
						int pos = GetCaretPos();
						if(pos > 0)
						{
							std::string ntext = text.substr(0,pos -1);
							if(pos < text.length())
								ntext += text.substr(pos,text.length() -pos);
							SetText(ntext);
							SetCaretPos(pos -1);
						}
					}
					else
					{
						int pos = GetCaretPos();
						if(pos < text.length())
						{
							std::string ntext = text.substr(0,pos) +text.substr(pos +1,text.length() -pos -1);
							SetText(ntext);
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
					std::vector<std::string> *lines;
					pText->GetLines(&lines);
					int pos = GetCaretPos();
					std::string *line;
					int lpos;
					int iline = GetLineInfo(pos,&line,&lpos);
					if(iline != -1)
					{
						int wSize;
						//std::cout<<"SUBSTRING: "<<line->line.substr(0,lpos)<<std::endl;
						FontManager::GetTextSize(line->substr(0,lpos).c_str(),pText->GetFont(),&wSize);
						auto *lnext = pText->GetLine(iline +(key == GLFW::Key::Down ? 1 : -1));
						if(lnext != NULL)
						{
							int posNew = 0;
							for(auto i=0;i<iline +(key == GLFW::Key::Down ? 1 : -1);i++)
								posNew += static_cast<int>((*lines)[i].length()) +1;
							int lenLine = static_cast<int>(lnext->length());
							for(int i=0;i<lenLine;i++)
							{
								int wChar;
								FontManager::GetTextSize((*lnext)[i],pText->GetFont(),&wChar);
								wSize -= wChar;
								if(wSize <= 0 || i == lenLine -1)
								{
									posNew += i +1;
									break;
								}
							}
							SetCaretPos(posNew);
						}
					}
				}
				break;
			}
		case GLFW::Key::Enter:
			if(!IsMultiLine())
			{
				if(HasFocus())
				{
					OnEnter();
					KillFocus();
				}
			}
			else
				CharCallback('\n');
			break;
		case GLFW::Key::V:
			{
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
						if(m_bHiddenInput == true)
							window.SetClipboardString("");
						else
						{
							std::string text = GetText();
							int st,en;
							GetSelectionBounds(&st,&en);
							std::string sub = text.substr(st,en -st);
							window.SetClipboardString(sub);
						}
						if(key == GLFW::Key::X)
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
		default:
			break;
		}
	}
}

void WITextEntryBase::CharCallback(unsigned int c,GLFW::Modifier mods)
{
	//std::cout<<"CharCallback: "<<c<<std::endl;
	if(!IsNumeric() || (c >= 48 && c <= 57) || (IsMultiLine() && c == '\n'))
	{
		std::string s = "";
		s += char(c);
		InsertText(s);
	}
}

void WITextEntryBase::InsertText(std::string instext,int pos)
{
	std::string text = GetText();
	std::string ntext = text.substr(0,pos) +instext;
	if(pos < text.length())
		ntext += text.substr(pos,text.length() -pos);
	SetText(ntext);
	SetCaretPos(GetCaretPos() +static_cast<int>(instext.length()));
}

void WITextEntryBase::InsertText(std::string text) {InsertText(text,GetCaretPos());}

bool WITextEntryBase::IsNumeric() {return m_bNumeric;}
void WITextEntryBase::SetNumeric(bool bNumeric)
{
	m_bNumeric = bNumeric;
	std::string text = GetText();
	std::string ntext = "";
	for(unsigned int i=0;i<text.size();i++)
	{
		if(text[i] >= 48 && text[i] <= 57)
			ntext += text[i];
	}
	SetText(ntext);
}

bool WITextEntryBase::IsEditable() {return m_bEditable;}
void WITextEntryBase::SetEditable(bool bEditable)
{
	m_bEditable = bEditable;
	if(HasFocus())
		KillFocus();
}
