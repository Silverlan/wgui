// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :types.button;

import pragma.string.unicode;

pragma::gui::types::WIButton::WIButton() : WIBase(), m_bPressed(false)
{
	SetMouseInputEnabled(true);
	RegisterCallbackWithOptionalReturn<util::EventReply>("OnPressed");
}
pragma::gui::types::WIButton::~WIButton() {}
void pragma::gui::types::WIButton::Initialize()
{
	WIBase::Initialize();
	SetSize(64, 28);
	m_text = CreateChild<WIText>();
}
void pragma::gui::types::WIButton::SetSize(int x, int y) { WIBase::SetSize(x, y); }
void pragma::gui::types::WIButton::SetText(const string::Utf8StringArg &text)
{
	if(!m_text.IsValid())
		return;
	auto *pText = static_cast<WIText *>(m_text.get());
	pText->SetText(text);
	pText->SizeToContents();
}
const pragma::string::Utf8String &pragma::gui::types::WIButton::GetText() const
{
	if(!m_text.IsValid()) {
		static string::Utf8String emptyString {};
		return emptyString;
	}
	return static_cast<const WIText *>(m_text.get())->GetText();
}
pragma::util::EventReply pragma::gui::types::WIButton::MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods)
{
	auto hThis = GetHandle();
	auto reply = WIBase::MouseCallback(button, state, mods);
	if(reply == util::EventReply::Handled || hThis.IsValid() == false)
		return util::EventReply::Handled;
	auto response = util::EventReply::Handled;
	if(button == platform::MouseButton::Left) {
		if(state == platform::KeyState::Press)
			m_bPressed = true;
		else {
			if(m_bPressed == true)
				CallCallbacks<util::EventReply>("OnPressed", &response);
			m_bPressed = false;
		}
	}
	return util::EventReply::Handled;
}

void pragma::gui::types::WIButton::SizeToContents(bool x, bool y)
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
