// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.menu_item;

import pragma.string.unicode;

pragma::gui::types::WIMenuItem::WIMenuItem() : WIBase {} { RegisterCallback<void>("OnSelected"); }

void pragma::gui::types::WIMenuItem::Initialize()
{
	WIBase::Initialize();
	SetMouseInputEnabled(true);

	auto &wgui = WGUI::GetInstance();
	auto *pBg = wgui.Create<WIRect>(this);
	pBg->SetAutoAlignToParent(true);
	pBg->SetColor(colors::SkyBlue);
	m_hBg = pBg->GetHandle();

	auto *pBgOutline = wgui.Create<WIOutlinedRect>(this);
	pBgOutline->SetAutoAlignToParent(true);
	pBgOutline->SetColor(colors::RoyalBlue);
	m_hBgOutline = pBgOutline->GetHandle();

	auto *pText = wgui.Create<WIText>(this);
	pText->SetColor(colors::Black);
	m_hText = pText->GetHandle();

	SetSelected(false);
}
pragma::gui::types::WIText *pragma::gui::types::WIMenuItem::GetTextElement() { return static_cast<WIText *>(m_hText.get()); }
void pragma::gui::types::WIMenuItem::SetRightText(const std::string &rightText)
{
	if(rightText.empty()) {
		if(m_hRightText.IsValid())
			m_hRightText->Remove();
		return;
	}
	if(m_hRightText.IsValid() == false) {
		auto *pText = WGUI::GetInstance().Create<WIText>(this);
		pText->SetColor(colors::Black);
		m_hRightText = pText->GetHandle();
	}
	auto *pText = static_cast<WIText *>(m_hRightText.get());
	pText->SetText(rightText);
	pText->SizeToContents();
	UpdateRightText();
}
void pragma::gui::types::WIMenuItem::SetSelected(bool bSelected)
{
	m_bSelected = bSelected;
	if(m_hBg.IsValid())
		m_hBg->SetVisible(bSelected);
	if(m_hBgOutline.IsValid())
		m_hBgOutline->SetVisible(bSelected);
	if(bSelected)
		CallCallbacks<void>("OnSelected");
}
bool pragma::gui::types::WIMenuItem::IsSelected() const { return m_bSelected; }
void pragma::gui::types::WIMenuItem::OnCursorEntered()
{
	WIBase::OnCursorEntered();
	SetSelected(true);
}
void pragma::gui::types::WIMenuItem::OnCursorExited()
{
	WIBase::OnCursorExited();
	SetSelected(false);
}
pragma::util::EventReply pragma::gui::types::WIMenuItem::MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods)
{
	if(WIBase::MouseCallback(button, state, mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(button != platform::MouseButton::Left || state != platform::KeyState::Press || m_fOnAction == nullptr)
		return util::EventReply::Handled;
	m_fOnAction();
	return util::EventReply::Unhandled;
}
void pragma::gui::types::WIMenuItem::SetSize(int x, int y)
{
	WIBase::SetSize(x, y);
	UpdateRightText();
}
const auto border = 8u;
void pragma::gui::types::WIMenuItem::SizeToContents(bool x, bool y)
{
	if(m_hText.IsValid() == false)
		return;
	auto sz = m_hText->GetSize();
	sz.x += border * 2;
	if(x && y)
		SetSize(sz);
	else if(x)
		SetWidth(sz.x);
	else if(y)
		SetHeight(sz.y);
	m_hText->SetX(border);
	m_hText->SetY(GetHeight() * 0.5f - m_hText->GetHeight() * 0.5f);
}
void pragma::gui::types::WIMenuItem::SetAction(const std::function<void(void)> &fOnClickAction) { m_fOnAction = fOnClickAction; }
void pragma::gui::types::WIMenuItem::SetKeybindCommand(const std::string &cmd) { m_keyBindCommand = cmd; }
const std::string &pragma::gui::types::WIMenuItem::GetKeybindCommand() const { return m_keyBindCommand; }
void pragma::gui::types::WIMenuItem::SetTitle(const std::string &title)
{
	if(m_hText.IsValid() == false)
		return;
	auto *pText = static_cast<WIText *>(m_hText.get());
	pText->SetText(title);
	pText->SizeToContents();
}

void pragma::gui::types::WIMenuItem::UpdateRightText()
{
	if(m_hRightText.IsValid() == false)
		return;
	m_hRightText->SetX(GetWidth() - m_hRightText->GetWidth() - border);
	m_hRightText->SetY(GetHeight() * 0.5f - m_hRightText->GetHeight() * 0.5f);
}
