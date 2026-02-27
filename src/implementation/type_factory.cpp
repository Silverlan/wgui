// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.gui;

import :type_factory;

#undef GetClassName

pragma::gui::TypeId pragma::gui::TypeFactory::PreRegisterClass(const std::string &name)
{
	auto it = m_types.find(name);
	if(it != m_types.end())
		return it->second.id;
	auto normName = name;
	string::to_lower(normName);
	TypeInfo typeInfo {};
	typeInfo.id = m_classNames.size();
	m_classNames.push_back(normName);
	m_types.insert(std::make_pair(normName, typeInfo));
	if(m_registrationCallback)
		m_registrationCallback(normName, typeInfo.id);
	return typeInfo.id;
}
pragma::gui::TypeId pragma::gui::TypeFactory::AddClass(const std::string &name, const std::type_info &info, Factory fc)
{
	auto typeId = PreRegisterClass(name);
	auto &normName = m_classNames[typeId];
	auto it = m_types.find(normName);
	assert(it != m_types.end());
	it->second.factory = fc;
	m_typeHashToClassName.insert(std::make_pair<size_t, std::string>(info.hash_code(), std::move(normName)));
	return typeId;
}

void pragma::gui::TypeFactory::SetRegistrationCallback(const std::function<void(const std::string &, TypeId)> &callback) { m_registrationCallback = callback; }

pragma::gui::TypeFactory::Factory pragma::gui::TypeFactory::FindFactory(const std::string &classname) const
{
	auto it = m_types.find(classname);
	if(it == m_types.end())
		return nullptr;
	return it->second.factory;
}
pragma::gui::TypeFactory::Factory pragma::gui::TypeFactory::FindFactory(TypeId typeId) const
{
	if(typeId >= m_classNames.size())
		return nullptr;
	auto &className = m_classNames[typeId];
	return FindFactory(className);
}

std::optional<pragma::gui::TypeId> pragma::gui::TypeFactory::FindTypeId(const std::string_view &className) const
{
	auto it = m_types.find(className);
	if(it == m_types.end())
		return {};
	return it->second.id;
}

std::optional<std::string> pragma::gui::TypeFactory::FindClassName(const std::type_info &info) const
{
	auto it = m_typeHashToClassName.find(info.hash_code());
	if(it == m_typeHashToClassName.end())
		return {};
	return it->second;
}
std::optional<std::string> pragma::gui::TypeFactory::FindClassName(TypeId typeId) const
{
	if(typeId >= m_classNames.size())
		return {};
	return m_classNames[typeId];
}
