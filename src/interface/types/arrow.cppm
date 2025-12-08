// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.gui:types.arrow;

import :types.shape;

export namespace pragma::gui::types {
	class DLLWGUI WIArrow : public WIShape {
	public:
		enum class DLLWGUI Direction { Right = 0, Down = 1, Left = 2, Up = 3 };
		WIArrow();
		virtual ~WIArrow() override;
		void SetDirection(Direction dir);
		virtual void OnCursorEntered() override;
		virtual void OnCursorExited() override;
	};
}
