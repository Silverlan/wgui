// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WITEXTUREDSUBRECT_HPP__
#define __WITEXTUREDSUBRECT_HPP__

#include "wishader_textured.hpp"

namespace wgui {
	class DLLWGUI ShaderTexturedSubRect : public ShaderTextured {
	  public:
		static constexpr auto PUSH_CONSTANT_BASE_SIZE = sizeof(ShaderTextured::PushConstants);
		static constexpr auto PUSH_CONSTANT_BASE_PADDING = sizeof(float);
		static constexpr auto PUSH_CONSTANT_TOTAL_BASE_SIZE = PUSH_CONSTANT_BASE_SIZE + PUSH_CONSTANT_BASE_PADDING;

#pragma pack(push, 1)
		struct PushConstants {
			Vector2 offset;
			Vector2 scale;
		};
#pragma pack(pop)

		ShaderTexturedSubRect(prosper::IPrContext &context, const std::string &identifier);
		bool RecordSetImageOffset(prosper::ShaderBindState &bindState, const Vector2 &offset, const Vector2 &scale) const;
	  protected:
		virtual void InitializePushConstantRanges() override;
	};
};

#endif
