// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.content_wrapper;

pragma::gui::types::WIContentWrapper::WIContentWrapper() : WIBase() {}

void pragma::gui::types::WIContentWrapper::SetPadding(const WIPadding &border)
{
	m_padding = border;
	UpdateChildElement();
}
void pragma::gui::types::WIContentWrapper::ClearPadding() { m_padding = {}; }
const pragma::gui::WIPadding &pragma::gui::types::WIContentWrapper::GetPadding() const { return m_padding; }
void pragma::gui::types::WIContentWrapper::SetPadding(int32_t left, int32_t right, int32_t top, int32_t bottom) { SetPadding(WIPadding {left, right, top, bottom}); }
void pragma::gui::types::WIContentWrapper::SetPadding(int32_t border) { SetPadding(border, border, border, border); }
void pragma::gui::types::WIContentWrapper::SetPaddingLeft(int32_t border)
{
	m_padding.left = border;
	UpdateChildElement();
}
void pragma::gui::types::WIContentWrapper::SetPaddingRight(int32_t border)
{
	m_padding.right = border;
	UpdateChildElement();
}
void pragma::gui::types::WIContentWrapper::SetPaddingTop(int32_t border)
{
	m_padding.top = border;
	UpdateChildElement();
}
void pragma::gui::types::WIContentWrapper::SetPaddingBottom(int32_t border)
{
	m_padding.bottom = border;
	UpdateChildElement();
}
void pragma::gui::types::WIContentWrapper::SetPaddingLeftRight(int32_t border)
{
	m_padding.left = border;
	m_padding.right = border;
	UpdateChildElement();
}
void pragma::gui::types::WIContentWrapper::SetPaddingTopBottom(int32_t border)
{
	m_padding.top = border;
	m_padding.bottom = border;
	UpdateChildElement();
}

void pragma::gui::types::WIContentWrapper::SetSize(int x, int y)
{
	WIBase::SetSize(x, y);
	if(m_skipChildResize)
		return;
	UpdateChildElement();
}

void pragma::gui::types::WIContentWrapper::UpdateChildElement()
{
	if(m_children.empty())
		return;
	auto &child = m_children.front();
	if(child.IsValid() == false)
		return;
	auto *pChild = child.get();
	pChild->SetPos(m_padding.left, m_padding.top);
	auto size = GetSize();
	m_skipCallback = true;
	pChild->SetSize(size.x - m_padding.left - m_padding.right, size.y - m_padding.top - m_padding.bottom);
	m_skipCallback = false;
	if(m_onChildSizeChanged.IsValid() == false) {
		m_onChildSizeChanged = pChild->AddCallback("SetSize", FunctionCallback<void>::Create([this, pChild]() {
			if(m_skipCallback || m_skipChildResize)
				return;
			m_skipChildResize = true;
			SetSize(pChild->GetWidth() + m_padding.left + m_padding.right, pChild->GetHeight() + m_padding.top + m_padding.bottom);
			m_skipChildResize = false;
		}));
	}
}
