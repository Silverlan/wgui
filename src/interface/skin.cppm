// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <string>

export module pragma.gui:skin;

export namespace wgui {
	class WIBase;
	class WGUI;
}
export class DLLWGUI WISkin {
  protected:
	WISkin(std::string id);
	WISkin();
	std::string m_identifier;
	void ReleaseElement(wgui::WIBase *el);
  public:
	friend wgui::WGUI;
  public:
	virtual ~WISkin();
	void SetIdentifier(const std::string &identifier) { m_identifier = identifier; }
	const std::string &GetIdentifier() const { return m_identifier; }
	virtual void Initialize(wgui::WIBase *el);
	virtual void Release(wgui::WIBase *el);
	virtual void InitializeClass(wgui::WIBase *el, std::string &className);
};
