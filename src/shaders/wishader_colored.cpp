/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_colored.hpp"
#include "wgui/wielementdata.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_util_square_shape.hpp>

using namespace wgui;

ShaderColored::ShaderColored(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_colored","wgui/fs_wgui_colored")
{}

ShaderColored::ShaderColored(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderColored::Draw(prosper::IBuffer &vertBuffer,uint32_t vertCount,const wgui::ElementData &pushConstants)
{
	if(
		RecordBindVertexBuffer(vertBuffer) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(vertCount) == false
	)
		return false;
	return true;
}

void ShaderColored::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(wgui::ElementData),prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

///////////////////////

ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_colored_cheap","wgui/fs_wgui_colored_cheap")
{}

ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderColoredRect::Draw(const wgui::ElementData &pushConstants)
{
	if(
		RecordPushConstants(pushConstants) == false ||
		RecordBindVertexBuffer(*prosper::util::get_square_vertex_buffer(WGUI::GetInstance().GetContext())) == false ||
		RecordDraw(prosper::util::get_square_vertex_count()) == false
	)
		return false;
	return true;
}

void ShaderColoredRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(wgui::ElementData),prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}
