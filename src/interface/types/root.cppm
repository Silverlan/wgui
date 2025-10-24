// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"

export module pragma.gui:types.root;

import :handle;
import :types.base;

export {
	class DLLWGUI WIRoot : public wgui::WIBase {
	public:
		WIRoot();
		virtual ~WIRoot() override;
		virtual void Initialize() override;
		virtual void OnCursorMoved(int x, int y) override;
		virtual void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd) override;
		const prosper::Window *GetWindow() const;
		prosper::Window *GetWindow();
		void Setup();

		void SetIMETarget(WIBase *el);

		Vector2 GetCursorPos() const;
		void SetCursorPosOverride(const Vector2 &pos);
		const std::optional<Vector2> &GetCursorPosOverride() const { return m_cursorPosOverride; }
		void ClearCursorPosOverride();

		const WIBase *GetFocusedElement() const { return const_cast<WIRoot *>(this)->GetFocusedElement(); }
		WIBase *GetFocusedElement();
	protected:
		friend WGUI;
		friend WIBase;

		WIBase &FindTooltipBaseElement(WIBase &el);
		void UpdateIMETarget();
		const std::weak_ptr<const prosper::Window> &GetWindowPtr() const;
		void SetWindow(const std::shared_ptr<const prosper::Window> &window);
		pragma::platform::Cursor::Shape GetMainCursor() const;
		const pragma::platform::CursorHandle &GetMainCustomCursor() const;
		void SetMainCursor(pragma::platform::Cursor::Shape cursor);
		void SetMainCustomCursor(const pragma::platform::CursorHandle &hCursor);
		void SetFocusEnabled(bool enabled);
		bool IsFocusEnabled() const;
		void SetFocusedElement(WIBase *el);
		uint32_t GetFocusCount() const;
		void SetFocusCount(uint32_t focusCount);
		std::deque<wgui::WIHandle> &GetFocusTrapStack();

		bool IsMainFileHovering() const;
		void SetMainFileHovering(bool hovering);
		void SetFileHoverElement(WIBase &el, bool hovering);
		void DropFiles(const std::vector<std::string> &files);

		void RestoreTrappedFocus(WIBase *elRef = nullptr);
	private:
		wgui::WIHandle m_hTooltip;
		wgui::WIHandle m_hTooltipTarget;
		wgui::WIHandle m_hImeTarget;
		util::Clock::time_point m_tCursorOver;
		std::weak_ptr<const prosper::Window> m_window {};
		std::optional<Vector2> m_cursorPosOverride = {};

		wgui::WIHandle m_elFocused = {};
		uint32_t m_focusCount = 0; // Used to detect changes
		std::deque<wgui::WIHandle> m_focusTrapStack;
		bool m_focusEnabled = true;

		bool m_fileDragHover = false;
		std::vector<wgui::WIHandle> m_fileHoverElements;

		pragma::platform::Cursor::Shape m_mainCursor = pragma::platform::Cursor::Shape::Arrow;
		pragma::platform::CursorHandle m_mainCustomCursor = {};
	};
};
