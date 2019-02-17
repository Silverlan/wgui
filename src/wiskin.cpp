/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/wiskin.h"
#include "wgui/wgui.h"
#include "wgui/wibase.h"

WISkin::WISkin(std::string id)
	: m_identifier(id)
{}

WISkin::~WISkin()
{}

void WISkin::Initialize(WIBase*) {}
void WISkin::Release(WIBase*) {}
void WISkin::ReleaseElement(WIBase *el)
{
	std::vector<WIHandle> *children = el->GetChildren();
	for(unsigned int i=0;i<children->size();i++)
	{
		WIHandle &hChild = (*children)[i];
		if(hChild.IsValid())
			ReleaseElement(hChild.get());
	}
	Release(el);
}
void WISkin::InitializeClass(WIBase*,std::string&) {}