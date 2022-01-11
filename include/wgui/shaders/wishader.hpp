/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHADER_HPP__
#define __WISHADER_HPP__

#include "wgui/wguidefinitions.h"
#include <shader/prosper_shader_rect.hpp>

#undef DrawState

namespace prosper {class IPrContext;};
class WGUI;
namespace wgui
{
	struct DrawState;
	enum class StencilPipeline : uint8_t;
	DLLWGUI prosper::IRenderPass &get_render_pass(prosper::IPrContext &context);
	DLLWGUI void get_render_pass(WGUI &gui,prosper::IPrContext &context,std::shared_ptr<prosper::IRenderPass> &outRenderPass,bool msaa);
	DLLWGUI void initialize_stencil_properties(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx);
	class DLLWGUI Shader
		: public prosper::ShaderGraphics
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;
		static prosper::DescriptorSetInfo DESCRIPTOR_SET;
		static wgui::StencilPipeline ToStencilPipelineIndex(uint32_t pipelineIdx,bool *optOutMsaa=nullptr);
		static uint32_t ToAbsolutePipelineIndex(wgui::StencilPipeline pipelineIdx,bool msaa);
		static bool IsMsaaPipeline(uint32_t pipelineIdx);

		Shader(prosper::IPrContext &context,const std::string &identifier);
		Shader(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");
		bool RecordBeginDraw(prosper::ShaderBindState &bindState,wgui::DrawState &drawState,uint32_t width,uint32_t height,StencilPipeline pipelineIdx,bool msaa) const;
		virtual size_t GetBaseTypeHashCode() const override;
		using ShaderGraphics::RecordBeginDraw;
	protected:
		bool RecordSetStencilReference(prosper::ShaderBindState &bindState,uint32_t testStencilLevel) const;
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx,bool enableStencilTest);
	};
};

#endif
