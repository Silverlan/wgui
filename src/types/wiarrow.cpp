/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wiarrow.h"
#include <prosper_util.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_context.hpp>

LINK_WGUI_TO_CLASS(WIArrow,WIArrow);

static const Vector4 COLOR_SELECTED {0.1176f,0.564f,1.f,1.f};
static std::array<std::array<Vector2,3>,4> s_vertices {
	std::array<Vector2,3>{
		Vector2{-1.f,-1.f},
		Vector2{-1.f,1.f},
		Vector2{1.f,0.f}
	},
	std::array<Vector2,3>{
		Vector2{0.f,1.f},
		Vector2{1.f,-1.f},
		Vector2{-1.f,-1.f}
	},
	std::array<Vector2,3>{
		Vector2{1.f,1.f},
		Vector2{1.f,-1.f},
		Vector2{-1.f,0.f}
	},
	std::array<Vector2,3>{
		Vector2{0.f,-1.f},
		Vector2{-1.f,1.f},
		Vector2{1.f,1.f}
	}
};
static std::array<std::shared_ptr<prosper::Buffer>,4> s_vertexBuffers {nullptr,nullptr,nullptr,nullptr};

static uint32_t s_arrowCount = 0u;
WIArrow::WIArrow()
	: WIShape()
{
	if(s_arrowCount++ == 0u)
	{
		auto &context = WGUI::GetInstance().GetContext();
		auto &dev = context.GetDevice();
		for(auto i=decltype(s_vertexBuffers.size()){0};i<s_vertexBuffers.size();++i)
		{
			auto &verts = s_vertices[i];

			auto bufCreateInfo = prosper::util::BufferCreateInfo {};
			bufCreateInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT;
			bufCreateInfo.size = verts.size() *sizeof(verts.front());
			bufCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
			s_vertexBuffers[i] = prosper::util::create_buffer(dev,bufCreateInfo,verts.data());
			s_vertexBuffers[i]->SetDebugName("gui_arrow_vertex_buffer_" +std::to_string(i));
		}
	}
	SetSize(8,8);
	SetColor(0.f,0.f,0.f,1.f);
	SetDirection(Direction::Down);
	SetMouseInputEnabled(true);
}
WIArrow::~WIArrow()
{
	if(--s_arrowCount == 0u)
		s_vertexBuffers = {};
}
void WIArrow::SetDirection(Direction dir)
{
	InitializeBufferData(*s_vertexBuffers[umath::to_integral(dir)]);
	auto &verts = s_vertices[umath::to_integral(dir)];
	m_vertices.clear();
	m_vertices.reserve(verts.size());
	for(auto &v : verts)
		m_vertices.push_back(v);
}
void WIArrow::OnCursorEntered()
{
	WIBase::OnCursorEntered();
	SetColor(COLOR_SELECTED);
}
void WIArrow::OnCursorExited()
{
	WIBase::OnCursorExited();
	SetColor(0.f,0.f,0.f,1.f);
}
