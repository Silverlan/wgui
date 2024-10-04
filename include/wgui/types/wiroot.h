/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIROOT_H__
#define __WIROOT_H__

#include "wgui/wibase.h"
#include <sharedutils/util_clock.hpp>

class DLLWGUI WIRoot : public WIBase {
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

	void UpdateIMETarget();
	const std::weak_ptr<const prosper::Window> &GetWindowPtr() const;
	void SetWindow(const std::shared_ptr<const prosper::Window> &window);
	GLFW::Cursor::Shape GetMainCursor() const;
	const GLFW::CursorHandle &GetMainCustomCursor() const;
	void SetMainCursor(GLFW::Cursor::Shape cursor);
	void SetMainCustomCursor(const GLFW::CursorHandle &hCursor);
	void SetFocusEnabled(bool enabled);
	bool IsFocusEnabled() const;
	void SetFocusedElement(WIBase *el);
	uint32_t GetFocusCount() const;
	void SetFocusCount(uint32_t focusCount);
	std::deque<WIHandle> &GetFocusTrapStack();

	bool IsMainFileHovering() const;
	void SetMainFileHovering(bool hovering);
	void SetFileHoverElement(WIBase &el, bool hovering);
	void DropFiles(const std::vector<std::string> &files);

	void RestoreTrappedFocus(WIBase *elRef = nullptr);
  private:
	WIHandle m_hTooltip;
	WIHandle m_hTooltipTarget;
	WIHandle m_hImeTarget;
	util::Clock::time_point m_tCursorOver;
	std::weak_ptr<const prosper::Window> m_window {};
	std::optional<Vector2> m_cursorPosOverride = {};

	WIHandle m_elFocused = {};
	uint32_t m_focusCount = 0; // Used to detect changes
	std::deque<WIHandle> m_focusTrapStack;
	bool m_focusEnabled = true;

	bool m_fileDragHover = false;
	std::vector<WIHandle> m_fileHoverElements;

	GLFW::Cursor::Shape m_mainCursor = GLFW::Cursor::Shape::Arrow;
	GLFW::CursorHandle m_mainCustomCursor = {};
};

#endif
