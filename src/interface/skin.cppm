// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.gui:skin;

export import std.compat;

export namespace pragma::gui {
	namespace types {
		class WIBase;
	}
	class WGUI;
	class DLLWGUI WISkin {
	  protected:
		WISkin(std::string id);
		WISkin();
		std::string m_identifier;
		void ReleaseElement(types::WIBase *el);
	  public:
		friend WGUI;
	  public:
		virtual ~WISkin();
		void SetIdentifier(const std::string &identifier) { m_identifier = identifier; }
		const std::string &GetIdentifier() const { return m_identifier; }
		virtual void Initialize(types::WIBase *el);
		virtual void Release(types::WIBase *el);
		virtual void InitializeClass(types::WIBase *el, std::string &className);
	};
}
