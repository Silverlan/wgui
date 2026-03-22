// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.button;

import :handle;
import :types.base;

import pragma.string.unicode;

export namespace pragma::gui::types {
	class DLLWGUI WIButton : public WIBase {
	  public:
		WIButton();
		virtual ~WIButton() override;
		virtual void Initialize() override;
		void SetText(const string::Utf8StringArg &text);
		const string::Utf8String &GetText() const;
		virtual util::EventReply MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods) override;
		virtual void SizeToContents(bool x = true, bool y = true, ChangeSource changeSource = ChangeSource::User) override;
	  protected:
		WIHandle m_text;
		bool m_bPressed;
	};
}
