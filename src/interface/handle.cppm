// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

export module pragma.gui:handle;

export import pragma.util;

export {
	class WIBase;
	using WIHandle = util::TWeakSharedHandle<WIBase>;
}
