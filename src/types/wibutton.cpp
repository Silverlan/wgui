// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "stdafx_wgui.h"
#include "wgui/types/wibutton.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"

import pragma.string.unicode;

LINK_WGUI_TO_CLASS(WIButton, WIButton);

WIButton::WIButton() : WIBase(), m_bPressed(false)
{
	SetMouseInputEnabled(true);
	RegisterCallbackWithOptionalReturn<util::EventReply>("OnPressed");
}
WIButton::~WIButton() {}
void WIButton::Initialize()
{
	WIBase::Initialize();
	SetSize(64, 28);
	m_text = CreateChild<WIText>();
}
void WIButton::SetSize(int x, int y) { WIBase::SetSize(x, y); }
void WIButton::SetText(const pragma::string::Utf8StringArg &text)
{
	if(!m_text.IsValid())
		return;
	auto *pText = static_cast<WIText *>(m_text.get());
	pText->SetText(text);
	pText->SizeToContents();
}
const pragma::string::Utf8String &WIButton::GetText() const
{
	if(!m_text.IsValid()) {
		static pragma::string::Utf8String emptyString {};
		return emptyString;
	}
	return static_cast<const WIText *>(m_text.get())->GetText();
}
util::EventReply WIButton::MouseCallback(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods)
{
	auto hThis = GetHandle();
	auto reply = WIBase::MouseCallback(button, state, mods);
	if(reply == util::EventReply::Handled || hThis.IsValid() == false)
		return util::EventReply::Handled;
	auto response = util::EventReply::Handled;
	if(button == pragma::platform::MouseButton::Left) {
		if(state == pragma::platform::KeyState::Press)
			m_bPressed = true;
		else {
			if(m_bPressed == true)
				CallCallbacks<util::EventReply>("OnPressed", &response);
			m_bPressed = false;
		}
	}
	return util::EventReply::Handled;
}

void WIButton::SizeToContents(bool x, bool y)
{
	if(!m_text.IsValid())
		return;
	auto *pText = static_cast<WIText *>(m_text.get());
	auto w = pText->GetWidth();
	auto h = pText->GetHeight();
	if(x && y)
		SetSize(w + 30, h + 15);
	else if(x)
		SetWidth(w + 30);
	else if(y)
		SetHeight(h + 15);
}
