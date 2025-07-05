// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "stdafx_wgui.h"
#include "wgui/types/witext_tags.hpp"
#include "wgui/wibase.h"
#include <util_formatted_text_tag.hpp>

void WITextTagTooltip::InitializeOverlay(WIBase &overlay) { overlay.SetTooltip(*static_cast<std::string *>(m_args.front().value.get())); }
void WITextTagTooltip::Apply()
{
	WITextDecorator::Apply();
	if(m_args.empty() || m_args.front().type != WITextTagArgument::Type::String)
		return;
	CreateOverlayElements();
}
