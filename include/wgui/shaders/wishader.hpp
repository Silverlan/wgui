/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHADER_HPP__
#define __WISHADER_HPP__

#include "wgui/wguidefinitions.h"
#include <shader/prosper_shader_rect.hpp>

namespace prosper {class IPrContext;};
class WGUI;
namespace wgui
{
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

		Shader(prosper::IPrContext &context,const std::string &identifier);
		Shader(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");
		bool BeginDraw(const std::shared_ptr<prosper::ICommandBuffer> &cmdBuffer,uint32_t width,uint32_t height,uint32_t pipelineIdx,bool msaa);
		virtual size_t GetBaseTypeHashCode() const override;
		using ShaderGraphics::BeginDraw;
	protected:
		static uint32_t TranslatePipelineIndex(uint32_t pipelineIdx,bool msaa);
		static bool IsMsaaPipeline(uint32_t pipelineIdx);
		bool RecordSetStencilReference(uint32_t testStencilLevel);
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx,bool enableStencilTest);
	};
};

#endif
