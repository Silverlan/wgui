// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :shaders.text;

using namespace wgui;

decltype(ShaderText::VERTEX_BINDING_VERTEX) ShaderText::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderText::VERTEX_ATTRIBUTE_POSITION) ShaderText::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, prosper::CommonBufferCache::GetSquareVertexFormat()};
decltype(ShaderText::VERTEX_ATTRIBUTE_UV) ShaderText::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_VERTEX, prosper::CommonBufferCache::GetSquareUvFormat()};

decltype(ShaderText::VERTEX_BINDING_GLYPH) ShaderText::VERTEX_BINDING_GLYPH = {prosper::VertexInputRate::Instance};
decltype(ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX) ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX = {VERTEX_BINDING_GLYPH, prosper::Format::R32_UInt};
decltype(ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS) ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS = {VERTEX_BINDING_GLYPH, prosper::Format::R32G32B32A32_SFloat};

decltype(ShaderText::DESCRIPTOR_SET_TEXTURE) ShaderText::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"GLYPH_MAP", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
decltype(ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER) ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER = {
  "GLYPH_BOUNDS",
  {prosper::DescriptorSetInfo::Binding {"GLYPH_BOUNDS", prosper::DescriptorType::UniformBuffer, prosper::ShaderStageFlags::VertexBit}},
};
ShaderText::ShaderText(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/text", "programs/gui/text") {}

ShaderText::ShaderText(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

void ShaderText::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderText>({{{prosper::Format::R8_UNorm, prosper::ImageLayout::ColorAttachmentOptimal, prosper::AttachmentLoadOp::DontCare, prosper::AttachmentStoreOp::Store, IsMsaaPipeline(pipelineIdx) ? wGUI::MSAA_SAMPLE_COUNT : prosper::SampleCountFlags::e1Bit,
	                                     prosper::ImageLayout::ShaderReadOnlyOptimal}}},
	  outRenderPass, pipelineIdx);
}

void ShaderText::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx, false); }
void ShaderText::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(VERTEX_ATTRIBUTE_UV);
	AddVertexAttribute(VERTEX_ATTRIBUTE_GLYPH_INDEX);
	AddVertexAttribute(VERTEX_ATTRIBUTE_GLYPH_BOUNDS);
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
	AddDescriptorSetGroup(DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit);
}

bool ShaderText::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const
{
	if(RecordBindVertexBuffers(bindState, {GetContext().GetCommonBufferCache().GetSquareVertexUvBuffer().get(), &glyphBoundsIndexBuffer}) == false || RecordBindDescriptorSets(bindState, {&descTextureSet}) == false || RecordPushConstants(bindState, pushConstants) == false
	  || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount(), instanceCount) == false)
		return false;
	return true;
}

///////////////////////

ShaderTextRect::ShaderTextRect(prosper::IPrContext &context, const std::string &identifier) : ShaderText(context, identifier, "programs/gui/text_cheap", "programs/gui/text_cheap") {}
ShaderTextRect::ShaderTextRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : ShaderText(context, identifier, vsShader, fsShader, gsShader) {}

bool ShaderTextRect::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const
{
	if(RecordBindVertexBuffers(bindState, {GetContext().GetCommonBufferCache().GetSquareVertexUvBuffer().get(), &glyphBoundsIndexBuffer}) == false || RecordBindDescriptorSets(bindState, {&descTextureSet}) == false || RecordPushConstants(bindState, pushConstants) == false
	  || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount(), instanceCount) == false)
		return false;
	return true;
}

void ShaderTextRect::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) { Shader::InitializeRenderPass(outRenderPass, pipelineIdx); }

void ShaderTextRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}
void ShaderTextRect::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(ShaderText::VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(ShaderText::VERTEX_ATTRIBUTE_UV);
	AddVertexAttribute(VERTEX_ATTRIBUTE_GLYPH_INDEX);
	AddVertexAttribute(VERTEX_ATTRIBUTE_GLYPH_BOUNDS);
	AddDescriptorSetGroup(ShaderText::DESCRIPTOR_SET_TEXTURE);
	AddDescriptorSetGroup(ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

///////////////////////

decltype(ShaderTextRectColor::VERTEX_BINDING_COLOR) ShaderTextRectColor::VERTEX_BINDING_COLOR = {prosper::VertexInputRate::Instance};
decltype(ShaderTextRectColor::VERTEX_ATTRIBUTE_COLOR) ShaderTextRectColor::VERTEX_ATTRIBUTE_COLOR = {VERTEX_BINDING_COLOR, prosper::Format::R32G32B32A32_SFloat};
ShaderTextRectColor::ShaderTextRectColor(prosper::IPrContext &context, const std::string &identifier) : ShaderTextRectColor {context, identifier, "programs/gui/text_cheap_color", "programs/gui/text_cheap_color"} {}
ShaderTextRectColor::ShaderTextRectColor(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : ShaderTextRect {context, identifier, vsShader, fsShader, gsShader} {}
bool ShaderTextRectColor::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IBuffer &colorBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const
{
	return RecordBindVertexBuffers(bindState, {&colorBuffer}, 2u) && ShaderTextRect::RecordDraw(bindState, glyphBoundsIndexBuffer, descTextureSet, pushConstants, instanceCount, testStencilLevel);
}
void ShaderTextRectColor::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderTextRect::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }
void ShaderTextRectColor::InitializeShaderResources()
{
	ShaderTextRect::InitializeShaderResources();
	AddVertexAttribute(VERTEX_ATTRIBUTE_COLOR);
}
