/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_util_square_shape.hpp>
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <prosper_command_buffer.hpp>

using namespace wgui;

decltype(Shader::VERTEX_BINDING_VERTEX) Shader::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(Shader::VERTEX_ATTRIBUTE_POSITION) Shader::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::util::get_square_vertex_format()};

decltype(Shader::DESCRIPTOR_SET) Shader::DESCRIPTOR_SET = {
	{
		prosper::DescriptorSetInfo::Binding {
			prosper::DescriptorType::UniformBufferDynamic,
			prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit
		}
	}
};
Shader::Shader(prosper::IPrContext &context,const std::string &identifier)
	: ShaderGraphics(context,identifier,"wgui/vs_wgui_colored","wgui/fs_wgui_colored")
{}

Shader::Shader(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: ShaderGraphics(context,identifier,vsShader,fsShader,gsShader)
{}

void Shader::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	ToggleDynamicScissorState(pipelineInfo,true);
}

size_t Shader::GetBaseTypeHashCode() const {return typeid(Shader).hash_code();}

bool Shader::BeginDraw(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &cmdBuffer,uint32_t width,uint32_t height,uint32_t pipelineIdx)
{
	if(ShaderGraphics::BeginDraw(cmdBuffer,pipelineIdx,RecordFlags::None) == false || cmdBuffer->RecordSetViewport(width,height) == false)
		return false;
	uint32_t x,y,w,h;
	WGUI::GetInstance().GetScissor(x,y,w,h);
	return cmdBuffer->RecordSetScissor(w,h,x,y);
}

