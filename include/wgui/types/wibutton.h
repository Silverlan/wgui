/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIBUTTON_H__
#define __WIBUTTON_H__
#include "wgui/wibase.h"
#include "wgui/wibufferbase.h"
#include "wgui/wihandle.h"

namespace util {
	class Utf8StringArg;
};
class DLLWGUI WIButton : public WIBase {
  protected:
	WIHandle m_text;
	bool m_bPressed;
  public:
	WIButton();
	virtual ~WIButton() override;
	virtual void Initialize() override;
	void SetText(const util::Utf8StringArg &text);
	const util::Utf8String &GetText() const;
	virtual void SetSize(int x, int y) override;
	virtual util::EventReply MouseCallback(GLFW::MouseButton button, GLFW::KeyState state, GLFW::Modifier mods) override;
	virtual void SizeToContents(bool x = true, bool y = true) override;
};

#endif
