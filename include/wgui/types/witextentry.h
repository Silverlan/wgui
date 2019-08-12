/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXTENTRY_H__
#define __WITEXTENTRY_H__
#include "wgui/wibase.h"
#include "wgui/wihandle.h"
#include "wgui/types/witextentrybase.h"

class DLLWGUI WITextEntry
	: public WIBase
{
protected:
	WIHandle m_hBase;
	WIHandle m_hOutline;
	WIHandle m_hBg;

	virtual void OnTextEntered();
	virtual void OnTextChanged(const std::string &text,bool changedByUser);
	virtual void OnContentsChanged();
public:
	WITextEntry();
	virtual ~WITextEntry() override;
	virtual void Initialize() override;
	virtual void SetSize(int x,int y) override;
	virtual void SetMouseInputEnabled(bool b) override;
	virtual void SetKeyboardInputEnabled(bool b) override;
	virtual void SizeToContents() override;
	virtual void SetColor(float r,float g,float b,float a=1.f) override;
	using WIBase::SetSize;
	using WIBase::SetColor;

	WIText *GetTextElement();
	void SetInputHidden(bool b);
	std::string_view GetText() const;
	void SetText(std::string_view text);
	void InsertText(std::string_view instext,int pos);
	void InsertText(std::string_view text);
	int GetCaretPos() const;
	void SetCaretPos(int pos);
	WIRect *GetCaretElement();
	virtual bool IsNumeric() const;
	bool IsMultiLine() const;
	void SetMultiLine(bool bMultiLine);
	bool IsEditable() const;
	void SetEditable(bool bEditable);
	bool IsSelectable() const;
	void SetSelectable(bool bSelectable);
	void GetSelectionBounds(int *start,int *end) const;
	void SetSelectionBounds(int start,int end);
	void ClearSelection();
	void RemoveSelection();
	void SetEnterCallback(TCallback *callback);
	virtual void RequestFocus() override;
	virtual bool HasFocus() override;
	void SetMaxLength(int length);
	int GetMaxLength() const;
};

class DLLWGUI WINumericEntry
	: public WITextEntry
{
private:
	struct Numeric
	{
		WIHandle hUpArrow;
		WIHandle hDownArrow;
		std::unique_ptr<int32_t> min;
		std::unique_ptr<int32_t> max;
	} m_numeric;
	void UpdateArrowPositions();
public:
	virtual void Initialize() override;
	virtual void SetSize(int x,int y) override;
	void SetMinValue(int32_t min);
	void SetMinValue();
	void SetMaxValue(int32_t max);
	void SetMaxValue();
	void SetRange(int32_t min,int32_t max);
	const int32_t *GetMinValue() const;
	const int32_t *GetMaxValue() const;
	virtual bool IsNumeric() const override;
};

#endif
