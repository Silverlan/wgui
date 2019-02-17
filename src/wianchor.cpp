/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/wianchor.hpp"

WIAnchor::WIAnchor(WIBase &owner,const Vector2 &pos)
	: m_hOwner{owner.GetHandle()},m_absPosProperty{util::Vector2iProperty::Create(pos)}
{}
void WIAnchor::UpdateAbsolutePosition()
{
	if(m_hOwner.IsValid() == false)
		return;
	auto *p = m_hOwner.get();
	auto &origin = p->GetPos();
	auto &sz = p->GetSize();
	*m_absPosProperty = origin +Vector2i(sz.x *m_relativePosition.x,sz.y *m_relativePosition.y);
}
const Vector2 &WIAnchor::GetRelativePosition() const {return m_relativePosition;}
void WIAnchor::SetRelativePosition(const Vector2 &pos)
{
	m_relativePosition = pos;
	UpdateAbsolutePosition();
}
const util::PVector2iProperty &WIAnchor::GetAbsPosProperty() const {return m_absPosProperty;}
