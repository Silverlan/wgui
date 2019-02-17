/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXTENTRY_H__
#define __WITEXTENTRY_H__
#include "wgui/wibase.h"
#include "wgui/wihandle.h"
#include "wgui/types/witextentrybase.h"

#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WITextEntry
	: public WIBase
{
protected:
	WIHandle m_hBase;
	WIHandle m_hOutline;
	WIHandle m_hBg;
public:
	WITextEntry();
	virtual ~WITextEntry() override;
	virtual void Initialize() override;
	virtual void SetSize(int x,int y) override;
	virtual void SetMouseInputEnabled(bool b) override;
	virtual void SetKeyboardInputEnabled(bool b) override;

	void SetInputHidden(bool b);
	std::string GetText();
	void SetText(std::string text);
	void InsertText(std::string instext,int pos);
	void InsertText(std::string text);
	int GetCaretPos();
	void SetCaretPos(int pos);
	virtual bool IsNumeric() const;
	bool IsMultiLine();
	void SetMultiLine(bool bMultiLine);
	bool IsEditable();
	void SetEditable(bool bEditable);
	void GetSelectionBounds(int *start,int *end);
	void SetSelectionBounds(int start,int end);
	void ClearSelection();
	void RemoveSelection();
	void SetEnterCallback(TCallback *callback);
	virtual void RequestFocus() override;
	virtual bool HasFocus() override;
	void SetMaxLength(int length);
	int GetMaxLength();
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
#pragma warning(pop)

#endif