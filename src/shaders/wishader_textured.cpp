/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_textured.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_context.hpp>
#include <prosper_command_buffer.hpp>
#include <buffers/prosper_buffer.hpp>

using namespace wgui;

decltype(ShaderTextured::VERTEX_BINDING_VERTEX) ShaderTextured::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderTextured::VERTEX_ATTRIBUTE_POSITION) ShaderTextured::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::CommonBufferCache::GetSquareVertexFormat()};

decltype(ShaderTextured::VERTEX_BINDING_UV) ShaderTextured::VERTEX_BINDING_UV = {prosper::VertexInputRate::Vertex};
decltype(ShaderTextured::VERTEX_ATTRIBUTE_UV) ShaderTextured::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_UV,prosper::CommonBufferCache::GetSquareUvFormat()};

decltype(ShaderTextured::DESCRIPTOR_SET_TEXTURE) ShaderTextured::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding {
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};
ShaderTextured::ShaderTextured(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_textured","wgui/fs_wgui_textured")
{}

ShaderTextured::ShaderTextured(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderTextured::Draw(
	const std::shared_ptr<prosper::IBuffer> &vertBuffer,const std::shared_ptr<prosper::IBuffer> &uvBuffer,
	uint32_t vertCount,prosper::IDescriptorSet &descSetTexture,
	const PushConstants &pushConstants,uint32_t testStencilLevel
)
{
	if(
		RecordBindVertexBuffers({vertBuffer.get(),uvBuffer.get()}) == false ||
		RecordBindDescriptorSets({&descSetTexture}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordSetStencilReference(testStencilLevel) == false ||
		RecordDraw(vertCount) == false
	)
		return false;
	return true;
}

void ShaderTextured::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

///////////////////////

ShaderTexturedRectExpensive::ShaderTexturedRectExpensive(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_textured_expensive","wgui/fs_wgui_textured_expensive")
{}
ShaderTexturedRectExpensive::ShaderTexturedRectExpensive(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderTexturedRectExpensive::Draw(const PushConstants &pushConstants,prosper::IDescriptorSet &descSet,uint32_t testStencilLevel)
{
	if(
		RecordBindRenderBuffer(*WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexUvRenderBuffer()) == false ||
		RecordBindDescriptorSets({&descSet}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordSetStencilReference(testStencilLevel) == false ||
		RecordDraw(prosper::CommonBufferCache::GetSquareVertexCount()) == false
	)
		return false;
	return true;
}

void ShaderTexturedRectExpensive::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,ShaderTextured::VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo,ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

///////////////////////

ShaderTexturedRect::ShaderTexturedRect(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_textured_cheap","wgui/fs_wgui_textured_cheap")
{}
ShaderTexturedRect::ShaderTexturedRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderTexturedRect::Draw(const PushConstants &pushConstants,prosper::IDescriptorSet &descSet,uint32_t testStencilLevel)
{
	if(
		RecordBindRenderBuffer(*WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexUvRenderBuffer()) == false ||
		RecordBindDescriptorSets({&descSet}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordSetStencilReference(testStencilLevel) == false ||
		RecordDraw(prosper::CommonBufferCache::GetSquareVertexCount()) == false
	)
		return false;
	return true;
}

void ShaderTexturedRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,ShaderTextured::VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo,ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}
