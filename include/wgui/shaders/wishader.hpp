/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHADER_HPP__
#define __WISHADER_HPP__

#include "wgui/wguidefinitions.h"
#include <shader/prosper_shader_rect.hpp>

namespace wgui
{
	class DLLWGUI Shader
		: public prosper::ShaderGraphics
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;
		static prosper::Shader::DescriptorSetInfo DESCRIPTOR_SET;

		Shader(prosper::Context &context,const std::string &identifier);
		Shader(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");
		bool BeginDraw(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,uint32_t width,uint32_t height,uint32_t pipelineIdx=0u);
		virtual size_t GetBaseTypeHashCode() const override;
		using ShaderGraphics::BeginDraw;
	protected:
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};
};

#endif
