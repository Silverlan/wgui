// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:enums;

export import std.compat;

export namespace pragma::gui {
	enum class StencilPipeline : uint8_t {
		Test = 0u,
		Increment,
		Decrement,

		Count
	};
};
