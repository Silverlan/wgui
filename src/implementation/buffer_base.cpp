// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :buffer_base;
import :draw_info;
import :draw_state;

#undef DrawState

pragma::gui::types::WIBufferBase::WIBufferBase() : WIBase()
{
	auto *pShaderColored = WGUI::GetInstance().GetColoredShader();
	auto *pShaderColoredCheap = WGUI::GetInstance().GetColoredRectShader();
	m_shader = (pShaderColored != nullptr) ? pShaderColored->GetHandle() : util::WeakHandle<prosper::Shader> {};
	m_shaderCheap = (pShaderColoredCheap != nullptr) ? pShaderColoredCheap->GetHandle() : util::WeakHandle<prosper::Shader> {};
#if USE_STAGING_BUFFER != 0
	if(s_elementCount++ == 0u) {
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT;
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable;
		createInfo.size = sizeof(Vector4) + sizeof(Mat4);
		createInfo.flags |= prosper::util::BufferCreateInfo::Flags::Persistent;
		s_stagingBuffer = prosper::util::create_buffer(WGUI::GetInstance().GetContext().GetDevice(), createInfo);
		s_stagingBuffer->SetPermanentlyMapped(true);
	}
#endif
}

pragma::gui::types::WIBufferBase::~WIBufferBase()
{
#if USE_STAGING_BUFFER != 0
	if(--s_elementCount == 0u)
		s_stagingBuffer = nullptr;
#endif
}

void pragma::gui::types::WIBufferBase::SetShader(prosper::Shader &shader, prosper::Shader *shaderCheap)
{
	if(dynamic_cast<shaders::ShaderColored *>(&shader) != nullptr)
		m_shader = shader.GetHandle();
	else
		m_shader = {};

	if(dynamic_cast<shaders::ShaderColoredRect *>(&shader) != nullptr)
		m_shaderCheap = shader.GetHandle();
	else
		m_shaderCheap = {};
}
prosper::Shader *pragma::gui::types::WIBufferBase::GetShader() { return m_shader.get(); }
prosper::Shader *pragma::gui::types::WIBufferBase::GetCheapShader() { return m_shaderCheap.get(); }

unsigned int pragma::gui::types::WIBufferBase::GetVertexCount() { return 0; }

void pragma::gui::types::WIBufferBase::InitializeBufferData(prosper::IBuffer &buffer)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to initialize GUI element buffer data during rendering, this is not allowed!"};
	m_vertexBufferData = std::make_unique<WIElementVertexBufferData>();
	m_vertexBufferData->SetBuffer(buffer.shared_from_this());
}

prosper::IBuffer *pragma::gui::types::WIBufferBase::GetBuffer() { return m_vertexBufferData ? m_vertexBufferData->GetBuffer().get() : nullptr; }
void pragma::gui::types::WIBufferBase::SetBuffer(prosper::IBuffer &buffer)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to change GUI element buffer during rendering, this is not allowed!"};
	ClearBuffer();
	m_vertexBufferData = std::make_unique<WIElementVertexBufferData>();
	m_vertexBufferData->SetBuffer(buffer.shared_from_this());
}

void pragma::gui::types::WIBufferBase::ClearBuffer()
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to clear GUI element buffer during rendering, this is not allowed!"};
	m_vertexBufferData = nullptr;
}

void pragma::gui::types::WIBufferBase::Render(const DrawInfo &drawInfo, DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale, uint32_t testStencilLevel, StencilPipeline stencilPipeline)
{
	// Try to use cheap shader if no custom vertex buffer was used
	auto col = drawInfo.GetColor(*this, drawState);
	if(col.a <= 0.f)
		return;
	col.a *= GetLocalAlpha();
	if(m_vertexBufferData == nullptr || m_shader.expired()) {
		if(m_shaderCheap.expired())
			return;
		auto *pShader = static_cast<shaders::ShaderColoredRect *>(m_shaderCheap.get());
		auto &context = WGUI::GetInstance().GetContext();
		prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
		if(pShader->RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, umath::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa)) == true) {
			pShader->RecordDraw(bindState, {matDraw, col, ElementData::ToViewportSize(drawInfo.size)}, testStencilLevel);
			pShader->RecordEndDraw(bindState);
		}
		return;
	}
	//

	auto buf = (m_vertexBufferData != nullptr) ? m_vertexBufferData->GetBuffer() : nullptr;
	if(buf == nullptr)
		return;
	auto &shader = static_cast<shaders::ShaderColored &>(*m_shader.get());
	auto &context = WGUI::GetInstance().GetContext();
	prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
	if(shader.RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, umath::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa)) == true) {
		shader.RecordDraw(bindState, *buf, GetVertexCount(), ElementData {matDraw, col, ElementData::ToViewportSize(drawInfo.size)}, testStencilLevel);
		shader.RecordEndDraw(bindState);
	}
}
