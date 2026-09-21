// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :shaders.colored;

pragma::gui::shaders::ShaderColored::ShaderColored(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/colored", "programs/gui/colored") {}

pragma::gui::shaders::ShaderColored::ShaderColored(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool pragma::gui::shaders::ShaderColored::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &vertBuffer, uint32_t vertCount, const ElementData &pushConstants, uint32_t testStencilLevel) const
{
	if(RecordBindVertexBuffer(bindState, vertBuffer) == false || RecordPushConstants(bindState, pushConstants) == false || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, vertCount) == false)
		return false;
	return true;
}

void pragma::gui::shaders::ShaderColored::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(ElementData), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

void pragma::gui::shaders::ShaderColored::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

///////////////////////

pragma::gui::shaders::ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/colored_cheap", "programs/gui/colored_cheap") {}

pragma::gui::shaders::ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool pragma::gui::shaders::ShaderColoredRect::RecordDraw(prosper::ShaderBindState &bindState, const ElementData &pushConstants, uint32_t testStencilLevel) const
{
	if(RecordPushConstants(bindState, pushConstants) == false || RecordBindVertexBuffer(bindState, *WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexBuffer()) == false || RecordSetStencilReference(bindState, testStencilLevel) == false
	  || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount()) == false)
		return false;
	return true;
}

void pragma::gui::shaders::ShaderColoredRect::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(ElementData), prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}
void pragma::gui::shaders::ShaderColoredRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

///////////////////////

decltype(pragma::gui::shaders::StyledRect::VERTEX_BINDING_VERTEX) pragma::gui::shaders::StyledRect::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(pragma::gui::shaders::StyledRect::VERTEX_ATTRIBUTE_POSITION) pragma::gui::shaders::StyledRect::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, prosper::CommonBufferCache::GetSquareVertexFormat()};

decltype(pragma::gui::shaders::StyledRect::VERTEX_BINDING_UV) pragma::gui::shaders::StyledRect::VERTEX_BINDING_UV = {prosper::VertexInputRate::Vertex};
decltype(pragma::gui::shaders::StyledRect::VERTEX_ATTRIBUTE_UV) pragma::gui::shaders::StyledRect::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_UV, prosper::CommonBufferCache::GetSquareUvFormat()};

decltype(pragma::gui::shaders::StyledRect::DESCRIPTOR_SET_STYLE) pragma::gui::shaders::StyledRect::DESCRIPTOR_SET_STYLE = {
	"GUI",
	{
		prosper::DescriptorSetInfo::Binding {"RECT_STYLE", prosper::DescriptorType::UniformBuffer, prosper::ShaderStageFlags::FragmentBit},
	  },
	};

pragma::gui::shaders::StyledRect::StyledRect(prosper::IPrContext &context, const std::string &identifier) : Shader{context, identifier, "programs/gui/styled_rect", "programs/gui/styled_rect"} {}

pragma::gui::shaders::StyledRect::StyledRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool pragma::gui::shaders::StyledRect::RecordDraw(prosper::ShaderBindState &bindState, const ElementData &pushConstants, prosper::IDescriptorSet &style, uint32_t testStencilLevel) const
{
	if(RecordBindRenderBuffer(bindState, *WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexUvRenderBuffer()) == false || RecordPushConstants(bindState, pushConstants) == false || RecordBindDescriptorSet(bindState, style, DESCRIPTOR_SET_STYLE.setIndex) == false || RecordSetStencilReference(bindState, testStencilLevel) == false
	  || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount()) == false)
		return false;
	return true;
}

void pragma::gui::shaders::StyledRect::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(VERTEX_ATTRIBUTE_UV);
	AttachPushConstantRange(0u, sizeof(ElementData), prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
	AddDescriptorSetGroup(DESCRIPTOR_SET_STYLE);
}
void pragma::gui::shaders::StyledRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

///////////////////////

pragma::gui::shaders::ShaderStencil::ShaderStencil(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/colored_cheap", "programs/gui/stencil") {}

bool pragma::gui::shaders::ShaderStencil::RecordDraw(prosper::ShaderBindState &bindState, const ElementData &pushConstants, uint32_t testStencilLevel) const
{
	if(RecordPushConstants(bindState, pushConstants) == false || RecordBindVertexBuffer(bindState, *WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexBuffer()) == false || RecordSetStencilReference(bindState, testStencilLevel) == false
	  || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount()) == false)
		return false;
	return true;
}

void pragma::gui::shaders::ShaderStencil::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	pipelineInfo.ToggleStencilTest(true);
	pipelineInfo.ToggleDynamicState(true, prosper::DynamicState::StencilReference);

	// Disable color write
	pipelineInfo.SetColorBlendAttachmentProperties(0, false, prosper::BlendOp::Add, prosper::BlendOp::Add, prosper::BlendFactor::Zero, prosper::BlendFactor::Zero, prosper::BlendFactor::Zero, prosper::BlendFactor::Zero, prosper::ColorComponentFlags::None);

	switch(Shader::ToStencilPipelineIndex(pipelineIdx)) {
	case StencilPipeline::Test:
	case StencilPipeline::Increment:
		pipelineInfo.SetStencilTestProperties(true, prosper::StencilOp::Keep, /* fail */
		  prosper::StencilOp::IncrementAndClamp,                              /* pass */
		  prosper::StencilOp::Keep,                                           /* depth fail */
		  prosper::CompareOp::Equal, ~0, ~0, 0);
		break;
	case StencilPipeline::Decrement:
		pipelineInfo.SetStencilTestProperties(true, prosper::StencilOp::Keep, /* fail */
		  prosper::StencilOp::DecrementAndClamp,                              /* pass */
		  prosper::StencilOp::Keep,                                           /* depth fail */
		  prosper::CompareOp::Equal, ~0, ~0, 0);
		break;
	}
}

void pragma::gui::shaders::ShaderStencil::InitializeShaderResources()
{
	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(ElementData), prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}
