// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.base;
import :debug_tracker;

void pragma::gui::debug_tracker::LogEntry::AddArgument(const std::string &name, const std::string &value) { args.push_back({name, value}); }

void pragma::gui::debug_tracker::LogEntry::AddInfo(const std::string &name, const std::string &value) { info.insert(std::make_pair(name, value)); }

pragma::gui::debug_tracker::Log::~Log()
{
	for(auto &callback : callbacks) {
		if(!callback.IsValid())
			continue;
		callback.Remove();
	}
}

pragma::gui::debug_tracker::Tracker::Tracker() {}
void pragma::gui::debug_tracker::Tracker::SetLogEntryHandler(const std::function<void(const types::WIBase &, LogEntry &)> &handler) { m_logEntryHandler = handler; }
void pragma::gui::debug_tracker::Tracker::EnableTracking(const types::WIBase &el)
{
	auto it = m_trackedElements.find(&el);
	if(it != m_trackedElements.end())
		return;
	auto insertResult = m_trackedElements.insert(std::make_pair(&el, Log {}));
	auto &log = insertResult.first->second;

	auto addCallback = [&log](const CallbackHandle &cb) { log.callbacks.push_back(cb); };
	addCallback(const_cast<types::WIBase &>(el).AddCallback("OnPosChanged", FunctionCallback<void, ChangeSource>::Create([this, &el](ChangeSource changeSource) {
		auto &pos = el.GetPos();
		LogPosition(el, pos.x, pos.y, changeSource);
	})));
	addCallback(const_cast<types::WIBase &>(el).AddCallback("OnSizeChanged", FunctionCallback<void, ChangeSource>::Create([this, &el](ChangeSource changeSource) {
		auto &sz = el.GetSize();
		LogSize(el, sz.x, sz.y, changeSource);
	})));
}
void pragma::gui::debug_tracker::Tracker::DisableTracking(const types::WIBase &el)
{
	auto it = m_trackedElements.find(&el);
	if(it == m_trackedElements.end())
		return;
	m_trackedElements.erase(it);
}
void pragma::gui::debug_tracker::Tracker::AddLogEntry(const types::WIBase &el, LogEntry &entry)
{
	auto *log = GetElementLog(el);
	if(!log)
		return;
	log->entries.push_back(entry);
	if(m_logEntryHandler)
		m_logEntryHandler(el, log->entries.back());
}
void pragma::gui::debug_tracker::Tracker::LogPosition(const types::WIBase &el, int x, int y, ChangeSource changedSource)
{
	auto *log = GetElementLog(el);
	if(!log)
		return;
	LogEntry entry {};
	entry.name = "pos";
	entry.AddArgument("x", std::to_string(x));
	entry.AddArgument("y", std::to_string(y));
	entry.AddArgument("changedSource", std::string {magic_enum::enum_name(changedSource)});
	AddLogEntry(el, entry);
}
void pragma::gui::debug_tracker::Tracker::LogSize(const types::WIBase &el, int x, int y, ChangeSource changedSource)
{
	auto *log = GetLog(el);
	if(!log)
		return;
	LogEntry entry {};
	entry.info["callstack"] = debug::get_formatted_stack_backtrace_string();
	entry.name = "size";
	entry.AddArgument("x", std::to_string(x));
	entry.AddArgument("y", std::to_string(y));
	entry.AddArgument("changedSource", std::string {magic_enum::enum_name(changedSource)});
	AddLogEntry(el, entry);
}
pragma::gui::debug_tracker::Log *pragma::gui::debug_tracker::Tracker::GetElementLog(const types::WIBase &el)
{
	auto it = m_trackedElements.find(&el);
	if(it == m_trackedElements.end())
		return nullptr;
	return &it->second;
}
