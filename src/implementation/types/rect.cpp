// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


module pragma.gui;

import :types.rect;

WIRect::WIRect() : WIShape()
{
	auto &context = WGUI::GetInstance().GetContext();
	//auto buf = prosper::util::get_square_vertex_buffer(context.GetDevice());
	//SetBuffer(*buf);
	m_vertices = prosper::CommonBufferCache::GetSquareVertices();
	//Update(); // No update required, updating would generate new buffers
}

WIRect::~WIRect() {}

///////////////////

WIOutlinedRect::WIOutlinedRect() : WIBase(), m_lineWidth(1)
{
	for(auto &hLine : m_lines)
		hLine = WIHandle {};
}

void WIOutlinedRect::Initialize()
{
	WIBase::Initialize();
	for(unsigned int i = 0; i < 4; i++) {
		WIRect *pLine = WGUI::GetInstance().Create<WIRect>(this);
		pLine->GetColorProperty()->Link(*GetColorProperty());
		m_lines[i] = pLine->GetHandle();
	}
}
unsigned int WIOutlinedRect::GetOutlineWidth() { return m_lineWidth; }

void WIOutlinedRect::SetOutlineWidth(unsigned int width)
{
	m_lineWidth = width;
	UpdateLines();
}

void WIOutlinedRect::UpdateLines()
{
	int w, h;
	GetSize(&w, &h);
	unsigned int wLine = GetOutlineWidth();
	for(auto i = decltype(m_lines.size()) {0}; i < m_lines.size(); ++i) {
		auto &hLine = m_lines[i];
		if(!hLine.IsValid())
			continue;
		auto *pRect = static_cast<WIRect *>(hLine.get());
		switch(i) {
		case 0:
			pRect->SetPos(0, 0);
			pRect->SetSize(w - wLine, wLine);
			break;
		case 1:
			pRect->SetPos(w - wLine, 0);
			pRect->SetSize(wLine, h - wLine);
			break;
		case 2:
			pRect->SetPos(wLine, h - wLine);
			pRect->SetSize(w - wLine, wLine);
			break;
		case 3:
			pRect->SetPos(0, wLine);
			pRect->SetSize(wLine, h - wLine);
			break;
		}
	}
}

void WIOutlinedRect::SetSize(int x, int y)
{
	WIBase::SetSize(x, y);
	UpdateLines();
}

///////////////////

WIRoundedRect::WIRoundedRect() : WIShape(), WIRoundedBase() {}

void WIRoundedRect::DoUpdate()
{
	WIRoundedBase::DoUpdate();
	WIShape::DoUpdate();
}
void WIRoundedRect::Initialize()
{
	WIRoundedBase::Initialize();
	WIShape::Initialize();
}

///////////////////

WITexturedRect::WITexturedRect() : WITexturedShape()
{
	auto &context = WGUI::GetInstance().GetContext();
	//auto vertexBuffer = prosper::util::get_square_vertex_buffer(dev);
	//auto uvBuffer = prosper::util::get_square_uv_buffer(dev);
	//SetBuffer(*vertexBuffer);
	//SetUVBuffer(*uvBuffer);
	m_vertices = prosper::CommonBufferCache::GetSquareVertices();
	m_uvs = prosper::CommonBufferCache::GetSquareUvCoordinates();
	//Update(); // No update required, updating would generate new buffers
}

///////////////////

WIRoundedTexturedRect::WIRoundedTexturedRect() : WITexturedShape(), WIRoundedBase() {}

void WIRoundedTexturedRect::DoUpdate()
{
	WIRoundedBase::DoUpdate();
	WITexturedShape::DoUpdate();
}
void WIRoundedTexturedRect::Initialize()
{
	WITexturedShape::Initialize();
	WIRoundedBase::Initialize();
}
