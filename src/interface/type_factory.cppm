// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.gui:type_factory;

export import std.compat;

export {
	class WIBase;
	namespace wgui {
		class DLLWGUI TypeFactory {
		  public:
			using Factory = WIBase *(*)(void);
			void AddClass(std::string name, const std::type_info &info, Factory fc);
			void GetFactories(std::unordered_map<std::string, Factory> **factories);
			bool GetClassName(const std::type_info &info, std::string *classname) const;
			Factory FindFactory(std::string classname) const;
		  private:
			std::unordered_map<std::string, Factory> m_factories;
			std::unordered_map<size_t, std::string> m_classNames;
		};
	};
};
