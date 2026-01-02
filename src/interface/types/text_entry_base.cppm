// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "util_enum_flags.hpp"

export module pragma.gui:types.text_entry_base;

import :handle;
import :text_tags;
import :types.base;
import pragma.string.unicode;

export namespace pragma::gui::types {
	class WIRect;
	class WIText;
	class DLLWGUI WITextEntryBase : public WIBase {
	  public:
		enum class StateFlags : uint32_t { None = 0u, Numeric = 1u, MultiLine = Numeric << 1u, Editable = MultiLine << 1u, Selectable = Editable << 1u };

		WITextEntryBase();
		virtual ~WITextEntryBase() override;
		virtual void Initialize() override;
		virtual void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd) override;
		virtual void RequestFocus() override;
		virtual void OnFocusGained() override;
		virtual void OnFocusKilled() override;
		void SetInputHidden(bool b);
		bool IsInputHidden() const;
		string::Utf8StringView GetText() const;
		WIText *GetTextElement();
		void SetText(const string::Utf8StringArg &text);
		void InsertText(const string::Utf8StringArg &instext, int pos);
		void InsertText(const string::Utf8StringArg &text);
		virtual util::EventReply MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods) override;
		virtual util::EventReply KeyboardCallback(platform::Key key, int scanCode, platform::KeyState state, platform::Modifier mods) override;
		virtual util::EventReply CharCallback(unsigned int c, platform::Modifier mods = platform::Modifier::None) override;
		virtual util::EventReply OnDoubleClick() override;
		virtual void SetSize(int x, int y) override;
		int GetCaretPos() const;
		void SetCaretPos(int pos);
		WIRect *GetCaretElement();

		virtual void SetColor(float r, float g, float b, float a = 1.f) override;
		using WIBase::SetColor;

		bool IsNumeric() const;
		void SetNumeric(bool bNumeric);
		bool IsMultiLine() const;
		void SetMultiLine(bool bMultiLine);
		bool IsEditable() const;
		void SetEditable(bool bEditable);
		bool IsSelectable() const;
		void SetSelectable(bool bSelectable);

		void GetSelectionBounds(int *start, int *end) const;
		void SetSelectionBounds(int start, int end);
		void ClearSelection();
		bool RemoveSelectedText();
		void OnEnter();
		void SetMaxLength(int length);
		int GetMaxLength() const;
		void SetEntryFieldElement(WIBase *el);
	  protected:
		virtual void OnTextContentsChanged();
		std::shared_ptr<WITextDecorator> m_selectionDecorator = nullptr;
		WIHandle m_hText;
		WIHandle m_hCaret;
		WIHandle m_hEntryFieldElement;
		int m_maxLength;
		int m_posCaret;
		StateFlags m_stateFlags = static_cast<StateFlags>(math::to_integral(StateFlags::Editable) | math::to_integral(StateFlags::Selectable));
		double m_tBlink;
		int m_selectStart;
		int m_selectEnd;
		bool m_bWasDoubleClick = false;
		int GetCharPos(int x, int y) const;
		int GetCharPos() const;
		int GetLineFromPos(int pos);
		void SetSelectionStart(int pos);
		void SetSelectionEnd(int pos);
		void UpdateSelection();
		std::pair<string::LineIndex, string::LineIndex> GetLineInfo(int pos, string::Utf8StringView &outLine, int *lpos) const;
		void UpdateTextPosition();
		virtual void OnTextChanged(const string::Utf8String &text, bool changedByUser);
		void OnTextChanged(bool changedByUser);
	};
};
export {
	REGISTER_ENUM_FLAGS(pragma::gui::types::WITextEntryBase::StateFlags)
}
