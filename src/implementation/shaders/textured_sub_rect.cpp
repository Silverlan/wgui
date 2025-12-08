// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :shaders.textured_sub_rect;

pragma::gui::shaders::ShaderTexturedSubRect::ShaderTexturedSubRect(prosper::IPrContext &context, const std::string &identifier) : ShaderTextured {context, identifier, "programs/gui/textured", "programs/gui/textured_subrect"} {}

bool pragma::gui::shaders::ShaderTexturedSubRect::RecordSetImageOffset(prosper::ShaderBindState &bindState, const Vector2 &offset, const Vector2 &scale) const
{
	static_assert(PUSH_CONSTANT_TOTAL_BASE_SIZE % 16 == 0, "Push constant size must be a multiple of 16 bytes");
	return RecordPushConstants(bindState, PushConstants {offset, scale}, PUSH_CONSTANT_TOTAL_BASE_SIZE);
}

void pragma::gui::shaders::ShaderTexturedSubRect::InitializePushConstantRanges() { AttachPushConstantRange(0u, PUSH_CONSTANT_TOTAL_BASE_SIZE + sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit); }
