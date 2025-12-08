// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :types.context_menu;

static pragma::gui::types::WIContextMenu *s_contextMenu = nullptr;
static std::function<std::string(pragma::platform::Key, const std::string &)> s_fBindKey = nullptr;
static std::function<std::optional<std::string>(const std::string &)> s_fGetBoundKey = nullptr;
void pragma::gui::types::WIContextMenu::SetKeyBindHandler(const std::function<std::string(pragma::platform::Key, const std::string &)> &fBindKey, const std::function<std::optional<std::string>(const std::string &)> &fGetBoundKey)
{
	s_fBindKey = fBindKey;
	s_fGetBoundKey = fGetBoundKey;
}
void pragma::gui::types::WIContextMenu::CloseContextMenu()
{
	if(s_contextMenu == nullptr)
		return;
	s_contextMenu->RemoveSafely();
	s_contextMenu = nullptr;
}
pragma::gui::types::WIContextMenu *pragma::gui::types::WIContextMenu::OpenContextMenu()
{
	CloseContextMenu();
	auto *pContextMenu = WGUI::GetInstance().Create<WIContextMenu>();
	if(pContextMenu != nullptr) {
		pContextMenu->RequestFocus();
		int32_t x, y;
		WGUI::GetInstance().GetMousePos(x, y);
		pContextMenu->SetPos(x, y);
		pContextMenu->SetZPos(100'000);
	}
	s_contextMenu = pContextMenu;
	return pContextMenu;
}
bool pragma::gui::types::WIContextMenu::IsContextMenuOpen() { return s_contextMenu != nullptr; }
pragma::gui::types::WIContextMenu *pragma::gui::types::WIContextMenu::GetActiveContextMenu() { return s_contextMenu; }

pragma::gui::types::WIContextMenu::WIContextMenu() : WIRect {} {}

void pragma::gui::types::WIContextMenu::Initialize()
{
	WIRect::Initialize();

	auto &wgui = WGUI::GetInstance();
	auto *pBg = wgui.Create<WIRect>(this);
	pBg->SetAutoAlignToParent(true);
	pBg->SetColor(colors::Beige);
	SetKeyboardInputEnabled(true);
	m_hBg = pBg->GetHandle();

	auto *pBgOutline = wgui.Create<WIOutlinedRect>(this);
	pBgOutline->SetAutoAlignToParent(true);
	pBgOutline->SetColor(colors::Gray);
	m_hBgOutline = pBgOutline->GetHandle();
}
void pragma::gui::types::WIContextMenu::OnRemove() { WIRect::OnRemove(); }
util::EventReply pragma::gui::types::WIContextMenu::KeyboardCallback(pragma::platform::Key key, int scanCode, pragma::platform::KeyState state, pragma::platform::Modifier mods)
{
	if(WIRect::KeyboardCallback(key, scanCode, state, mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	auto *pItem = GetSelectedItem();
	if(pItem == nullptr)
		return util::EventReply::Handled;
	auto &cmd = pItem->GetKeybindCommand();
	if(cmd.empty() || s_fBindKey == nullptr)
		return util::EventReply::Handled;
	pItem->SetRightText(s_fBindKey(key, cmd));
	return util::EventReply::Handled;
}
void pragma::gui::types::WIContextMenu::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	WIRect::Think(drawCmd);
	UpdateChildrenMouseInBounds();
}
void pragma::gui::types::WIContextMenu::DoUpdate()
{
	WIRect::DoUpdate();
	auto yOffset = 0u;
	auto wItem = 256u;
	auto hItem = 17u;
	for(auto &item : m_items) {
		if(item.IsValid() == false)
			continue;
		item->SetWidth(wItem);
		item->SetHeight(hItem);
		item->SetY(yOffset);
		yOffset = yOffset + hItem;
	}
	SetSize(wItem, yOffset);
}

bool pragma::gui::types::WIContextMenu::IsCursorInMenuBounds() const
{
	if(MouseInBounds())
		return true;
	for(auto &hSubMenu : m_subMenues) {
		if(hSubMenu.IsValid() == false || hSubMenu.get()->MouseInBounds() == false)
			continue;
		return true;
	}
	return false;
}
pragma::gui::types::WIMenuItem *pragma::gui::types::WIContextMenu::GetSelectedItem()
{
	auto itItem = std::find_if(m_items.begin(), m_items.end(), [](const WIHandle &hItem) { return hItem.IsValid() && static_cast<const WIMenuItem *>(hItem.get())->IsSelected(); });
	if(itItem != m_items.end())
		return static_cast<WIMenuItem *>(itItem->get());
	for(auto &hSubMenu : m_subMenues) {
		if(hSubMenu.IsValid() == false)
			continue;
		auto *pSelectedItem = static_cast<WIContextMenu *>(hSubMenu.get())->GetSelectedItem();
		if(pSelectedItem != nullptr)
			return pSelectedItem;
	}
	return nullptr;
}
pragma::gui::types::WIMenuItem *pragma::gui::types::WIContextMenu::AddItem(const std::string &name, const std::function<bool(WIMenuItem &)> &fOnClick, const std::string &keyBind)
{
	auto *pItem = WGUI::GetInstance().Create<WIMenuItem>(this);
	if(pItem == nullptr)
		return nullptr;
	pItem->SetTitle(name);
	if(keyBind.empty() == false) {
		pItem->SetKeybindCommand(keyBind);
		if(s_fGetBoundKey != nullptr) {
			auto mappedKey = s_fGetBoundKey(keyBind);
			if(mappedKey.has_value())
				pItem->SetRightText(*mappedKey);
		}
	}
	auto hThis = GetHandle();
	pItem->SetAction([this, hThis, fOnClick, pItem]() {
		if(hThis.IsValid() == false)
			return;
		if((fOnClick && fOnClick(*pItem) == false) || hThis.IsValid() == false)
			return;
		if(s_contextMenu == hThis.get())
			CloseContextMenu();
		else
			const_cast<WIBase *>(hThis.get())->RemoveSafely();
	});
	m_items.push_back(pItem->GetHandle());
	return pItem;
}
std::pair<pragma::gui::types::WIContextMenu *, pragma::gui::types::WIMenuItem *> pragma::gui::types::WIContextMenu::AddSubMenu(const std::string &name)
{
	auto *pItem = AddItem(name, [](WIMenuItem &) { return false; });
	if(pItem == nullptr)
		return {nullptr, nullptr};
	auto *pSubMenu = WGUI::GetInstance().Create<WIContextMenu>();
	auto hSubMenu = pSubMenu->GetHandle();
	pItem->AddCallback("OnCursorEntered", FunctionCallback<void>::Create([hSubMenu, pItem]() mutable {
		if(hSubMenu.IsValid() == false)
			return;
		auto *pSubMenu = static_cast<WIContextMenu *>(hSubMenu.get());
		pSubMenu->SetVisible(true);
		auto pos = pItem->GetAbsolutePos();
		pSubMenu->SetX(pos.x + pItem->GetWidth());
		pSubMenu->SetY(pos.y);
	}));
	pItem->AddCallback("OnCursorExited", FunctionCallback<void>::Create([hSubMenu, pItem]() mutable {
		if(hSubMenu.IsValid() == false)
			return;
		auto *pSubMenu = static_cast<WIContextMenu *>(hSubMenu.get());
		if(pSubMenu->MouseInBounds()) {
			pItem->KillFocus();
			pSubMenu->RequestFocus();
		}
		else
			pSubMenu->SetVisible(false);
	}));
	pSubMenu->AddCallback("OnCursorExited", FunctionCallback<void>::Create([this, hSubMenu]() mutable {
		if(hSubMenu.IsValid() == false)
			return;
		auto *pSubMenu = static_cast<WIContextMenu *>(hSubMenu.get());
		pSubMenu->KillFocus();
		pSubMenu->SetVisible(false);
		RequestFocus();
	}));
	pSubMenu->SetVisible(false);
	pItem->RemoveOnRemoval(pSubMenu);
	m_subMenues.push_back(pSubMenu->GetHandle());
	return {pSubMenu, pItem};
}
void pragma::gui::types::WIContextMenu::ClearItems()
{
	for(auto &hItem : m_items) {
		if(hItem.IsValid())
			hItem->Remove();
	}
	m_items.clear();
	for(auto &hSubMenu : m_subMenues) {
		if(hSubMenu.IsValid())
			hSubMenu->Remove();
	}
	m_subMenues.clear();
}
uint32_t pragma::gui::types::WIContextMenu::GetItemCount() const { return m_items.size(); }
uint32_t pragma::gui::types::WIContextMenu::GetSubMenuCount() const { return m_subMenues.size(); }
const std::vector<pragma::gui::WIHandle> &pragma::gui::types::WIContextMenu::GetItems() const { return m_items; }
const std::vector<pragma::gui::WIHandle> &pragma::gui::types::WIContextMenu::GetSubMenues() const { return m_subMenues; }
pragma::gui::types::WIMenuItem *pragma::gui::types::WIContextMenu::SelectItem(uint32_t idx)
{
	auto curIdx = GetSelectedItemIndex();
	if(curIdx.has_value()) {
		auto &hItem = m_items.at(*curIdx);
		if(hItem.IsValid())
			static_cast<WIMenuItem *>(hItem.get())->SetSelected(false);
	}
	if(idx >= m_items.size())
		return nullptr;
	auto *pItem = static_cast<WIMenuItem *>(m_items.at(idx).get());
	pItem->SetSelected(true);
	return pItem;
}
std::optional<uint32_t> pragma::gui::types::WIContextMenu::GetSelectedItemIndex() const
{
	auto itItem = std::find_if(m_items.begin(), m_items.end(), [](const WIHandle &hItem) { return hItem.IsValid() && static_cast<const WIMenuItem *>(hItem.get())->IsSelected(); });
	if(itItem == m_items.end())
		return {};
	return itItem - m_items.begin();
}
