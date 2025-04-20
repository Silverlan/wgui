/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/witexturedsubrect.hpp"
#include <shader/prosper_shader_t.hpp>

using namespace wgui;

ShaderTexturedSubRect::ShaderTexturedSubRect(prosper::IPrContext &context, const std::string &identifier) : wgui::ShaderTextured {context, identifier, "programs/gui/textured", "programs/gui/textured_subrect"} {}

bool ShaderTexturedSubRect::RecordSetImageOffset(prosper::ShaderBindState &bindState, const Vector2 &offset, const Vector2 &scale) const
{
	static_assert(PUSH_CONSTANT_TOTAL_BASE_SIZE % 16 == 0, "Push constant size must be a multiple of 16 bytes");
	return RecordPushConstants(bindState, PushConstants {offset, scale}, PUSH_CONSTANT_TOTAL_BASE_SIZE);
}

void ShaderTexturedSubRect::InitializePushConstantRanges() { AttachPushConstantRange(0u, PUSH_CONSTANT_TOTAL_BASE_SIZE + sizeof(PushConstants), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit); }
