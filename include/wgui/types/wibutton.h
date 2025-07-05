// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WIBUTTON_H__
#define __WIBUTTON_H__
#include "wgui/wibase.h"
#include "wgui/wibufferbase.h"
#include "wgui/wihandle.h"

#ifdef _MSC_VER
namespace pragma::string {
	class Utf8StringArg;
};
#else
import pragma.string.unicode;
#endif

class DLLWGUI WIButton : public WIBase {
  protected:
	WIHandle m_text;
	bool m_bPressed;
  public:
	WIButton();
	virtual ~WIButton() override;
	virtual void Initialize() override;
	void SetText(const pragma::string::Utf8StringArg &text);
	const pragma::string::Utf8String &GetText() const;
	virtual void SetSize(int x, int y) override;
	virtual util::EventReply MouseCallback(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods) override;
	virtual void SizeToContents(bool x = true, bool y = true) override;
};

#endif
