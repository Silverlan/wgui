// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.vbox;

export import :types.flexbox;

export namespace pragma::gui::types {
	class VBox : public FlexBox {
	  public:
		VBox() : FlexBox {FlexDirection::Vertical} {}
	};
}
