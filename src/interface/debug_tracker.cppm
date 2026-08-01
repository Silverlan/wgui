// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:debug_tracker;

import :handle;
export import pragma.util;

export namespace pragma::gui {
	enum class ChangeSource : uint8_t;
	namespace types {
		class WIBase;
	}

	namespace debug_tracker {
		struct DLLWGUI LogArg {
			std::string name;
			std::string value;
		};

		struct DLLWGUI LogEntry {
			std::string name;
			std::vector<LogArg> args;
			std::unordered_map<std::string, std::string> info;

			void AddArgument(const std::string &name, const std::string &value);
			void AddInfo(const std::string &name, const std::string &value);
		};

		struct DLLWGUI Log {
			Log() = default;
			~Log();
			Log(const Log &) = delete;
			Log &operator=(const Log &) = delete;

			Log(Log &&other) noexcept : entries(std::move(other.entries)), callbacks(std::move(other.callbacks)) { other.callbacks.clear(); }
			Log &operator=(Log &&other) noexcept
			{
				if(this != &other) {
					entries = std::move(other.entries);
					callbacks = std::move(other.callbacks);
					other.callbacks.clear();
				}
				return *this;
			}

			std::vector<LogEntry> entries;
			std::vector<CallbackHandle> callbacks;
		};

		class DLLWGUI Tracker {
		  public:
			Tracker();
			Tracker(const Tracker &) = delete;
			Tracker &operator=(const Tracker &) = delete;
			void SetLogEntryHandler(const std::function<void(const types::WIBase &, LogEntry &)> &handler);
			void EnableTracking(const types::WIBase &el);
			void DisableTracking(const types::WIBase &el);
			void LogPosition(const types::WIBase &el, int x, int y, ChangeSource changedSource);
			void LogSize(const types::WIBase &el, int x, int y, ChangeSource changedSource);
			const Log *GetLog(const types::WIBase &el) const { return const_cast<Tracker *>(this)->GetElementLog(el); }
		  private:
			void AddLogEntry(const types::WIBase &el, LogEntry &entry);
			Log *GetElementLog(const types::WIBase &el);
			std::unordered_map<const types::WIBase *, Log> m_trackedElements;
			std::function<void(const types::WIBase &, LogEntry &)> m_logEntryHandler;
		};
	};
}
