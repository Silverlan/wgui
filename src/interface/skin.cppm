// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <string>

export module pragma.gui:skin;

export {
	class WIBase;
	class WGUI;
	class DLLWGUI WISkin {
	protected:
		WISkin(std::string id);
		WISkin();
		std::string m_identifier;
		void ReleaseElement(WIBase *el);
	public:
		friend WGUI;
	public:
		virtual ~WISkin();
		void SetIdentifier(const std::string &identifier) { m_identifier = identifier; }
		const std::string &GetIdentifier() const { return m_identifier; }
		virtual void Initialize(WIBase *el);
		virtual void Release(WIBase *el);
		virtual void InitializeClass(WIBase *el, std::string &className);
	};
}
