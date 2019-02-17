/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITOOLTIP_H__
#define __WITOOLTIP_H__

#include "wgui/wibase.h"

#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WITooltip
	: public WIBase
{
protected:
	WIHandle m_hText;
public:
	WITooltip();
	virtual void Initialize() override;
	void SetText(const std::string &text);
	const std::string &GetText() const;
};
#pragma warning(pop)

#endif
