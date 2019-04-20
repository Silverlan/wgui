/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIARROW_H__
#define __WIARROW_H__

#include "wishape.h"

class DLLWGUI WIArrow
	: public WIShape
{
public:
	enum class DLLWGUI Direction
	{
		Right = 0,
		Down = 1,
		Left = 2,
		Up = 3
	};
	WIArrow();
	virtual ~WIArrow() override;
	void SetDirection(Direction dir);
	virtual void OnCursorEntered() override;
	virtual void OnCursorExited() override;
};

#endif
