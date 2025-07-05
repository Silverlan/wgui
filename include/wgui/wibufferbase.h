// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WIBUFFERBASE_H__
#define __WIBUFFERBASE_H__

#include <sharedutils/util_weak_handle.hpp>
#include "wielementbufferdata.hpp"

namespace prosper {
	class Shader;
};
class DLLWGUI WIBufferBase : public WIBase {
  public:
	virtual ~WIBufferBase() override;
	virtual unsigned int GetVertexCount();
	virtual void Render(const DrawInfo &drawInfo, wgui::DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale = {1.f, 1.f}, uint32_t testStencilLevel = 0u, wgui::StencilPipeline stencilPipeline = wgui::StencilPipeline::Test) override;

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

#endif
