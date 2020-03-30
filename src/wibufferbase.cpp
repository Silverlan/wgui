/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/wibufferbase.h"
#include "wgui/shaders/wishader_colored.hpp"
#include "wgui/wielementdata.hpp"
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_util_square_shape.hpp>
#include <prosper_command_buffer.hpp>
#include <wrappers/memory_block.h>
#include <buffers/prosper_uniform_resizable_buffer.hpp>

WIBufferBase::WIBufferBase()
	: WIBase()
{
	auto *pShaderColored = WGUI::GetInstance().GetColoredShader();
	auto *pShaderColoredCheap = WGUI::GetInstance().GetColoredRectShader();
	m_shader = (pShaderColored != nullptr) ? pShaderColored->GetHandle() : util::WeakHandle<prosper::Shader>{};
	m_shaderCheap = (pShaderColoredCheap != nullptr) ? pShaderColoredCheap->GetHandle() : util::WeakHandle<prosper::Shader>{};
#if USE_STAGING_BUFFER != 0
	if(s_elementCount++ == 0u)
	{
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT;
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable;
		createInfo.size = sizeof(Vector4) +sizeof(Mat4);
		s_stagingBuffer = prosper::util::create_buffer(WGUI::GetInstance().GetContext().GetDevice(),createInfo);
		s_stagingBuffer->SetPermanentlyMapped(true);
	}
#endif
}

WIBufferBase::~WIBufferBase()
{
#if USE_STAGING_BUFFER != 0
	if(--s_elementCount == 0u)
		s_stagingBuffer = nullptr;
#endif
}

void WIBufferBase::SetShader(prosper::Shader &shader,prosper::Shader *shaderCheap)
{
	if(dynamic_cast<wgui::ShaderColored*>(&shader) != nullptr)
		m_shader = shader.GetHandle();
	else
		m_shader = {};
	
	if(dynamic_cast<wgui::ShaderColoredRect*>(&shader) != nullptr)
		m_shaderCheap = shader.GetHandle();
	else
		m_shaderCheap = {};
}
prosper::Shader *WIBufferBase::GetShader() {return m_shader.get();}
prosper::Shader *WIBufferBase::GetCheapShader() {return m_shaderCheap.get();}

unsigned int WIBufferBase::GetVertexCount() {return 0;}

void WIBufferBase::InitializeBufferData(prosper::Buffer &buffer)
{
	m_vertexBufferData = std::make_unique<WIElementVertexBufferData>();
	m_vertexBufferData->SetBuffer(buffer.shared_from_this());
}

void WIBufferBase::Render(const DrawInfo &drawInfo,const Mat4 &matDraw)
{
	// Try to use cheap shader if no custom vertex buffer was used
	auto col = drawInfo.GetColor(*this);
	if(col.a <= 0.f)
		return;
	auto &dev = WGUI::GetInstance().GetContext().GetDevice();
	if(m_vertexBufferData == nullptr || m_shader.expired())
	{
		if(m_shaderCheap.expired())
			return;
		auto *pShader = static_cast<wgui::ShaderColoredRect*>(m_shaderCheap.get());
		auto &context = WGUI::GetInstance().GetContext();
		if(pShader->BeginDraw(context.GetDrawCommandBuffer(),drawInfo.size.x,drawInfo.size.y) == true)
		{
			pShader->Draw({matDraw,col});
			pShader->EndDraw();
		}
		return;
	}
	//

	auto buf = (m_vertexBufferData != nullptr) ? m_vertexBufferData->GetBuffer() : nullptr;
	if(buf == nullptr)
		return;
	auto &shader = static_cast<wgui::ShaderColored&>(*m_shader.get());
	auto &context = WGUI::GetInstance().GetContext();
	if(shader.BeginDraw(context.GetDrawCommandBuffer(),drawInfo.size.x,drawInfo.size.y) == true)
	{
		shader.Draw(*buf,GetVertexCount(),wgui::ElementData{matDraw,col});
		shader.EndDraw();
	}
}
