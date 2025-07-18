// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WIRECT_H__
#define __WIRECT_H__
#include "wishape.h"
#include "wiline.h"
#include "wiroundedbase.h"
#include "wgui/wihandle.h"
#include <array>

class DLLWGUI WIRect : public WIShape {
  public:
	WIRect();
};

class DLLWGUI WIOutlinedRect : public WIBase {
  private:
	std::array<WIHandle, 4> m_lines;
	unsigned int m_lineWidth;
	void UpdateLines();
  public:
	WIOutlinedRect();
	virtual void Initialize() override;
	unsigned int GetOutlineWidth();
	void SetOutlineWidth(unsigned int width);
	virtual void SetSize(int x, int y) override;
	using WIBase::SetColor;
};

class DLLWGUI WIRoundedRect : public WIShape, public WIRoundedBase {
  public:
	WIRoundedRect();
	virtual ~WIRoundedRect() override = default;
	virtual void DoUpdate() override;
	virtual void Initialize() override;
};

class DLLWGUI WITexturedRect : public WITexturedShape {
  public:
	WITexturedRect();
	virtual ~WITexturedRect() override = default;
};

class DLLWGUI WIRoundedTexturedRect : public WITexturedShape, public WIRoundedBase {
  public:
	WIRoundedTexturedRect();
	virtual ~WIRoundedTexturedRect() override = default;
	virtual void DoUpdate() override;
	virtual void Initialize() override;
};

#endif
