// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include "util_enum_flags.hpp"

export module pragma.gui:text_iterator;

export import pragma.string.formatted_text;
export import pragma.string.unicode;

export namespace pragma::gui {
	namespace types {
		class WIText;
	}
	class DLLWGUI TextLineIteratorBase {
	  public:
		struct DLLWGUI Info {
			string::FormattedTextLine *line = nullptr;
			string::LineIndex lineIndex = 0;
			string::LineIndex relSubLineIndex = 0;
			string::LineIndex absSubLineIndex = 0;
			bool isLastLine = false;
			bool isLastSubLine = false;

			string::TextLength charCountLine = 0;
			string::TextLength charCountSubLine = 0;

			string::CharOffset relCharStartOffset = 0;
			string::TextOffset absLineStartOffset = 0;
			string::TextOffset GetAbsCharStartOffset() const { return absLineStartOffset + relCharStartOffset; }
		};

		using iterator_category = std::forward_iterator_tag;
		using value_type = Info;
		using difference_type = value_type;
		using pointer = value_type *;
		using reference = value_type &;

		static const auto INVALID_LINE = std::numeric_limits<string::LineIndex>::max();
		static const Info INVALID_LINE_INFO;

		TextLineIteratorBase(types::WIText &text, string::LineIndex lineIndex, string::LineIndex subLineIndex = 0, bool iterateSubLines = true);
		TextLineIteratorBase(const TextLineIteratorBase &other) = default;
		TextLineIteratorBase &operator=(const TextLineIteratorBase &other) = default;

		const value_type &operator++();
		const value_type &operator++(int);
		const value_type &operator--();
		const value_type &operator--(int);
		const value_type &operator*() const;

		bool operator==(const TextLineIteratorBase &other) const;
		bool operator!=(const TextLineIteratorBase &other) const;
	  private:
		types::WIText &GetText() const;
		void UpdateLine();
		types::WIText *m_text = nullptr;
		Info m_info = {};
		bool m_bIterateSubLines = true;
	};

	class DLLWGUI TextLineIterator {
	  public:
		TextLineIterator(types::WIText &text, string::LineIndex startLineIndex = 0, string::LineIndex subLineIndex = 0, bool iterateSubLines = true);
		TextLineIteratorBase begin() const;
		TextLineIteratorBase end() const;
	  private:
		types::WIText &m_text;
		bool m_bIterateSubLines = true;
		string::LineIndex m_startLineIndex = 0;
		string::LineIndex m_startSubLineIndex = 0;
	};

	//////////////////////

	class DLLWGUI CharIteratorBase {
	  public:
		struct DLLWGUI Info {
			string::LineIndex lineIndex = 0;
			string::LineIndex subLineIndex = 0;
			string::CharOffset charOffsetRelToSubLine = 0;
			string::CharOffset charOffsetRelToLine = 0;
			string::CharOffset charOffsetRelToText = 0;

			uint32_t pxOffset = 0;
			uint32_t pxWidth = 0;
		};
		enum class Flags : uint32_t { None = 0, BreakAtEndOfSubLine = 1, UpdatePixelWidth = BreakAtEndOfSubLine << 1 };

		using iterator_category = std::forward_iterator_tag;
		using value_type = Info;
		using difference_type = value_type;
		using pointer = value_type *;
		using reference = value_type &;

		static const Info INVALID_INFO;

		CharIteratorBase(types::WIText &text, string::LineIndex lineIndex, string::LineIndex subLineIndex, string::TextOffset absLineStartOffset, string::CharOffset charOffset, Flags flags);

		const value_type &operator++();
		const value_type &operator++(int);
		const value_type &operator*() const;

		bool operator==(const CharIteratorBase &other) const;
		bool operator!=(const CharIteratorBase &other) const;
	  private:
		void UpdatePixelWidth();
		types::WIText &GetText() const;
		types::WIText *m_text = nullptr;
		Info m_info = {};
		Flags m_flags = Flags::BreakAtEndOfSubLine;
	};

	class DLLWGUI CharIterator {
	  public:
		CharIterator(types::WIText &text, string::LineIndex lineIndex, string::LineIndex subLineIndex, string::TextOffset absLineStartOffset, string::CharOffset charOffset = 0, bool updatePixelWidth = false, bool breakAtEndOfSubLine = true);
		CharIterator(types::WIText &text, const TextLineIteratorBase::Info &lineInfo, bool updatePixelWidth = false, bool breakAtEndOfSubLine = true);
		CharIteratorBase begin() const;
		CharIteratorBase end() const;
	  private:
		types::WIText &m_text;
		string::LineIndex m_lineIndex = 0;
		string::LineIndex m_subLineIndex = 0;
		string::TextOffset m_absLineStartOffset = 0;
		string::CharOffset m_charOffset = 0;
		CharIteratorBase::Flags m_flags = CharIteratorBase::Flags::BreakAtEndOfSubLine;
	};
};

export {REGISTER_ENUM_FLAGS(pragma::gui::CharIteratorBase::Flags)}
