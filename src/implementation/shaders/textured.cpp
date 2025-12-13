// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :shaders.textured;

decltype(pragma::gui::shaders::ShaderTextured::VERTEX_BINDING_VERTEX) pragma::gui::shaders::ShaderTextured::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(pragma::gui::shaders::ShaderTextured::VERTEX_ATTRIBUTE_POSITION) pragma::gui::shaders::ShaderTextured::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, prosper::CommonBufferCache::GetSquareVertexFormat()};

decltype(pragma::gui::shaders::ShaderTextured::VERTEX_BINDING_UV) pragma::gui::shaders::ShaderTextured::VERTEX_BINDING_UV = {prosper::VertexInputRate::Vertex};
decltype(pragma::gui::shaders::ShaderTextured::VERTEX_ATTRIBUTE_UV) pragma::gui::shaders::ShaderTextured::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_UV, prosper::CommonBufferCache::GetSquareUvFormat()};

decltype(pragma::gui::shaders::ShaderTextured::DESCRIPTOR_SET_TEXTURE) pragma::gui::shaders::ShaderTextured::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"TEXTURE", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
pragma::gui::shaders::ShaderTextured::ShaderTextured(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/textured", "programs/gui/textured") {}

pragma::gui::shaders::ShaderTextured::ShaderTextured(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool pragma::gui::shaders::ShaderTextured::RecordDraw(prosper::ShaderBindState &bindState, const std::shared_ptr<prosper::IBuffer> &vertBuffer, const std::shared_ptr<prosper::IBuffer> &uvBuffer, uint32_t vertCount, prosper::IDescriptorSet &descSetTexture, const PushConstants &pushConstants,
  uint32_t testStencilLevel) const
{
	if(RecordBindVertexBuffers(bindState, {vertBuffer.get(), uvBuffer.get()}) == false || RecordBindDescriptorSets(bindState, {&descSetTexture}) == false || RecordPushConstants(bindState, pushConstants) == false || RecordSetStencilReference(bindState, testStencilLevel) == false
	  || ShaderGraphics::RecordDraw(bindState, vertCount) == false)
		return false;
	return true;
}

void pragma::gui::shaders::ShaderTextured::InitializePushConstantRanges() { AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit); }

void pragma::gui::shaders::ShaderTextured::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
	InitializePushConstantRanges();
}

void pragma::gui::shaders::ShaderTextured::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

///////////////////////

pragma::gui::shaders::ShaderTexturedRectExpensive::ShaderTexturedRectExpensive(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/textured_expensive", "programs/gui/textured_expensive") {}
pragma::gui::shaders::ShaderTexturedRectExpensive::ShaderTexturedRectExpensive(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool pragma::gui::shaders::ShaderTexturedRectExpensive::RecordDraw(prosper::ShaderBindState &bindState, const PushConstants &pushConstants, prosper::IDescriptorSet &descSet, uint32_t testStencilLevel) const
{
	if(RecordBindRenderBuffer(bindState, *WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexUvRenderBuffer()) == false || RecordBindDescriptorSets(bindState, {&descSet}) == false || RecordPushConstants(bindState, pushConstants) == false
	  || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount()) == false)
		return false;
	return true;
}

void pragma::gui::shaders::ShaderTexturedRectExpensive::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(ShaderTextured::VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}
void pragma::gui::shaders::ShaderTexturedRectExpensive::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

///////////////////////

pragma::gui::shaders::ShaderTexturedRect::ShaderTexturedRect(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/textured_cheap", "programs/gui/textured_cheap") {}
pragma::gui::shaders::ShaderTexturedRect::ShaderTexturedRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool pragma::gui::shaders::ShaderTexturedRect::RecordDraw(prosper::ShaderBindState &bindState, const PushConstants &pushConstants, prosper::IDescriptorSet &descSet, uint32_t testStencilLevel) const
{
	if(RecordBindRenderBuffer(bindState, *WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexUvRenderBuffer()) == false || RecordBindDescriptorSets(bindState, {&descSet}) == false || RecordPushConstants(bindState, pushConstants) == false
	  || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount()) == false)
		return false;
	return true;
}

void pragma::gui::shaders::ShaderTexturedRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

void pragma::gui::shaders::ShaderTexturedRect::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(ShaderTextured::VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}
