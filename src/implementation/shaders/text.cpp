// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :shaders.text;

decltype(pragma::gui::shaders::ShaderText::VERTEX_BINDING_VERTEX) pragma::gui::shaders::ShaderText::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_POSITION) pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, prosper::CommonBufferCache::GetSquareVertexFormat()};
decltype(pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_UV) pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_VERTEX, prosper::CommonBufferCache::GetSquareUvFormat()};

decltype(pragma::gui::shaders::ShaderText::VERTEX_BINDING_GLYPH) pragma::gui::shaders::ShaderText::VERTEX_BINDING_GLYPH = {prosper::VertexInputRate::Instance};
decltype(pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX) pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_GLYPH_INDEX = {VERTEX_BINDING_GLYPH, prosper::Format::R32_UInt};
decltype(pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS) pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_GLYPH_BOUNDS = {VERTEX_BINDING_GLYPH, prosper::Format::R32G32B32A32_SFloat};

decltype(pragma::gui::shaders::ShaderText::DESCRIPTOR_SET_TEXTURE) pragma::gui::shaders::ShaderText::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"GLYPH_MAP", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
decltype(pragma::gui::shaders::ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER) pragma::gui::shaders::ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER = {
  "GLYPH_BOUNDS",
  {prosper::DescriptorSetInfo::Binding {"GLYPH_BOUNDS", prosper::DescriptorType::UniformBuffer, prosper::ShaderStageFlags::VertexBit}},
};
pragma::gui::shaders::ShaderText::ShaderText(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/text", "programs/gui/text") {}

pragma::gui::shaders::ShaderText::ShaderText(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

void pragma::gui::shaders::ShaderText::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderText>({{{prosper::Format::R8_UNorm, prosper::ImageLayout::ColorAttachmentOptimal, prosper::AttachmentLoadOp::DontCare, prosper::AttachmentStoreOp::Store, IsMsaaPipeline(pipelineIdx) ? wGUI::MSAA_SAMPLE_COUNT : prosper::SampleCountFlags::e1Bit,
	                                     prosper::ImageLayout::ShaderReadOnlyOptimal}}},
	  outRenderPass, pipelineIdx);
}

void pragma::gui::shaders::ShaderText::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx, false); }
void pragma::gui::shaders::ShaderText::InitializeShaderResources()
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

bool pragma::gui::shaders::ShaderText::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const
{
	if(RecordBindVertexBuffers(bindState, {GetContext().GetCommonBufferCache().GetSquareVertexUvBuffer().get(), &glyphBoundsIndexBuffer}) == false || RecordBindDescriptorSets(bindState, {&descTextureSet}) == false || RecordPushConstants(bindState, pushConstants) == false
	  || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount(), instanceCount) == false)
		return false;
	return true;
}

///////////////////////

pragma::gui::shaders::ShaderTextRect::ShaderTextRect(prosper::IPrContext &context, const std::string &identifier) : ShaderText(context, identifier, "programs/gui/text_cheap", "programs/gui/text_cheap") {}
pragma::gui::shaders::ShaderTextRect::ShaderTextRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : ShaderText(context, identifier, vsShader, fsShader, gsShader) {}

bool pragma::gui::shaders::ShaderTextRect::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const
{
	if(RecordBindVertexBuffers(bindState, {GetContext().GetCommonBufferCache().GetSquareVertexUvBuffer().get(), &glyphBoundsIndexBuffer}) == false || RecordBindDescriptorSets(bindState, {&descTextureSet}) == false || RecordPushConstants(bindState, pushConstants) == false
	  || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount(), instanceCount) == false)
		return false;
	return true;
}

void pragma::gui::shaders::ShaderTextRect::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) { Shader::InitializeRenderPass(outRenderPass, pipelineIdx); }

void pragma::gui::shaders::ShaderTextRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}
void pragma::gui::shaders::ShaderTextRect::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pragma::gui::shaders::ShaderText::VERTEX_ATTRIBUTE_UV);
	AddVertexAttribute(VERTEX_ATTRIBUTE_GLYPH_INDEX);
	AddVertexAttribute(VERTEX_ATTRIBUTE_GLYPH_BOUNDS);
	AddDescriptorSetGroup(pragma::gui::shaders::ShaderText::DESCRIPTOR_SET_TEXTURE);
	AddDescriptorSetGroup(pragma::gui::shaders::ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

///////////////////////

decltype(pragma::gui::shaders::ShaderTextRectColor::VERTEX_BINDING_COLOR) pragma::gui::shaders::ShaderTextRectColor::VERTEX_BINDING_COLOR = {prosper::VertexInputRate::Instance};
decltype(pragma::gui::shaders::ShaderTextRectColor::VERTEX_ATTRIBUTE_COLOR) pragma::gui::shaders::ShaderTextRectColor::VERTEX_ATTRIBUTE_COLOR = {VERTEX_BINDING_COLOR, prosper::Format::R32G32B32A32_SFloat};
pragma::gui::shaders::ShaderTextRectColor::ShaderTextRectColor(prosper::IPrContext &context, const std::string &identifier) : ShaderTextRectColor {context, identifier, "programs/gui/text_cheap_color", "programs/gui/text_cheap_color"} {}
pragma::gui::shaders::ShaderTextRectColor::ShaderTextRectColor(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : ShaderTextRect {context, identifier, vsShader, fsShader, gsShader} {}
bool pragma::gui::shaders::ShaderTextRectColor::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IBuffer &colorBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const
{
	return RecordBindVertexBuffers(bindState, {&colorBuffer}, 2u) && pragma::gui::shaders::ShaderTextRect::RecordDraw(bindState, glyphBoundsIndexBuffer, descTextureSet, pushConstants, instanceCount, testStencilLevel);
}
void pragma::gui::shaders::ShaderTextRectColor::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { pragma::gui::shaders::ShaderTextRect::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }
void pragma::gui::shaders::ShaderTextRectColor::InitializeShaderResources()
{
	pragma::gui::shaders::ShaderTextRect::InitializeShaderResources();
	AddVertexAttribute(VERTEX_ATTRIBUTE_COLOR);
}
