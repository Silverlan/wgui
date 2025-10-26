// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <array>
#include <cinttypes>
#include <memory>
#include <optional>

#undef DrawState

export module pragma.gui:draw_info;

export import pragma.prosper;

export {
	class WIBase;
	namespace wgui {
		struct DrawState;
		struct DLLWGUI DrawInfo {
			enum class Flags : uint8_t {
				None = 0u,
				UseScissor = 1u,
				UseStencil = UseScissor << 1u,
				Msaa = UseStencil << 1u,
				DontSkipIfOutOfBounds = Msaa << 1u, // This will apply to all children as well
			};
			DrawInfo(const std::shared_ptr<prosper::ICommandBuffer> &cmdBuf) : commandBuffer {cmdBuf} {}
			Vector2i offset = {};
			Vector2i size = {};
			std::optional<Vector4> color = {};
			Mat4 transform = umat::identity();
			std::optional<Mat4> postTransform = {};
			mutable std::shared_ptr<prosper::ICommandBuffer> commandBuffer;
			Flags flags = Flags::UseScissor;

			Vector4 GetColor(WIBase &el, const wgui::DrawState &drawState) const;
		};
	};
}
