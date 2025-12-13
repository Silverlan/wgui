// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :type_factory;

#undef GetClassName

void pragma::gui::TypeFactory::AddClass(std::string name, const std::type_info &info, Factory fc)
{
	string::to_lower(name);
	m_factories.insert(std::unordered_map<std::string, types::WIBase *(*)(void)>::value_type(name, fc));
	m_classNames.insert(std::unordered_map<size_t, std::string>::value_type(info.hash_code(), name));
}

void pragma::gui::TypeFactory::GetFactories(std::unordered_map<std::string, Factory> **factories) { *factories = &m_factories; }

bool pragma::gui::TypeFactory::GetClassName(const std::type_info &info, std::string *classname) const
{
	auto i = m_classNames.find(info.hash_code());
	if(i == m_classNames.end())
		return false;
	*classname = i->second;
	return true;
}

pragma::gui::TypeFactory::Factory pragma::gui::TypeFactory::FindFactory(std::string classname) const
{
	string::to_lower(classname);
	auto i = m_factories.find(classname);
	if(i == m_factories.end())
		return nullptr;
	return i->second;
}
