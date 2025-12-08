// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :text_tags;

void pragma::gui::WITextTagTooltip::InitializeOverlay(types::WIBase &overlay) { overlay.SetTooltip(*static_cast<std::string *>(m_args.front().value.get())); }
void pragma::gui::WITextTagTooltip::Apply()
{
	WITextDecorator::Apply();
	if(m_args.empty() || m_args.front().type != WITextTagArgument::Type::String)
		return;
	CreateOverlayElements();
}
