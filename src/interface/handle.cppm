// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <sharedutils/util_shared_handle.hpp>

export module pragma.gui:handle;

export class WIBase;
export using WIHandle = util::TWeakSharedHandle<WIBase>;
