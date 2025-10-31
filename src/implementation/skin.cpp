// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;






module pragma.gui;

import :skin;

WISkin::WISkin(std::string id) : m_identifier(id) {}
WISkin::WISkin() {}

WISkin::~WISkin() {}

void WISkin::Initialize(WIBase *) {}
void WISkin::Release(WIBase *) {}
void WISkin::ReleaseElement(WIBase *el)
{
	std::vector<WIHandle> *children = el->GetChildren();
	for(unsigned int i = 0; i < children->size(); i++) {
		WIHandle &hChild = (*children)[i];
		if(hChild.IsValid())
			ReleaseElement(hChild.get());
	}
	Release(el);
}
void WISkin::InitializeClass(WIBase *, std::string &) {}
