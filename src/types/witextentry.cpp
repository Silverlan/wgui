/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witextentry.h"
#include "wgui/types/wirect.h"
#include "wgui/types/witext.h"
#include "wgui/types/wiarrow.h"

LINK_WGUI_TO_CLASS(WITextEntry,WITextEntry);
LINK_WGUI_TO_CLASS(WINumericEntry,WINumericEntry);

#pragma optimize("",off)
WITextEntry::WITextEntry()
	: WIBase()
{
	RegisterCallback<void>("OnTextEntered");
	RegisterCallback<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged");
}

WITextEntry::~WITextEntry()
{}

void WITextEntry::SetInputHidden(bool b)
{
	if(!m_hBase.IsValid())
		return;
	m_hBase.get<WITextEntryBase>()->SetInputHidden(b);
}

WIText *WITextEntry::GetTextElement()
{
	if(m_hBase.IsValid() == false)
		return nullptr;
	return static_cast<WITextEntryBase*>(m_hBase.get())->GetTextElement();
}

void WITextEntry::RequestFocus()
{
	if(!m_hBase.IsValid())
		return;
	m_hBase->RequestFocus();
}

bool WITextEntry::HasFocus()
{
	if(!m_hBase.IsValid())
		return false;
	return m_hBase->HasFocus();
}

void WITextEntry::SetMouseInputEnabled(bool b)
{
	WIBase::SetMouseInputEnabled(b);
	if(!m_hBase.IsValid())
		return;
	m_hBase->SetMouseInputEnabled(b);
}
void WITextEntry::SetKeyboardInputEnabled(bool b)
{
	WIBase::SetKeyboardInputEnabled(b);
	if(!m_hBase.IsValid())
		return;
	m_hBase->SetKeyboardInputEnabled(b);
}
void WITextEntry::SizeToContents()
{
	if(m_hBase.IsValid())
	{
		m_hBase->SizeToContents();
		SetSize(m_hBase->GetSize());
	}
	else
		WIBase::SizeToContents();
}
void WITextEntry::SetColor(float r,float g,float b,float a)
{
	WIBase::SetColor(r,g,b,a);
	m_hBase->SetColor(r,g,b,a);
}
void WITextEntry::Initialize()
{
	WIBase::Initialize();

	SetMouseInputEnabled(true);
	SetKeyboardInputEnabled(true);
	SetHeight(24);
	Vector2i size = GetSize();
	m_hBg = CreateChild<WIRect>();
	WIRect *pBg = m_hBg.get<WIRect>();
	pBg->SetName("background");
	pBg->SetColor(1,1,1,1);
	pBg->SetSize(size.x,size.y);
	pBg->SetAnchor(0,0,1,1);

	m_hBase = CreateChild<WITextEntryBase>();
	WITextEntryBase *pBase = m_hBase.get<WITextEntryBase>();
	pBase->SetSize(size.x,size.y);
	pBase->AddCallback("OnTextEntered",FunctionCallback<>::Create(std::bind([](WIHandle hTextEntry) {
		if(!hTextEntry.IsValid())
			return;
		WITextEntry *te = hTextEntry.get<WITextEntry>();
		te->OnTextEntered();
	},this->GetHandle())));
	pBase->AddCallback("OnTextChanged",FunctionCallback<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>::Create(std::bind([](WIHandle hTextEntry,std::reference_wrapper<const std::string> oldText,std::reference_wrapper<const std::string> text) {
		if(!hTextEntry.IsValid())
			return;
		WITextEntry *te = hTextEntry.get<WITextEntry>();
		te->OnTextChanged(oldText,text);
	},this->GetHandle(),std::placeholders::_1,std::placeholders::_2)));
	pBase->AddCallback("OnFocusGained",FunctionCallback<>::Create(std::bind([](WIHandle hTextEntry) {
		if(!hTextEntry.IsValid())
			return;
		WITextEntry *te = hTextEntry.get<WITextEntry>();
		te->OnFocusGained();
	},this->GetHandle())));
	pBase->AddCallback("OnFocusKilled",FunctionCallback<>::Create(std::bind([](WIHandle hTextEntry) {
		if(!hTextEntry.IsValid())
			return;
		WITextEntry *te = hTextEntry.get<WITextEntry>();
		te->OnFocusKilled();
	},this->GetHandle())));
	pBase->SetMouseInputEnabled(GetMouseInputEnabled());
	pBase->SetKeyboardInputEnabled(GetKeyboardInputEnabled());

	m_hOutline = CreateChild<WIOutlinedRect>();
	WIOutlinedRect *pORect = m_hOutline.get<WIOutlinedRect>();
	pORect->SetName("background_outline");
	pORect->SetOutlineWidth(1);
	pORect->WIBase::SetSize(size.x,size.y);
	pORect->SetZPos(10);
	pORect->SetColor(0,0,0,1);
}

void WITextEntry::OnTextEntered()
{
	CallCallbacks<void>("OnTextEntered");
}

void WITextEntry::OnTextChanged(const std::string &oldText,const std::string &text)
{
	if(m_hBase.IsValid())
	{
		auto *pBase = static_cast<WITextEntryBase*>(m_hBase.get());
		auto *pText = pBase->GetTextElement();
		if(pText && pText->ShouldAutoSizeToText())
		{
			//pBase->SizeToContents();
			if(pText->GetAutoBreakMode() != WIText::AutoBreak::NONE)
				SetHeight(pBase->GetHeight());
		}
	}
	CallCallbacks<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged",oldText,text);
}

void WITextEntry::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	if(m_hBase.IsValid())
	{
		auto *pBase = m_hBase.get<WITextEntryBase>();
		auto *pText = pBase->GetTextElement();
		if(IsMultiLine() == false)
		{
			auto yBase = y -10;
			if(pText != nullptr)
			{
				auto *font = pText->GetFont();
				if(font != nullptr)
					yBase = umath::max(static_cast<uint32_t>(yBase),font->GetSize());
			}
			pBase->SetSize(x -10,yBase);
			pBase->SetPos(5,(y -yBase) /2);
		}
		else
		{
			pBase->SetSize(x,y);
			pBase->SetPos(0,0);
		}
	}
	if(m_hOutline.IsValid())
	{
		WIOutlinedRect *pRect = m_hOutline.get<WIOutlinedRect>();
		pRect->SetSize(x,y);
	}
	if(m_hBg.IsValid())
	{
		WIRect *pBg = m_hBg.get<WIRect>();
		pBg->SetSize(x,y);
	}
}

void WITextEntry::SetMaxLength(int length)
{
	if(!m_hBase.IsValid())
		return;
	m_hBase.get<WITextEntryBase>()->SetMaxLength(length);
}
int WITextEntry::GetMaxLength() const
{
	if(!m_hBase.IsValid())
		return -1;
	return m_hBase.get<WITextEntryBase>()->GetMaxLength();
}

std::string WITextEntry::GetText() const
{
	if(!m_hBase.IsValid())
		return "";
	return m_hBase.get<WITextEntryBase>()->GetText();
}
void WITextEntry::SetText(std::string text)
{
	if(!m_hBase.IsValid())
		return;
	m_hBase.get<WITextEntryBase>()->SetText(text);
}
void WITextEntry::InsertText(std::string instext,int pos)
{
	if(!m_hBase.IsValid())
		return;
	m_hBase.get<WITextEntryBase>()->InsertText(instext,pos);
}
void WITextEntry::InsertText(std::string text)
{
	if(!m_hBase.IsValid())
		return;
	m_hBase.get<WITextEntryBase>()->InsertText(text);
}
int WITextEntry::GetCaretPos() const
{
	if(!m_hBase.IsValid())
		return 0;
	return m_hBase.get<WITextEntryBase>()->GetCaretPos();
}
void WITextEntry::SetCaretPos(int pos)
{
	if(!m_hBase.IsValid())
		return;
	m_hBase.get<WITextEntryBase>()->SetCaretPos(pos);
}
WIRect *WITextEntry::GetCaretElement()
{
	return m_hBase.IsValid() ? static_cast<WITextEntryBase*>(m_hBase.get())->GetCaretElement() : nullptr;
}
bool WITextEntry::IsNumeric() const {return false;}
bool WITextEntry::IsMultiLine() const
{
	if(!m_hBase.IsValid())
		return false;
	return m_hBase.get<WITextEntryBase>()->IsMultiLine();
}
void WITextEntry::SetMultiLine(bool bMultiLine)
{
	if(!m_hBase.IsValid())
		return;
	m_hBase.get<WITextEntryBase>()->SetMultiLine(bMultiLine);
}
bool WITextEntry::IsEditable() const
{
	if(!m_hBase.IsValid())
		return false;
	return m_hBase.get<WITextEntryBase>()->IsEditable();
}
void WITextEntry::SetEditable(bool bEditable)
{
	if(!m_hBase.IsValid())
		return;
	return m_hBase.get<WITextEntryBase>()->SetEditable(bEditable);
}
bool WITextEntry::IsSelectable() const
{
	if(!m_hBase.IsValid())
		return false;
	return m_hBase.get<WITextEntryBase>()->IsSelectable();
}
void WITextEntry::SetSelectable(bool bSelectable)
{
	if(!m_hBase.IsValid())
		return;
	return m_hBase.get<WITextEntryBase>()->SetSelectable(bSelectable);
}
void WITextEntry::GetSelectionBounds(int *start,int *end) const
{
	if(!m_hBase.IsValid())
	{
		*start = 0;
		*end = 0;
		return;
	}
	return m_hBase.get<WITextEntryBase>()->GetSelectionBounds(start,end);
}
void WITextEntry::SetSelectionBounds(int start,int end)
{
	if(!m_hBase.IsValid())
		return;
	return m_hBase.get<WITextEntryBase>()->SetSelectionBounds(start,end);
}
void WITextEntry::ClearSelection()
{
	if(!m_hBase.IsValid())
		return;
	return m_hBase.get<WITextEntryBase>()->ClearSelection();
}
void WITextEntry::RemoveSelection()
{
	if(!m_hBase.IsValid())
		return;
	return m_hBase.get<WITextEntryBase>()->RemoveSelection();
}

///////////////////////////////////

void WINumericEntry::Initialize()
{
	WITextEntry::Initialize();

	m_numeric.min = nullptr;
	m_numeric.max = nullptr;
	if(!m_numeric.hUpArrow.IsValid())
	{
		m_numeric.hUpArrow = CreateChild<WIArrow>();
		m_numeric.hUpArrow->SetSize(6,6);
		m_numeric.hUpArrow->SetMouseInputEnabled(true);
		auto hThis = GetHandle();
		m_numeric.hUpArrow->AddCallback("OnMousePressed",FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this,hThis](util::EventReply *reply) -> CallbackReturnType {
			*reply = util::EventReply::Handled;
			if(!hThis.IsValid())
				return CallbackReturnType::HasReturnValue;
			auto text = GetText();
			auto i = atoi(text.c_str());
			if(m_numeric.max != nullptr)
				i = umath::min(++i,*m_numeric.max);
			SetText(ustring::int_to_string(i));
			return CallbackReturnType::HasReturnValue;
		}));
		static_cast<WIArrow*>(m_numeric.hUpArrow.get())->SetDirection(WIArrow::Direction::Up);
	}
	if(!m_numeric.hDownArrow.IsValid())
	{
		m_numeric.hDownArrow = CreateChild<WIArrow>();
		m_numeric.hDownArrow->SetSize(6,6);
		auto hThis = GetHandle();
		m_numeric.hDownArrow->AddCallback("OnMousePressed",FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this,hThis](util::EventReply *reply) -> CallbackReturnType {
			*reply = util::EventReply::Handled;
			if(!hThis.IsValid())
				return CallbackReturnType::HasReturnValue;
			auto text = GetText();
			auto i = atoi(text.c_str());
			if(m_numeric.min != nullptr)
				i = umath::max(--i,*m_numeric.min);
			SetText(ustring::int_to_string(i));
			return CallbackReturnType::HasReturnValue;
		}));
	}
	UpdateArrowPositions();
}

bool WINumericEntry::IsNumeric() const {return true;}

const int32_t *WINumericEntry::GetMinValue() const {return m_numeric.min.get();}
const int32_t *WINumericEntry::GetMaxValue() const {return m_numeric.max.get();}

void WINumericEntry::SetRange(int32_t min,int32_t max)
{
	SetMinValue(min);
	SetMaxValue(max);
}

void WINumericEntry::SetMinValue(int32_t min)
{
	m_numeric.min = std::make_unique<int32_t>(min);
}
void WINumericEntry::SetMinValue()
{
	m_numeric.min = nullptr;
}
void WINumericEntry::SetMaxValue(int32_t max)
{
	m_numeric.max = std::make_unique<int32_t>(max);
}
void WINumericEntry::SetMaxValue()
{
	m_numeric.max = nullptr;
}

void WINumericEntry::UpdateArrowPositions()
{
	if(m_numeric.hDownArrow.IsValid() == false || m_numeric.hUpArrow.IsValid() == false)
		return;
	auto w = GetWidth();
	m_numeric.hUpArrow->SetPos(w -m_numeric.hUpArrow->GetWidth() -4,2);
	m_numeric.hDownArrow->SetPos(w -m_numeric.hDownArrow->GetWidth() -4,GetHeight() -m_numeric.hDownArrow->GetHeight() -2);
}

void WINumericEntry::SetSize(int x,int y)
{
	WITextEntry::SetSize(x,y);
	UpdateArrowPositions();
}
#pragma optimize("",on)
