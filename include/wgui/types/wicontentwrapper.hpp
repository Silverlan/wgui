/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WICONTENT_HPP__
#define __WICONTENT_HPP__

#include "wgui/wibase.h"
#include "wgui/wibufferbase.h"
#include <cinttypes>

struct DLLWGUI WIPadding {
	int32_t left = 0;
	int32_t right = 0;
	int32_t top = 0;
	int32_t bottom = 0;
};

class DLLWGUI WIContentWrapper : public WIBase {
  public:
	WIContentWrapper();

	void SetPadding(const WIPadding &border);
	void ClearPadding();
	const WIPadding &GetPadding() const;
	void SetPadding(int32_t left, int32_t right, int32_t top, int32_t bottom);
	void SetPadding(int32_t border);
	void SetPaddingLeft(int32_t border);
	void SetPaddingRight(int32_t border);
	void SetPaddingTop(int32_t border);
	void SetPaddingBottom(int32_t border);
	void SetPaddingLeftRight(int32_t border);
	void SetPaddingTopBottom(int32_t border);

	virtual void SetSize(int x, int y) override;
  private:
	void UpdateChildElement();
	WIPadding m_padding;
	CallbackHandle m_onChildSizeChanged;
	bool m_skipCallback = false;
	bool m_skipChildResize = false;
};

#endif
