/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIMENUITEM_HPP__
#define __WIMENUITEM_HPP__

#include "wgui/wibase.h"

class WIText;
class DLLWGUI WIMenuItem
	: public WIBase
{
public:
	WIMenuItem();
	virtual void Initialize() override;

	void SetRightText(const std::string &rightText);
	void SetSelected(bool bSelected);
	bool IsSelected() const;
	virtual void OnCursorEntered() override;
	virtual void OnCursorExited() override;
	virtual util::EventReply MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual void SetSize(int x,int y) override;
	using WIBase::SetSize;
	virtual void SizeToContents() override;
	void SetAction(const std::function<void(void)> &fOnClickAction);
	void SetKeybindCommand(const std::string &cmd);
	const std::string &GetKeybindCommand() const;
	void SetTitle(const std::string &title);

	WIText *GetTextElement();
private:
	void UpdateRightText();
	std::function<void(void)> m_fOnAction = nullptr;
	std::vector<WIBase*> m_items = {};
	WIHandle m_hBg = {};
	WIHandle m_hBgOutline = {};
	WIHandle m_hText = {};
	WIHandle m_hRightText = {};
	std::string m_keyBindCommand = {};
	bool m_bSelected = false;
};

#endif
