/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wicontextmenu.hpp"
#include "wgui/types/wimenuitem.hpp"

LINK_WGUI_TO_CLASS(WIContextMenu,WIContextMenu);

#pragma optimize("",off)
static WIContextMenu *s_contextMenu = nullptr;
static std::function<std::string(GLFW::Key,const std::string&)> s_fBindKey = nullptr;
static std::function<std::optional<std::string>(const std::string&)> s_fGetBoundKey = nullptr;
void WIContextMenu::SetKeyBindHandler(const std::function<std::string(GLFW::Key,const std::string&)> &fBindKey,const std::function<std::optional<std::string>(const std::string&)> &fGetBoundKey)
{
	s_fBindKey = fBindKey;
	s_fGetBoundKey = fGetBoundKey;
}
void WIContextMenu::CloseContextMenu()
{
	if(s_contextMenu == nullptr)
		return;
	s_contextMenu->RemoveSafely();
	s_contextMenu = nullptr;
}
WIContextMenu *WIContextMenu::OpenContextMenu()
{
	CloseContextMenu();
	auto *pContextMenu = WGUI::GetInstance().Create<WIContextMenu>();
	if(pContextMenu != nullptr)
	{
		pContextMenu->RequestFocus();
		int32_t x,y;
		WGUI::GetInstance().GetMousePos(x,y);
		pContextMenu->SetPos(x,y);
		pContextMenu->SetZPos(100'000);
	}
	s_contextMenu = pContextMenu;
	return pContextMenu;
}
bool WIContextMenu::IsContextMenuOpen()
{
	return s_contextMenu != nullptr;
}
WIContextMenu *WIContextMenu::GetActiveContextMenu()
{
	return s_contextMenu;
}

WIContextMenu::WIContextMenu()
	: WIRect{}
{}

void WIContextMenu::Initialize()
{
	WIRect::Initialize();

	auto &wgui = WGUI::GetInstance();
	auto *pBg = wgui.Create<WIRect>(this);
	pBg->SetAutoAlignToParent(true);
	pBg->SetColor(Color::Beige);
	SetKeyboardInputEnabled(true);
	m_hBg = pBg->GetHandle();

	auto *pBgOutline = wgui.Create<WIOutlinedRect>(this);
	pBgOutline->SetAutoAlignToParent(true);
	pBgOutline->SetColor(Color::Gray);
	m_hBgOutline = pBgOutline->GetHandle();
}
void WIContextMenu::OnRemove()
{
	WIRect::OnRemove();
}
util::EventReply WIContextMenu::KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	if(WIRect::KeyboardCallback(key,scanCode,state,mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	auto *pItem = GetSelectedItem();
	if(pItem == nullptr)
		return util::EventReply::Handled;
	auto &cmd = pItem->GetKeybindCommand();
	if(cmd.empty() || s_fBindKey == nullptr)
		return util::EventReply::Handled;
	pItem->SetRightText(s_fBindKey(key,cmd));
	return util::EventReply::Handled;
}
void WIContextMenu::Think()
{
	WIRect::Think();
	UpdateChildrenMouseInBounds();
}
void WIContextMenu::Update()
{
	WIRect::Update();
	auto yOffset = 0u;
	auto wItem = 256u;
	auto hItem = 16u;
	for(auto &item : m_items)
	{
		if(item.IsValid() == false)
			continue;
		item->SetWidth(wItem);
		item->SetHeight(hItem);
		item->SetY(yOffset);
		yOffset = yOffset +hItem;
	}
	SetSize(wItem,yOffset);
}
	
bool WIContextMenu::IsCursorInMenuBounds() const
{
	if(MouseInBounds())
		return true;
	for(auto &hSubMenu : m_subMenues)
	{
		if(hSubMenu.IsValid() == false || hSubMenu.get()->MouseInBounds() == false)
			continue;
		return true;
	}
	return false;
}
WIMenuItem *WIContextMenu::GetSelectedItem()
{
	auto itItem = std::find_if(m_items.begin(),m_items.end(),[](const WIHandle &hItem) {
		return hItem.IsValid() && static_cast<WIMenuItem*>(hItem.get())->IsSelected();
	});
	if(itItem != m_items.end())
		return static_cast<WIMenuItem*>(itItem->get());
	for(auto &hSubMenu : m_subMenues)
	{
		if(hSubMenu.IsValid() == false)
			continue;
		auto *pSelectedItem = static_cast<WIContextMenu*>(hSubMenu.get())->GetSelectedItem();
		if(pSelectedItem != nullptr)
			return pSelectedItem;
	}
	return nullptr;
}
WIMenuItem *WIContextMenu::AddItem(const std::string &name,const std::function<bool(WIMenuItem&)> &fOnClick,const std::string &keyBind)
{
	auto *pItem = WGUI::GetInstance().Create<WIMenuItem>(this);
	if(pItem == nullptr)
		return nullptr;
	pItem->SetTitle(name);
	if(keyBind.empty() == false)
	{
		pItem->SetKeybindCommand(keyBind);
		if(s_fGetBoundKey != nullptr)
		{
			auto mappedKey = s_fGetBoundKey(keyBind);
			if(mappedKey.has_value())
				pItem->SetRightText(*mappedKey);
		}
	}
	auto hThis = GetHandle();
	pItem->SetAction([this,hThis,fOnClick,pItem]() {
		if(hThis.IsValid() == false)
			return;
		if((fOnClick && fOnClick(*pItem) == false) || hThis.IsValid() == false)
			return;
		if(s_contextMenu == hThis.get())
			CloseContextMenu();
		else
			hThis.get()->RemoveSafely();
	});
	m_items.push_back(pItem->GetHandle());
	return pItem;
}
std::pair<WIContextMenu*,WIMenuItem*> WIContextMenu::AddSubMenu(const std::string &name)
{
	auto *pItem = AddItem(name,[](WIMenuItem&) {return false;});
	if(pItem == nullptr)
		return {nullptr,nullptr};
	auto *pSubMenu = WGUI::GetInstance().Create<WIContextMenu>();
	auto hSubMenu = pSubMenu->GetHandle();
	pItem->AddCallback("OnCursorEntered",FunctionCallback<void>::Create([hSubMenu,pItem]() {
		if(hSubMenu.IsValid() == false)
			return;
		auto *pSubMenu = static_cast<WIContextMenu*>(hSubMenu.get());
		pSubMenu->SetVisible(true);
		auto pos = pItem->GetAbsolutePos();
		pSubMenu->SetX(pos.x +pItem->GetWidth());
		pSubMenu->SetY(pos.y);
	}));
	pItem->AddCallback("OnCursorExited",FunctionCallback<void>::Create([hSubMenu,pItem]() {
		if(hSubMenu.IsValid() == false)
			return;
		auto *pSubMenu = static_cast<WIContextMenu*>(hSubMenu.get());
		if(pSubMenu->MouseInBounds())
		{
			pItem->KillFocus();
			pSubMenu->RequestFocus();
		}
		else
			pSubMenu->SetVisible(false);
	}));
	pSubMenu->AddCallback("OnCursorExited",FunctionCallback<void>::Create([this,hSubMenu]() {
		if(hSubMenu.IsValid() == false)
			return;
		auto *pSubMenu = static_cast<WIContextMenu*>(hSubMenu.get());
		pSubMenu->KillFocus();
		pSubMenu->SetVisible(false);
		RequestFocus();
	}));
	pSubMenu->SetVisible(false);
	pItem->RemoveOnRemoval(pSubMenu);
	m_subMenues.push_back(pSubMenu->GetHandle());
	return {pSubMenu,pItem};
}
void WIContextMenu::ClearItems()
{
	for(auto &hItem : m_items)
	{
		if(hItem.IsValid())
			hItem->Remove();
	}
	m_items.clear();
	for(auto &hSubMenu : m_subMenues)
	{
		if(hSubMenu.IsValid())
			hSubMenu->Remove();
	}
	m_subMenues.clear();
}
uint32_t WIContextMenu::GetItemCount() const {return m_items.size();}
uint32_t WIContextMenu::GetSubMenuCount() const {return m_subMenues.size();}
const std::vector<WIHandle> &WIContextMenu::GetItems() const {return m_items;}
const std::vector<WIHandle> &WIContextMenu::GetSubMenues() const {return m_subMenues;}
WIMenuItem *WIContextMenu::SelectItem(uint32_t idx)
{
	auto curIdx = GetSelectedItemIndex();
	if(curIdx.has_value())
	{
		auto &hItem = m_items.at(*curIdx);
		if(hItem.IsValid())
			static_cast<WIMenuItem*>(hItem.get())->SetSelected(false);
	}
	if(idx >= m_items.size())
		return nullptr;
	auto *pItem = static_cast<WIMenuItem*>(m_items.at(idx).get());
	pItem->SetSelected(true);
	return pItem;
}
std::optional<uint32_t> WIContextMenu::GetSelectedItemIndex() const
{
	auto itItem = std::find_if(m_items.begin(),m_items.end(),[](const WIHandle &hItem) {
		return hItem.IsValid() && static_cast<WIMenuItem*>(hItem.get())->IsSelected();
	});
	if(itItem == m_items.end())
		return {};
	return itItem -m_items.begin();
}
#pragma optimize("",on)
