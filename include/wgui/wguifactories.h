// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __GUIFACTORIES_H__
#define __GUIFACTORIES_H__
#include <string>
#include <unordered_map>
#include <typeinfo>
#include "wguidefinitions.h"
#undef GetClassName
class DLLWGUI WIBase;
class DLLWGUI WGUIClassMap {
  private:
	std::unordered_map<std::string, WIBase *(*)(void)> m_factories;
	std::unordered_map<size_t, std::string> m_classNames;
  public:
	void AddClass(std::string name, const std::type_info &info, WIBase *(*fc)(void));
	void GetFactories(std::unordered_map<std::string, WIBase *(*)(void)> **factories);
	bool GetClassName(const std::type_info &info, std::string *classname);
	WIBase *(*FindFactory(std::string classname))();
};

DLLWGUI void LinkWGUIToFactory(std::string name, const std::type_info &info, WIBase *(*fc)(void));
DLLWGUI WGUIClassMap *GetWGUIClassMap();
class DLLWGUI __reg_wgui_factory {
  public:
	__reg_wgui_factory(std::string name, const std::type_info &info, WIBase *(*fc)(void));
};

#define LINK_WGUI_TO_CLASS(localname, classname)                                                                                                                                                                                                                                                 \
	static WIBase *CreateWGUI##classname()                                                                                                                                                                                                                                                       \
	{                                                                                                                                                                                                                                                                                            \
		WIBase *p = WGUI::GetInstance().Create<classname>();                                                                                                                                                                                                                                     \
		return p;                                                                                                                                                                                                                                                                                \
	}                                                                                                                                                                                                                                                                                            \
	__reg_wgui_factory *__reg_wgui_factory_##classname = new __reg_wgui_factory(#localname, typeid(classname), &CreateWGUI##classname);

#endif
