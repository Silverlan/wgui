/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITOOLTIP_H__
#define __WITOOLTIP_H__

#include "wgui/wibase.h"
#ifdef __linux__
import pragma.string.unicode;
#else
namespace pragma::string {
	class Utf8String;
};
#endif
class DLLWGUI WITooltip : public WIBase {
  protected:
	WIHandle m_hText;
  public:
	WITooltip();
	virtual void Initialize() override;
	void SetText(const std::string &text);
	const pragma::string::Utf8String &GetText() const;
};

#endif
