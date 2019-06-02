/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext_tags.hpp"
#include "wgui/wibase.h"
#include <util_formatted_text_tag.hpp>

void WITextTagTooltip::InitializeOverlay(WIBase &overlay)
{
	overlay.SetTooltip(*static_cast<std::string*>(m_args.front().value.get()));
}
void WITextTagTooltip::Apply()
{
	WITextDecorator::Apply();
	if(m_args.empty() || m_args.front().type != WITextTagArgument::Type::String)
		return;
	CreateOverlayElements();
}
