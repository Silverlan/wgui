// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WICIRCLE_H__
#define __WICIRCLE_H__
#include "wibase.h"
#include "wibufferbase.h"

class DLLWGUI WICircle {
  protected:
	WILineBase();
  public:
	unsigned int GetLineWidth();
	void SetLineWidth(unsigned int width);
};

class WILine : public WIBufferBase, WILineBase {
  private:
	Vector2i m_posEnd;
  public:
	WILine();
	Vector2i *GetEndPos();
	void SetEndPos(Vector2i pos);
	void SetEndPos(int x, int y);
	virtual void Update() override;
	virtual void SizeToContents() override;
	virtual unsigned int GetVertexCount() override;
	virtual void Render(int w, int h, Mat4 &mat) override;
	virtual Mat4 GetTransformedMatrix(const Vector2i &origin, int w, int h, Mat4 mat) const override;
};

#endif
