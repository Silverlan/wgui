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
	RegisterCallback<void>("OnPressed");
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
void WIButton::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	WIBase::MouseCallback(button,state,mods);
	if(button == GLFW::MouseButton::Left)
	{
		if(state == GLFW::KeyState::Press)
			m_bPressed = true;
		else
		{
			if(m_bPressed == true)
				CallCallbacks<void>("OnPressed");
			m_bPressed = false;
		}
	}
}

void WIButton::SizeToContents()
{
	if(!m_text.IsValid())
		return;
	auto *pText = m_text.get<WIText>();
	auto w = pText->GetWidth();
	auto h = pText->GetHeight();
	SetSize(w +30,h +15);
}