// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :anchor;

pragma::gui::WIAttachment::WIAttachment(types::WIBase &owner, const Vector2 &pos) : m_hOwner {owner.GetHandle()}, m_absPosProperty {util::Vector2iProperty::Create(pos)} {}
void pragma::gui::WIAttachment::UpdateAbsolutePosition()
{
	if(m_hOwner.IsValid() == false)
		return;
	auto *p = m_hOwner.get();
	auto &origin = p->GetPos();
	auto &sz = p->GetSize();
	*m_absPosProperty = origin + Vector2i(sz.x * m_relativePosition.x, sz.y * m_relativePosition.y);
}
const Vector2 &pragma::gui::WIAttachment::GetRelativePosition() const { return m_relativePosition; }
void pragma::gui::WIAttachment::SetRelativePosition(const Vector2 &pos)
{
	m_relativePosition = pos;
	UpdateAbsolutePosition();
}
const pragma::util::PVector2iProperty &pragma::gui::WIAttachment::GetAbsPosProperty() const { return m_absPosProperty; }
