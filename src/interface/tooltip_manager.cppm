// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:tooltip_manager;

import :display_text;

export namespace pragma::gui {
	namespace types {
		class WIBase;
	}
	class DLLWGUI TooltipManager {
	  public:
		void Set(const types::WIBase &e, const string::Utf8StringArg &msg);
		void Set(const types::WIBase &e, const LocalizedString &str);
		void Clear(const types::WIBase &e);
		bool Contains(const types::WIBase &e) const;
		std::optional<string::Utf8String> Get(const types::WIBase &e) const;
	  private:
		std::unordered_map<const types::WIBase *, DisplayText> tooltips;
	};

	void TooltipManager::Set(const types::WIBase &e, const string::Utf8StringArg &msg) { tooltips[&e] = msg->to_str(); }
	void TooltipManager::Set(const types::WIBase &e, const LocalizedString &str) { tooltips[&e] = str; }
	void TooltipManager::Clear(const types::WIBase &e)
	{
		auto it = tooltips.find(&e);
		if(it == tooltips.end())
			return;
		tooltips.erase(it);
	}
	bool TooltipManager::Contains(const types::WIBase &e) const { return tooltips.find(&e) != tooltips.end(); }
	std::optional<string::Utf8String> TooltipManager::Get(const types::WIBase &e) const
	{
		auto it = tooltips.find(&e);
		if(it == tooltips.end())
			return {};
		auto &displayText = it->second;
		if(std::holds_alternative<string::Utf8String>(displayText))
			return std::get<string::Utf8String>(displayText);
		return std::get<LocalizedString>(displayText).Resolve();
	}
}
