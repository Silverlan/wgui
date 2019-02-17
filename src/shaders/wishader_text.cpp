/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_text.hpp"
#include "wgui/fontmanager.h"
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_util.hpp>
#include <prosper_util_square_shape.hpp>
#include <vulkan/vulkan.hpp>
#include <wrappers/descriptor_set_group.h>

using namespace wgui;

decltype(ShaderText::VERTEX_BINDING_VERTEX) ShaderText::VERTEX_BINDING_VERTEX = {Anvil::VertexInputRate::VERTEX};
decltype(ShaderText::VERTEX_ATTRIBUTE_POSITION) ShaderText::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::util::get_square_vertex_format()};
decltype(ShaderText::VERTEX_ATTRIBUTE_UV) ShaderText::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_VERTEX,prosper::util::get_square_uv_format()};

decltype(ShaderText::VERTEX_BINDING_GLYPH) ShaderText::VERTEX_BINDING_GLYPH = {Anvil::VertexInputRate::INSTANCE};
decltype(ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX) ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX = {VERTEX_BINDING_GLYPH,Anvil::Format::R32_UINT};
decltype(ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS) ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS = {VERTEX_BINDING_GLYPH,Anvil::Format::R32G32B32A32_SFLOAT};

decltype(ShaderText::DESCRIPTOR_SET_TEXTURE) ShaderText::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding {
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
decltype(ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER) ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER = {
	{
		prosper::Shader::DescriptorSetInfo::Binding {
			Anvil::DescriptorType::UNIFORM_BUFFER,
			Anvil::ShaderStageFlagBits::VERTEX_BIT
		}
	}
};
ShaderText::ShaderText(prosper::Context &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_text","wgui/fs_wgui_text")
{}

ShaderText::ShaderText(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

void ShaderText::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderText>({{{
		Anvil::Format::R8_UNORM,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,Anvil::AttachmentLoadOp::DONT_CARE,
		Anvil::AttachmentStoreOp::STORE,Anvil::SampleCountFlagBits::_1_BIT,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL
	}}},outRenderPass,pipelineIdx);
}

void ShaderText::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_UV);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_GLYPH_INDEX);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_GLYPH_BOUNDS);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),Anvil::ShaderStageFlagBits::VERTEX_BIT);
}

bool ShaderText::Draw(
	prosper::Buffer &glyphBoundsIndexBuffer,
	Anvil::DescriptorSet &descTextureSet,const PushConstants &pushConstants,
	uint32_t instanceCount
)
{
	auto &dev = GetContext().GetDevice();
	if(
		RecordBindVertexBuffers({
			&prosper::util::get_square_vertex_uv_buffer(dev)->GetAnvilBuffer(),&glyphBoundsIndexBuffer.GetAnvilBuffer()
		}) == false ||
		RecordBindDescriptorSets({&descTextureSet}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(prosper::util::get_square_vertex_count(),instanceCount) == false
	)
		return false;
	return true;
}

///////////////////////

ShaderTextRect::ShaderTextRect(prosper::Context &context,const std::string &identifier)
	: ShaderText(context,identifier,"wgui/vs_wgui_text_cheap","wgui/fs_wgui_text_cheap")
{}
ShaderTextRect::ShaderTextRect(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: ShaderText(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderTextRect::Draw(
	prosper::Buffer &glyphBoundsIndexBuffer,
	Anvil::DescriptorSet &descTextureSet,const PushConstants &pushConstants,
	uint32_t instanceCount
)
{
	auto &dev = GetContext().GetDevice();
	if(
		RecordBindVertexBuffers({
			&prosper::util::get_square_vertex_uv_buffer(dev)->GetAnvilBuffer(),&glyphBoundsIndexBuffer.GetAnvilBuffer()
			}) == false ||
		RecordBindDescriptorSets({&descTextureSet}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(prosper::util::get_square_vertex_count(),instanceCount) == false
	)
		return false;
	return true;
}

void ShaderTextRect::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderTextRect>({{{
		Anvil::Format::R8G8B8A8_UNORM,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,Anvil::AttachmentLoadOp::DONT_CARE,
		Anvil::AttachmentStoreOp::STORE,Anvil::SampleCountFlagBits::_1_BIT,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL
	}}},outRenderPass,pipelineIdx);
}

void ShaderTextRect::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	AddVertexAttribute(pipelineInfo,ShaderText::VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,ShaderText::VERTEX_ATTRIBUTE_UV);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_GLYPH_INDEX);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_GLYPH_BOUNDS);
	AddDescriptorSetGroup(pipelineInfo,ShaderText::DESCRIPTOR_SET_TEXTURE);
	AddDescriptorSetGroup(pipelineInfo,ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),Anvil::ShaderStageFlagBits::VERTEX_BIT | Anvil::ShaderStageFlagBits::FRAGMENT_BIT);
}
