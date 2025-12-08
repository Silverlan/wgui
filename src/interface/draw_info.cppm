// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

#undef DrawState

export module pragma.gui:draw_info;

export import pragma.prosper;

export namespace pragma::gui {
	namespace types {
		class WIBase;
	}

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

		Vector4 GetColor(types::WIBase &el, const DrawState &drawState) const;
	};
}
