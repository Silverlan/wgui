/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wimenuitem.hpp"
#include "wgui/types/wirect.h"
#include "wgui/types/witext.h"

LINK_WGUI_TO_CLASS(WIMenuItem,WIMenuItem);

WIMenuItem::WIMenuItem()
	: WIBase{}
{
	RegisterCallback<void>("OnSelected");
}

void WIMenuItem::Initialize()
{
	WIBase::Initialize();
	SetMouseInputEnabled(true);

	auto &wgui = WGUI::GetInstance();
	auto *pBg = wgui.Create<WIRect>(this);
	pBg->SetAutoAlignToParent(true);
	pBg->SetColor(Color::SkyBlue);
	m_hBg = pBg->GetHandle();

	auto *pBgOutline = wgui.Create<WIOutlinedRect>(this);
	pBgOutline->SetAutoAlignToParent(true);
	pBgOutline->SetColor(Color::RoyalBlue);
	m_hBgOutline = pBgOutline->GetHandle();

	auto *pText = wgui.Create<WIText>(this);
	pText->SetColor(Color::Black);
	m_hText = pText->GetHandle();

	SetSelected(false);
}
WIText *WIMenuItem::GetTextElement() {return static_cast<WIText*>(m_hText.get());}
void WIMenuItem::SetRightText(const std::string &rightText)
{
	if(rightText.empty())
	{
		if(m_hRightText.IsValid())
			m_hRightText->Remove();
		return;
	}
	if(m_hRightText.IsValid() == false)
	{
		auto *pText = WGUI::GetInstance().Create<WIText>(this);
		pText->SetColor(Color::Black);
		m_hRightText = pText->GetHandle();
	}
	auto *pText = static_cast<WIText*>(m_hRightText.get());
	pText->SetText(rightText);
	pText->SizeToContents();
	UpdateRightText();
}
void WIMenuItem::SetSelected(bool bSelected)
{
	m_bSelected = bSelected;
	if(m_hBg.IsValid())
		m_hBg->SetVisible(bSelected);
	if(m_hBgOutline.IsValid())
		m_hBgOutline->SetVisible(bSelected);
	if(bSelected)
		CallCallbacks<void>("OnSelected");
}
bool WIMenuItem::IsSelected() const {return m_bSelected;}
void WIMenuItem::OnCursorEntered()
{
	WIBase::OnCursorEntered();
	SetSelected(true);
}
void WIMenuItem::OnCursorExited()
{
	WIBase::OnCursorExited();
	SetSelected(false);
}
util::EventReply WIMenuItem::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	if(WIBase::MouseCallback(button,state,mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(button != GLFW::MouseButton::Left || state != GLFW::KeyState::Press || m_fOnAction == nullptr)
		return util::EventReply::Handled;
	m_fOnAction();
	return util::EventReply::Unhandled;
}
void WIMenuItem::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	UpdateRightText();
}
const auto border = 8u;
void WIMenuItem::SizeToContents()
{
	if(m_hText.IsValid() == false)
		return;
	auto sz = m_hText->GetSize();
	sz.x += border *2;
	SetSize(sz);
	m_hText->SetX(border);
	m_hText->SetY(GetHeight() *0.5f -m_hText->GetHeight() *0.5f);
}
void WIMenuItem::SetAction(const std::function<void(void)> &fOnClickAction) {m_fOnAction = fOnClickAction;}
void WIMenuItem::SetKeybindCommand(const std::string &cmd) {m_keyBindCommand = cmd;}
const std::string &WIMenuItem::GetKeybindCommand() const {return m_keyBindCommand;}
void WIMenuItem::SetTitle(const std::string &title)
{
	if(m_hText.IsValid() == false)
		return;
	auto *pText = static_cast<WIText*>(m_hText.get());
	pText->SetText(title);
	pText->SizeToContents();
}

void WIMenuItem::UpdateRightText()
{
	if(m_hRightText.IsValid() == false)
		return;
	m_hRightText->SetX(GetWidth() -m_hRightText->GetWidth() -border);
	m_hRightText->SetY(GetHeight() *0.5f -m_hRightText->GetHeight() *0.5f);
}
