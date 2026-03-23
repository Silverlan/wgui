// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:display_text;

import pragma.string.unicode;

export namespace pragma::gui {
	struct DLLWGUI LocalizedString {
		using FormatArg = std::variant<string::Utf8String, LocalizedString>;

		std::string key;
		std::vector<FormatArg> args;
		string::Utf8String Resolve() const;
		bool IsValid() const { return !key.empty(); }
	};
	using DisplayText = std::variant<string::Utf8String, LocalizedString>;
	using Loc = LocalizedString;
}
