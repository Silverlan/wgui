/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIROOT_H__
#define __WIROOT_H__

#include "wgui/wibase.h"
#include <sharedutils/util_clock.hpp>

class DLLWGUI WIRoot
	: public WIBase
{
private:
	virtual ~WIRoot();
	WIHandle m_hTooltip;
	WIHandle m_hTooltipTarget;
	util::Clock::time_point m_tCursorOver;
public:
	WIRoot();
	virtual void Initialize() override;
	virtual void OnCursorMoved(int x,int y) override;
	virtual void Think() override;
	void Setup();
};

#endif
