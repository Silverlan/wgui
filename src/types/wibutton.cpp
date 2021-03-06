/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wibutton.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"

LINK_WGUI_TO_CLASS(WIButton,WIButton);

WIButton::WIButton()
	: WIBase(),m_bPressed(false)
{
	SetMouseInputEnabled(true);
	RegisterCallbackWithOptionalReturn<util::EventReply>("OnPressed");
}
WIButton::~WIButton()
{}
void WIButton::Initialize()
{
	WIBase::Initialize();
	SetSize(64,28);
	m_text = CreateChild<WIText>();
}
void WIButton::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
}
void WIButton::SetText(std::string text)
{
	if(!m_text.IsValid())
		return;
	WIText *pText = m_text.get<WIText>();
	pText->SetText(text);
	pText->SizeToContents();
}
std::string WIButton::GetText()
{
	if(!m_text.IsValid())
		return "";
	return m_text.get<WIText>()->GetText();
}
util::EventReply WIButton::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto hThis = GetHandle();
	auto reply = WIBase::MouseCallback(button,state,mods);
	if(reply == util::EventReply::Handled || hThis.IsValid() == false)
		return util::EventReply::Handled;
	auto response = util::EventReply::Handled;
	if(button == GLFW::MouseButton::Left)
	{
		if(state == GLFW::KeyState::Press)
			m_bPressed = true;
		else
		{
			if(m_bPressed == true)
				CallCallbacks<util::EventReply>("OnPressed",&response);
			m_bPressed = false;
		}
	}
	return util::EventReply::Handled;
}

void WIButton::SizeToContents(bool x,bool y)
{
	if(!m_text.IsValid())
		return;
	auto *pText = m_text.get<WIText>();
	auto w = pText->GetWidth();
	auto h = pText->GetHeight();
	if(x && y)
		SetSize(w +30,h +15);
	else if(x)
		SetWidth(w +30);
	else if(y)
		SetHeight(h +15);
}