// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :shaders.textured_nine_slice;

pragma::gui::shaders::ShaderTexturedNineSlice::ShaderTexturedNineSlice(prosper::IPrContext &context, const std::string &identifier) : ShaderTextured {context, identifier, "programs/gui/textured", "programs/gui/textured_nineslice"} {}

bool pragma::gui::shaders::ShaderTexturedNineSlice::RecordSetNineSliceSettings(prosper::ShaderBindState &bindState, const Vector2 &uiElementSize, const Vector2 &textureSize, const Vector4 &borderSizes) const
{
	static_assert(PUSH_CONSTANT_TOTAL_BASE_SIZE % 16 == 0, "Push constant size must be a multiple of 16 bytes");
	return RecordPushConstants(bindState, PushConstants {borderSizes, uiElementSize, textureSize}, PUSH_CONSTANT_TOTAL_BASE_SIZE);
}

void pragma::gui::shaders::ShaderTexturedNineSlice::InitializePushConstantRanges() { AttachPushConstantRange(0u, PUSH_CONSTANT_TOTAL_BASE_SIZE + sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit); }
