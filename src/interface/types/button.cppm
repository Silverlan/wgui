// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wibase.h"
#include "wgui/wibufferbase.h"
#include "wgui/wihandle.h"

export module pragma.gui:button;

import pragma.string.unicode;

export class DLLWGUI WIButton : public WIBase {
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
