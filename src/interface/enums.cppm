// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cinttypes>

export module pragma.gui:enums;

export namespace wgui {
	enum class StencilPipeline : uint8_t {
		Test = 0u,
		Increment,
		Decrement,

		Count
	};
};
