/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIDROPDOWNMENU_H__
#define __WIDROPDOWNMENU_H__

#include "wgui/wibase.h"
#include "witextentry.h"
#include "wgui/wihandle.h"

namespace pragma::string {
	class Utf8StringArg;
};
class DLLWGUI WIDropDownMenu;
class DLLWGUI WIDropDownMenuOption : public WIBase {
  public:
	friend WIDropDownMenu;
  protected:
	int m_index;

	bool m_selected = false;
	WIHandle m_hBackground;
	WIHandle m_hText;
	WIHandle m_dropDownMenu;
	std::string m_value;

	void UpdateTextPos();
	void SetDropDownMenu(WIDropDownMenu *menu);
  public:
	WIDropDownMenuOption();
	virtual ~WIDropDownMenuOption() override;
	virtual void Initialize() override;
	void SetValue(const std::string &val);
	const std::string &GetValue();
	void SetIndex(int idx);
	int GetIndex();
	void SetText(const pragma::string::Utf8StringArg &text);
	pragma::string::Utf8StringView GetText() const;
	WIText *GetTextElement();
	virtual void SetSize(int x, int y) override;
	virtual void OnCursorEntered() override;
	virtual void OnCursorExited() override;
	virtual void OnVisibilityChanged(bool bVisible) override;
	WIDropDownMenu *GetDropDownMenu();
	bool IsSelected() const;
};

class DLLWGUI WIDropDownMenu : public WITextEntry {
  protected:
	unsigned int m_numListItems;
	unsigned int m_listOffset;
	int m_selected;

	WIHandle m_hOutline;
	WIHandle m_hArrow;
	WIHandle m_hList;
	CallbackHandle m_cbListWindowUpdate;
	WIHandle m_hScrollBar;
	std::vector<WIHandle> m_options;

	void UpdateTextPos();
	void UpdateText();
	virtual void OnTextChanged(const pragma::string::Utf8String &text, bool changedByUser) override;
	void UpdateListWindow();
	void UpdateOptionItems(std::optional<uint32_t> oldOffset);
  public:
	WIDropDownMenu();
	virtual ~WIDropDownMenu() override;
	virtual void Initialize() override;
	virtual void OnRemove() override;
	virtual void DoUpdate() override;
	void SetListItemCount(uint32_t n);
	void SelectOption(unsigned int idx);
	void SelectOption(const std::string &value);
	void SelectOptionByText(const pragma::string::Utf8StringArg &name);
	const WIDropDownMenuOption *FindOptionByValue(const std::string &value) const;
	WIDropDownMenuOption *FindOptionByValue(const std::string &value);
	bool HasOption(const std::string &value) const;
	void OnOptionSelected(WIDropDownMenuOption *option);
	void ClearSelectedOption();
	pragma::string::Utf8StringView GetText() const;
	std::string GetValue();
	int32_t GetSelectedOption() const;
	pragma::string::Utf8StringView GetOptionText(uint32_t idx);
	std::string GetOptionValue(uint32_t idx);
	void SetOptionText(uint32_t idx, const std::string &text);
	void SetOptionValue(uint32_t idx, const std::string &val);
	void SetText(const pragma::string::Utf8StringArg &text);
	unsigned int GetOptionCount();
	WIDropDownMenuOption *AddOption(const std::string &option, const std::string &value);
	WIDropDownMenuOption *AddOption(const std::string &option);
	WIDropDownMenuOption *GetOptionElement(uint32_t idx);
	WIDropDownMenuOption *FindOptionSelectedByCursor();
	void ClearOptions();
	void SetOptions(const std::vector<std::string> &options);
	void SetOptions(const std::unordered_map<std::string, std::string> &options);
	void SetOptionOffset(unsigned int offset);
	void OpenMenu();
	void CloseMenu();
	void ScrollToOption(uint32_t offset, bool center = false);
	bool IsMenuOpen();
	void ToggleMenu();
	virtual util::EventReply MouseCallback(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods) override;
	virtual util::EventReply ScrollCallback(Vector2 offset, bool offsetAsPixels = false) override;
	virtual void SetSize(int x, int y) override;
};

#endif
