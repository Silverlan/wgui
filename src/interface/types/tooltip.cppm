// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.tooltip;

import :handle;
import :types.base;
import :types.text;
import pragma.string.unicode;

export namespace pragma::gui::types {
	class DLLWGUI WITooltip : public WIBase {
	  protected:
		WIHandle m_hText;
	  public:
		WITooltip();
		virtual void Initialize() override;
		void SetText(const std::string &text);
		const string::Utf8String &GetText() const;
	};

	WITooltip::WITooltip() : WIBase() {}

	void WITooltip::Initialize()
	{
		WIBase::Initialize();

		auto *pText = WGUI::GetInstance().Create<WIText>(this);
		//pText->SetAutoBreakMode(WIText::AutoBreak::WHITESPACE);
		m_hText = pText->GetHandle();
	}

	void WITooltip::SetText(const std::string &text)
	{
		if(!m_hText.IsValid())
			return;
		//auto *parent = GetParent();
		//auto maxWidth = parent->GetWidth() -GetX();
		auto *pText = static_cast<WIText *>(m_hText.get());
		//pText->SetWidth(maxWidth);
		pText->SetText(text);
		pText->SizeToContents();
	}
	const string::Utf8String &WITooltip::GetText() const
	{
		if(!m_hText.IsValid()) {
			static string::Utf8String r;
			return r;
		}
		return static_cast<const WIText *>(m_hText.get())->GetText();
	}
}
