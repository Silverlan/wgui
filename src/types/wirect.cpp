/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wirect.h"
#include "cmaterialmanager.h"
#include <prosper_context.hpp>
#include <prosper_util_square_shape.hpp>

LINK_WGUI_TO_CLASS(WIRect,WIRect);
LINK_WGUI_TO_CLASS(WIOutlinedRect,WIOutlinedRect);
LINK_WGUI_TO_CLASS(WITexturedRect,WITexturedRect);
LINK_WGUI_TO_CLASS(WIRoundedRect,WIRoundedRect);
LINK_WGUI_TO_CLASS(WIRoundedTexturedRect,WIRoundedTexturedRect);

WIRect::WIRect()
	: WIShape()
{
	auto &context = WGUI::GetInstance().GetContext();
	//auto buf = prosper::util::get_square_vertex_buffer(context.GetDevice());
	//SetBuffer(*buf);
	m_vertices = prosper::util::get_square_vertices();
	//Update(); // No update required, updating would generate new buffers
}

///////////////////

WIOutlinedRect::WIOutlinedRect()
	: WIBase(),m_lineWidth(1)
{
	for(auto &hLine : m_lines)
		hLine = {};
}

void WIOutlinedRect::Initialize()
{
	WIBase::Initialize();
	for(unsigned int i=0;i<4;i++)
	{
		WIRect *pLine = WGUI::GetInstance().Create<WIRect>(this);
		pLine->GetColorProperty()->Link(*GetColorProperty());
		m_lines[i] = pLine->GetHandle();
	}
}
unsigned int WIOutlinedRect::GetOutlineWidth() {return m_lineWidth;}

void WIOutlinedRect::SetOutlineWidth(unsigned int width)
{
	m_lineWidth = width;
	UpdateLines();
}

void WIOutlinedRect::UpdateLines()
{
	int w,h;
	GetSize(&w,&h);
	unsigned int wLine = GetOutlineWidth();
	for(auto i=decltype(m_lines.size()){0};i<m_lines.size();++i)
	{
		auto &hLine = m_lines[i];
		if(!hLine.IsValid())
			continue;
		auto *pRect = hLine.get<WIRect>();
		switch(i)
		{
		case 0:
			pRect->SetPos(0,0);
			pRect->SetSize(w -wLine,wLine);
			break;
		case 1:
			pRect->SetPos(w -wLine,0);
			pRect->SetSize(wLine,h -wLine);
			break;
		case 2:
			pRect->SetPos(wLine,h -wLine);
			pRect->SetSize(w -wLine,wLine);
			break;
		case 3:
			pRect->SetPos(0,wLine);
			pRect->SetSize(wLine,h -wLine);
			break;
		}
	}
}

void WIOutlinedRect::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	UpdateLines();
}

///////////////////

WIRoundedRect::WIRoundedRect()
	: WIShape(),WIRoundedBase()
{}

void WIRoundedRect::Update()
{
	WIRoundedBase::Update();
	WIShape::Update();
}
void WIRoundedRect::Initialize()
{
	WIRoundedBase::Initialize();
	WIShape::Initialize();
}

///////////////////

WITexturedRect::WITexturedRect()
	: WITexturedShape()
{
	auto &context = WGUI::GetInstance().GetContext();
	//auto vertexBuffer = prosper::util::get_square_vertex_buffer(dev);
	//auto uvBuffer = prosper::util::get_square_uv_buffer(dev);
	//SetBuffer(*vertexBuffer);
	//SetUVBuffer(*uvBuffer);
	m_vertices = prosper::util::get_square_vertices();
	m_uvs = prosper::util::get_square_uv_coordinates();
	//Update(); // No update required, updating would generate new buffers
}

///////////////////

WIRoundedTexturedRect::WIRoundedTexturedRect()
	: WITexturedShape(),WIRoundedBase()
{}

void WIRoundedTexturedRect::Update()
{
	WIRoundedBase::Update();
	WITexturedShape::Update();
}
void WIRoundedTexturedRect::Initialize()
{
	WITexturedShape::Initialize();
	WIRoundedBase::Initialize();
}
