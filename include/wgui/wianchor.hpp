/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIANCHOR_HPP__
#define __WIANCHOR_HPP__

#include "wguidefinitions.h"
#include <sharedutils/property/util_property_vector.h>

class DLLWGUI WIAnchor
{
public:
	WIAnchor(WIBase &owner,const Vector2 &pos={});
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