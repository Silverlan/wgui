// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WIATTACHMENT_HPP__
#define __WIATTACHMENT_HPP__

#include "wguidefinitions.h"
#include "types.hpp"
#include <sharedutils/property/util_property_vector.h>

class DLLWGUI WIAttachment {
  public:
	WIAttachment(WIBase &owner, const Vector2 &pos = {});
	void SetRelativePosition(const Vector2 &pos);
	const Vector2 &GetRelativePosition() const;

	void UpdateAbsolutePosition();
	const util::PVector2iProperty &GetAbsPosProperty() const;
  private:
	WIHandle m_hOwner = {};
	Vector2 m_relativePosition = {};
	util::PVector2iProperty m_absPosProperty = nullptr;
};

#endif
