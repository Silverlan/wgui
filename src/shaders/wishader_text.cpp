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

decltype(ShaderText::VERTEX_BINDING_VERTEX) ShaderText::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderText::VERTEX_ATTRIBUTE_POSITION) ShaderText::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::util::get_square_vertex_format()};
decltype(ShaderText::VERTEX_ATTRIBUTE_UV) ShaderText::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_VERTEX,prosper::util::get_square_uv_format()};

decltype(ShaderText::VERTEX_BINDING_GLYPH) ShaderText::VERTEX_BINDING_GLYPH = {prosper::VertexInputRate::Instance};
decltype(ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX) ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX = {VERTEX_BINDING_GLYPH,prosper::Format::R32_UInt};
decltype(ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS) ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS = {VERTEX_BINDING_GLYPH,prosper::Format::R32G32B32A32_SFloat};

decltype(ShaderText::DESCRIPTOR_SET_TEXTURE) ShaderText::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding {
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};
decltype(ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER) ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER = {
	{
		prosper::DescriptorSetInfo::Binding {
		prosper::DescriptorType::UniformBuffer,
		prosper::ShaderStageFlags::VertexBit
		}
	}
};
ShaderText::ShaderText(prosper::Context &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_text","wgui/fs_wgui_text")
{}

ShaderText::ShaderText(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

void ShaderText::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderText>({{{
		prosper::Format::R8_UNorm,prosper::ImageLayout::ColorAttachmentOptimal,prosper::AttachmentLoadOp::DontCare,
		prosper::AttachmentStoreOp::Store,prosper::SampleCountFlags::e1Bit,prosper::ImageLayout::ShaderReadOnlyOptimal
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
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::VertexBit);
}

bool ShaderText::Draw(
	prosper::IBuffer &glyphBoundsIndexBuffer,
	prosper::IDescriptorSet &descTextureSet,const PushConstants &pushConstants,
	uint32_t instanceCount
)
{
	if(
		RecordBindVertexBuffers({
			prosper::util::get_square_vertex_uv_buffer(GetContext()).get(),&glyphBoundsIndexBuffer
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
	prosper::IBuffer &glyphBoundsIndexBuffer,
	prosper::IDescriptorSet &descTextureSet,const PushConstants &pushConstants,
	uint32_t instanceCount
)
{
	if(
		RecordBindVertexBuffers({
			prosper::util::get_square_vertex_uv_buffer(GetContext()).get(),&glyphBoundsIndexBuffer
			}) == false ||
		RecordBindDescriptorSets({&descTextureSet}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(prosper::util::get_square_vertex_count(),instanceCount) == false
	)
		return false;
	return true;
}

void ShaderTextRect::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderTextRect>({{{
		prosper::Format::R8G8B8A8_UNorm,prosper::ImageLayout::ColorAttachmentOptimal,prosper::AttachmentLoadOp::DontCare,
		prosper::AttachmentStoreOp::Store,prosper::SampleCountFlags::e1Bit,prosper::ImageLayout::ShaderReadOnlyOptimal
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
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

///////////////////////

decltype(ShaderTextRectColor::VERTEX_BINDING_COLOR) ShaderTextRectColor::VERTEX_BINDING_COLOR = {prosper::VertexInputRate::Instance};
decltype(ShaderTextRectColor::VERTEX_ATTRIBUTE_COLOR) ShaderTextRectColor::VERTEX_ATTRIBUTE_COLOR = {VERTEX_BINDING_COLOR,prosper::Format::R32G32B32A32_SFloat};
ShaderTextRectColor::ShaderTextRectColor(prosper::Context &context,const std::string &identifier)
	: ShaderTextRectColor{context,identifier,"wgui/vs_wgui_text_cheap_color","wgui/fs_wgui_text_cheap_color"}
{}
ShaderTextRectColor::ShaderTextRectColor(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: ShaderTextRect{context,identifier,vsShader,fsShader,gsShader}
{}
bool ShaderTextRectColor::Draw(
	prosper::IBuffer &glyphBoundsIndexBuffer,prosper::IBuffer &colorBuffer,
	prosper::IDescriptorSet &descTextureSet,const PushConstants &pushConstants,
	uint32_t instanceCount
)
{
	return RecordBindVertexBuffers({&colorBuffer},2u) && ShaderTextRect::Draw(glyphBoundsIndexBuffer,descTextureSet,pushConstants,instanceCount);
}
void ShaderTextRectColor::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderTextRect::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_COLOR);
}
