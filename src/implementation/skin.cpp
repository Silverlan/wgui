// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :skin;

pragma::gui::WISkin::WISkin(std::string id) : m_identifier(id) {}
pragma::gui::WISkin::WISkin() {}

pragma::gui::WISkin::~WISkin() {}

void pragma::gui::WISkin::Initialize(types::WIBase *) {}
void pragma::gui::WISkin::Release(types::WIBase *) {}
void pragma::gui::WISkin::ReleaseElement(types::WIBase *el)
{
	std::vector<WIHandle> *children = el->GetChildren();
	for(unsigned int i = 0; i < children->size(); i++) {
		WIHandle &hChild = (*children)[i];
		if(hChild.IsValid())
			ReleaseElement(hChild.get());
	}
	Release(el);
}
void pragma::gui::WISkin::InitializeClass(types::WIBase *, std::string &) {}
