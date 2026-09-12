// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.hbox;

export import :types.flexbox;

export namespace pragma::gui::types {
	class HBox : public FlexBox {
	  public:
		HBox() : FlexBox {FlexDirection::Horizontal} {}
	};
}
