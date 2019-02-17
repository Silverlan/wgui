/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIROUNDEDRECTBASE_H__
#define __WIROUNDEDRECTBASE_H__

#include "wgui/wguidefinitions.h"

class WIShape;
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WIRoundedBase
{
public:
	friend WIShape;
protected:
	WIRoundedBase();
	char m_roundness;
	float m_cornerSize;
	bool m_bRoundUpperLeft;
	bool m_bRoundUpperRight;
	bool m_bRoundLowerLeft;
	bool m_bRoundLowerRight;
	virtual void Update();
	virtual void Initialize();
public:
	virtual ~WIRoundedBase()=default;
	char GetRoundness();
	void SetRoundness(char roundness);
	void SetCornerSize(float size);
	float GetCornerSize();
	void SetRoundTopRight(bool b);
	void SetRoundTopLeft(bool b);
	void SetRoundBottomLeft(bool b);
	void SetRoundBottomRight(bool b);
	bool IsTopRightRound();
	bool IsTopLeftRound();
	bool IsBottomLeftRound();
	bool IsBottomRightRound();
};
#pragma warning(pop)

#endif