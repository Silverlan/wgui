// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :anchor;

static pragma::gui::Anchor::StateFlags edge_to_availability_flag(pragma::gui::Anchor::Edge edge)
{
	switch(edge) {
	case pragma::gui::Anchor::Edge::Left:
		return pragma::gui::Anchor::StateFlags::LeftEdgeEnabled;
	case pragma::gui::Anchor::Edge::Right:
		return pragma::gui::Anchor::StateFlags::RightEdgeEnabled;
	case pragma::gui::Anchor::Edge::Top:
		return pragma::gui::Anchor::StateFlags::TopEdgeEnabled;
	case pragma::gui::Anchor::Edge::Bottom:
		return pragma::gui::Anchor::StateFlags::BottomEdgeEnabled;
	}
	std::unreachable();
	return {};
}
bool pragma::gui::Anchor::IsEdgeEnabled(Edge edge) const { return math::is_flag_set(stateFlags, edge_to_availability_flag(edge)); }
void pragma::gui::Anchor::SetEdgeEnabled(Edge edge, bool enabled) { math::set_flag(stateFlags, edge_to_availability_flag(edge), enabled); }

bool pragma::gui::Anchor::IsInitialized() const { return math::is_flag_set(stateFlags, StateFlags::Initialized); }
void pragma::gui::Anchor::SetInitialized(bool initialized) { math::set_flag(stateFlags, StateFlags::Initialized, initialized); }

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
