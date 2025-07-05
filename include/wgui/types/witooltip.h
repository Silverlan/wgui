// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

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
