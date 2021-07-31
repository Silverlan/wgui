/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witooltip.h"
#include "wgui/types/wirect.h"
#include "wgui/types/witext.h"

LINK_WGUI_TO_CLASS(WITooltip,WITooltip);

WITooltip::WITooltip()
	: WIBase()
{}

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
	auto *pText = static_cast<WIText*>(m_hText.get());
	//pText->SetWidth(maxWidth);
	pText->SetText(text);
	pText->SizeToContents();
}
const std::string &WITooltip::GetText() const
{
	if(!m_hText.IsValid())
	{
		static std::string r;
		return r;
	}
	return static_cast<const WIText*>(m_hText.get())->GetText();
}
