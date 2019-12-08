/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wiroot.h"
#include "wgui/types/witooltip.h"

WIRoot::WIRoot()
	: WIBase(),m_hTooltipTarget{},m_tCursorOver()
{}

WIRoot::~WIRoot()
{}

void WIRoot::Initialize()
{
	WIBase::Initialize();
	SetMouseMovementCheckEnabled(true);
	EnableThinking();
}

void WIRoot::Think()
{
	WIBase::Think();
	if(!m_hTooltip.IsValid())
		return;
	if(m_hTooltipTarget.IsValid() == true)
	{
		auto *pTooltip = static_cast<WITooltip*>(m_hTooltip.get());
		if(pTooltip->IsVisible() == false)
		{
			auto t = util::Clock::now();
			auto tDelta = std::chrono::duration_cast<std::chrono::milliseconds>(t -m_tCursorOver).count();
			if(tDelta >= 500)
			{
				auto *el = m_hTooltipTarget.get();
				int32_t x,y;
				WGUI::GetInstance().GetMousePos(x,y);
				pTooltip->FadeIn(0.1f);
				pTooltip->SetVisible(true);
				pTooltip->SetText(el->GetTooltip());
				pTooltip->SetZPos(std::numeric_limits<int>::max());
				auto xMax = GetWidth() -pTooltip->GetWidth();
				auto yMax = GetHeight() -pTooltip->GetHeight();
				if(xMax >= 0 && xMax < x)
					x = xMax;
				if(yMax >= 0 && yMax < y)
					y = yMax;
				pTooltip->SetPos(x,y);
			}
		}
	}
}

void WIRoot::OnCursorMoved(int x,int y)
{
	WIBase::OnCursorMoved(x,y);
	if(!m_hTooltip.IsValid())
		return;
	auto *pTooltip = static_cast<WITooltip*>(m_hTooltip.get());
	auto *el = WGUI::GetInstance().GetGUIElement(this,x,y,[](WIBase *elChild) -> bool {return elChild->HasTooltip();});
	if(el == nullptr || el == this)
	{
		m_hTooltipTarget = {};
		pTooltip->SetVisible(false);
		pTooltip->SetAlpha(0.f);
		return;
	}
	if(el == m_hTooltipTarget.get())
		return;
	m_tCursorOver = util::Clock::now();
	if(pTooltip->IsVisible() == true)
	{
		m_tCursorOver -= std::chrono::milliseconds(500);
		pTooltip->SetVisible(false);
	}
	m_hTooltipTarget = el->GetHandle();
}

void WIRoot::Setup()
{
	auto *pTooltip = WGUI::GetInstance().Create<WITooltip>(this);
	pTooltip->SetVisible(false);
	pTooltip->SetZPos(100'000);
	m_hTooltip = pTooltip->GetHandle();
}
