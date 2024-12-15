/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wicontentwrapper.hpp"
LINK_WGUI_TO_CLASS(WIContentWrapper, WIContentWrapper);

WIContentWrapper::WIContentWrapper() : WIBase() {}

void WIContentWrapper::SetPadding(const WIPadding &border)
{
	m_padding = border;
	UpdateChildElement();
}
void WIContentWrapper::ClearPadding() { m_padding = {}; }
const WIPadding &WIContentWrapper::GetPadding() const { return m_padding; }
void WIContentWrapper::SetPadding(int32_t left, int32_t right, int32_t top, int32_t bottom) { SetPadding(WIPadding {left, right, top, bottom}); }
void WIContentWrapper::SetPadding(int32_t border) { SetPadding(border, border, border, border); }
void WIContentWrapper::SetPaddingLeft(int32_t border)
{
	m_padding.left = border;
	UpdateChildElement();
}
void WIContentWrapper::SetPaddingRight(int32_t border)
{
	m_padding.right = border;
	UpdateChildElement();
}
void WIContentWrapper::SetPaddingTop(int32_t border)
{
	m_padding.top = border;
	UpdateChildElement();
}
void WIContentWrapper::SetPaddingBottom(int32_t border)
{
	m_padding.bottom = border;
	UpdateChildElement();
}
void WIContentWrapper::SetPaddingLeftRight(int32_t border)
{
	m_padding.left = border;
	m_padding.right = border;
	UpdateChildElement();
}
void WIContentWrapper::SetPaddingTopBottom(int32_t border)
{
	m_padding.top = border;
	m_padding.bottom = border;
	UpdateChildElement();
}

void WIContentWrapper::SetSize(int x, int y)
{
	WIBase::SetSize(x, y);
	if(m_skipChildResize)
		return;
	UpdateChildElement();
}

void WIContentWrapper::UpdateChildElement()
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
