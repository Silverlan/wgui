/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WICONTEXTMENU_HPP__
#define __WICONTEXTMENU_HPP__

#include "wirect.h"
#include <optional>

class WIMenuItem;
class DLLWGUI WIContextMenu
	: public WIRect
{
public:
	static void CloseContextMenu();
	static WIContextMenu *OpenContextMenu();
	static bool IsContextMenuOpen();
	static WIContextMenu *GetActiveContextMenu();
	static void SetKeyBindHandler(const std::function<std::string(GLFW::Key,const std::string&)> &fBindKey,const std::function<std::optional<std::string>(const std::string&)> &fGetBoundKey);

	WIContextMenu();
	virtual void Initialize() override;
	virtual void OnRemove() override;
	virtual util::EventReply KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual void Update() override;
	virtual void Think() override;
	
	bool IsCursorInMenuBounds() const;
	WIMenuItem *GetSelectedItem();
	WIMenuItem *AddItem(const std::string &name,const std::function<bool(WIMenuItem&)> &fOnClick,const std::string &keyBind="");
	std::pair<WIContextMenu*,WIMenuItem*> AddSubMenu(const std::string &name);
	void ClearItems();
	
	uint32_t GetItemCount() const;
	uint32_t GetSubMenuCount() const;
	const std::vector<WIHandle> &GetItems() const;
	const std::vector<WIHandle> &GetSubMenues() const;
	WIMenuItem *SelectItem(uint32_t idx);
	std::optional<uint32_t> GetSelectedItemIndex() const;
private:
	std::vector<WIHandle> m_items = {};
	std::vector<WIHandle> m_subMenues = {};
	WIHandle m_hBg = {};
	WIHandle m_hBgOutline = {};
};

#endif
