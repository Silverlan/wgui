/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIBUTTON_H__
#define __WIBUTTON_H__
#include "wgui/wibase.h"
#include "wgui/wibufferbase.h"
#include "wgui/wihandle.h"

class DLLWGUI WIButton
	: public WIBase
{
protected:
	WIHandle m_text;
	bool m_bPressed;
public:
	WIButton();
	virtual ~WIButton() override;
	virtual void Initialize() override;
	void SetText(std::string text);
	std::string GetText();
	virtual void SetSize(int x,int y) override;
	virtual void MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual void SizeToContents() override;
};

#endif