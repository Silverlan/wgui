/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/wguifactories.h"
#include "wgui/wibase.h"
#include <algorithm>

WGUIClassMap *g_wguiFactories = nullptr;
void LinkWGUIToFactory(std::string name,const std::type_info &info,WIBase*(*fc)(void))
{
	if(g_wguiFactories == nullptr)
	{
		static WGUIClassMap map;
		g_wguiFactories = &map;
	}
	g_wguiFactories->AddClass(name,info,fc);
}
WGUIClassMap *GetWGUIClassMap() {return g_wguiFactories;}

__reg_wgui_factory::__reg_wgui_factory(std::string name,const std::type_info &info,WIBase*(*fc)(void))
{
	LinkWGUIToFactory(name,info,fc);
	delete this;
}

/////////////////////////////////////////

void WGUIClassMap::AddClass(std::string name,const std::type_info &info,WIBase *(*fc)(void))
{
	ustring::to_lower(name);
	m_factories.insert(std::unordered_map<std::string,WIBase*(*)(void)>::value_type(name,fc));
	m_classNames.insert(std::unordered_map<size_t,std::string>::value_type(info.hash_code(),name));
}

void WGUIClassMap::GetFactories(std::unordered_map<std::string,WIBase*(*)(void)> **factories) {*factories = &m_factories;}
#undef GetClassName
bool WGUIClassMap::GetClassName(const std::type_info &info,std::string *classname)
{
	std::unordered_map<size_t,std::string>::iterator i = m_classNames.find(info.hash_code());
	if(i == m_classNames.end())
		return false;
	*classname = i->second;
	return true;
}

WIBase *(*WGUIClassMap::FindFactory(std::string classname))()
{
	ustring::to_lower(classname);
	std::unordered_map<std::string,WIBase*(*)()>::iterator i = m_factories.find(classname);
	if(i == m_factories.end())
		return nullptr;
	return i->second;
}
