// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WIARROW_H__
#define __WIARROW_H__

#include "wishape.h"

class DLLWGUI WIArrow : public WIShape {
  public:
	enum class DLLWGUI Direction { Right = 0, Down = 1, Left = 2, Up = 3 };
	WIArrow();
	virtual ~WIArrow() override;
	void SetDirection(Direction dir);
	virtual void OnCursorEntered() override;
	virtual void OnCursorExited() override;
};

#endif
