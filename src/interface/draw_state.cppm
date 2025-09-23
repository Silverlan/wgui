// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <array>
#include <cinttypes>

export module pragma.gui:draw_state;

export namespace wgui {
	struct DLLWGUI DrawState {
		std::array<uint32_t, 4> scissor = {0u, 0, 0u, 0u};
		float renderAlpha = 1.f;

		void GetScissor(uint32_t &x, uint32_t &y, uint32_t &w, uint32_t &h);
		void SetScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	};
};
