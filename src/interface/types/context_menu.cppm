// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.context_menu;

import :handle;
import :types.menu_item;
import :types.rect;

export namespace pragma::gui::types {
	class DLLWGUI WIContextMenu : public WIRect {
	  public:
		static void CloseContextMenu();
		static WIContextMenu *OpenContextMenu();
		static bool IsContextMenuOpen();
		static WIContextMenu *GetActiveContextMenu();
		static void SetKeyBindHandler(const std::function<std::string(platform::Key, const std::string &)> &fBindKey, const std::function<std::optional<std::string>(const std::string &)> &fGetBoundKey);

		WIContextMenu();
		virtual void Initialize() override;
		virtual void OnRemove() override;
		virtual util::EventReply KeyboardCallback(platform::Key key, int scanCode, platform::KeyState state, platform::Modifier mods) override;
		virtual void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd) override;

		bool IsCursorInMenuBounds() const;
		WIMenuItem *GetSelectedItem();
		WIMenuItem *AddItem(const std::string &name, const std::function<bool(WIMenuItem &)> &fOnClick, const std::string &keyBind = "");
		std::pair<WIContextMenu *, WIMenuItem *> AddSubMenu(const std::string &name);
		void ClearItems();

		uint32_t GetItemCount() const;
		uint32_t GetSubMenuCount() const;
		const std::vector<WIHandle> &GetItems() const;
		const std::vector<WIHandle> &GetSubMenues() const;
		WIMenuItem *SelectItem(uint32_t idx);
		std::optional<uint32_t> GetSelectedItemIndex() const;
	  private:
		virtual void DoUpdate() override;
		std::vector<WIHandle> m_items = {};
		std::vector<WIHandle> m_subMenues = {};
		WIHandle m_hBg = {};
		WIHandle m_hBgOutline = {};
	};
}
