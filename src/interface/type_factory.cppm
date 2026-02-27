// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:type_factory;

export import pragma.string;

export namespace pragma::gui {
	namespace types {
		class WIBase;
	}
	using TypeId = uint32_t;
	class DLLWGUI TypeFactory {
	  public:
		using Factory = types::WIBase *(*)(void);
		struct TypeInfo {
			Factory factory;
			TypeId id;
		};
		TypeId AddClass(const std::string &name, const std::type_info &info, Factory fc);
		TypeId PreRegisterClass(const std::string &name);
		void SetRegistrationCallback(const std::function<void(const std::string &, TypeId)> &callback);
		const string::CIStringMap<TypeInfo> &GetTypes() const { return m_types; }
		std::optional<std::string> FindClassName(const std::type_info &info) const;
		std::optional<std::string> FindClassName(TypeId typeId) const;
		std::optional<TypeId> FindTypeId(const std::string_view &className) const;

		Factory FindFactory(const std::string &classname) const;
		Factory FindFactory(TypeId typeId) const;
	  private:
		string::CIStringMap<TypeInfo> m_types;
		std::unordered_map<size_t, std::string> m_typeHashToClassName;
		std::vector<std::string> m_classNames;
		std::function<void(const std::string &, TypeId)> m_registrationCallback;
	};
};
