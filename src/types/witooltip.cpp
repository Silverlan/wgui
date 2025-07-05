// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "stdafx_wgui.h"
#include "wgui/types/witooltip.h"
#include "wgui/types/wirect.h"
#include "wgui/types/witext.h"

import pragma.string.unicode;

LINK_WGUI_TO_CLASS(WITooltip, WITooltip);

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
const pragma::string::Utf8String &WITooltip::GetText() const
{
	if(!m_hText.IsValid()) {
		static pragma::string::Utf8String r;
		return r;
	}
	return static_cast<const WIText *>(m_hText.get())->GetText();
}
