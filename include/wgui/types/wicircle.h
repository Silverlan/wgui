/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WICIRCLE_H__
#define __WICIRCLE_H__
#include "wibase.h"
#include "wibufferbase.h"

class DLLWGUI WICircle
{
protected:
	WILineBase();
public:
	unsigned int GetLineWidth();
	void SetLineWidth(unsigned int width);
};

class WILine
	: public WIBufferBase,WILineBase
{
private:
	Vector2i m_posEnd;
public:
	WILine();
	Vector2i *GetEndPos();
	void SetEndPos(Vector2i pos);
	void SetEndPos(int x,int y);
	virtual void Update() override;
	virtual void SizeToContents() override;
	virtual unsigned int GetVertexCount() override;
	virtual void Render(int w,int h,Mat4 &mat) override;
	virtual Mat4 GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) const override;
};

#endif