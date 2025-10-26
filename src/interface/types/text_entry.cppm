// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"

#include <cinttypes>
#include <vector>
#include <memory>
#include <string>
#include <functional>

export module pragma.gui:types.text_entry;

import :handle;
import :types.base;

export {
	class WIText;
	class WIRect;
	class DLLWGUI WITextEntry : public WIBase {
	protected:
		WIHandle m_hBase;
		WIHandle m_hOutline;
		WIHandle m_hBg;

		virtual void OnTextEntered();
		virtual void OnTextChanged(const pragma::string::Utf8String &text, bool changedByUser);
		virtual void OnContentsChanged();
	public:
		WITextEntry();
		virtual ~WITextEntry() override;
		virtual void Initialize() override;
		virtual void SetSize(int x, int y) override;
		virtual void SetMouseInputEnabled(bool b) override;
		virtual void SetKeyboardInputEnabled(bool b) override;
		virtual void SizeToContents(bool x = true, bool y = true) override;
		virtual void SetColor(float r, float g, float b, float a = 1.f) override;
		using WIBase::SetColor;
		using WIBase::SetSize;

		WIText *GetTextElement();
		void SetInputHidden(bool b);
		pragma::string::Utf8StringView GetText() const;
		void SetText(const pragma::string::Utf8StringArg &text);
		void InsertText(pragma::string::Utf8StringView instext, int pos);
		void InsertText(pragma::string::Utf8StringView text);
		int GetCaretPos() const;
		void SetCaretPos(int pos);
		WIRect *GetCaretElement();
		virtual bool IsNumeric() const;
		bool IsMultiLine() const;
		void SetMultiLine(bool bMultiLine);
		bool IsEditable() const;
		virtual void SetEditable(bool bEditable);
		bool IsSelectable() const;
		void SetSelectable(bool bSelectable);
		void GetSelectionBounds(int *start, int *end) const;
		void SetSelectionBounds(int start, int end);
		void ClearSelection();
		void RemoveSelection();
		void SetEnterCallback(TCallback *callback);
		virtual void RequestFocus() override;
		virtual bool HasFocus() override;
		void SetMaxLength(int length);
		int GetMaxLength() const;
	};

	class DLLWGUI WINumericEntry : public WITextEntry {
	private:
		struct Numeric {
			WIHandle hUpArrow;
			WIHandle hDownArrow;
			std::unique_ptr<int32_t> min;
			std::unique_ptr<int32_t> max;
		} m_numeric;
		void UpdateArrowPositions();
	public:
		virtual void Initialize() override;
		virtual void SetSize(int x, int y) override;
		void SetMinValue(int32_t min);
		void SetMinValue();
		void SetMaxValue(int32_t max);
		void SetMaxValue();
		void SetRange(int32_t min, int32_t max);
		const int32_t *GetMinValue() const;
		const int32_t *GetMaxValue() const;
		virtual bool IsNumeric() const override;
	};
};
