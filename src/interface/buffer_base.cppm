// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"

#undef DrawState

export module pragma.gui:buffer_base;

import :draw_info;
import :draw_state;
import :enums;
import :element_buffer_data;
import :types.base;

export class DLLWGUI WIBufferBase : public wgui::WIBase {
  public:
	~WIBufferBase() override;
	virtual unsigned int GetVertexCount();
	virtual void Render(const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale = {1.f, 1.f}, uint32_t testStencilLevel = 0u, wgui::StencilPipeline stencilPipeline = wgui::StencilPipeline::Test) override;

	prosper::IBuffer *GetBuffer();
	void SetBuffer(prosper::IBuffer &buffer);

	virtual void ClearBuffer();
  protected:
	WIBufferBase();

	virtual void SetShader(prosper::Shader &shader, prosper::Shader *shaderCheap = nullptr);
	prosper::Shader *GetShader();
	prosper::Shader *GetCheapShader();

	void InitializeBufferData(prosper::IBuffer &buffer);

	// If this isn't set, the standard square vertex buffer (with optimized shaders) will be used instead
	std::unique_ptr<WIElementVertexBufferData> m_vertexBufferData = nullptr;

	util::WeakHandle<prosper::Shader> m_shader = {};
	util::WeakHandle<prosper::Shader> m_shaderCheap = {};
};
