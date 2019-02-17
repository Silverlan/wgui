/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISKIN_H__
#define __WISKIN_H__

#include "wguidefinitions.h"
#include <string>

class WGUI;
class WIBase;
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WISkin
{
protected:
	WISkin(std::string id);
	virtual ~WISkin();
	std::string m_identifier;
	void ReleaseElement(WIBase *el);
public:
	friend WGUI;
public:
	virtual void Initialize(WIBase *el);
	virtual void Release(WIBase *el);
	virtual void InitializeClass(WIBase *el,std::string &className);
};
#pragma warning(pop)

#endif
