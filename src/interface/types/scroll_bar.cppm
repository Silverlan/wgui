// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"

#include <array>
#include <cinttypes>
#include <string>
#include <memory>

export module pragma.gui:types.scroll_bar;

import :types.rect;

export
{
	class DLLWGUI WIScrollBar;
	class DLLWGUI WIScrollBarSlider : public WIRect {
	  public:
		friend WIScrollBar;
	  protected:
		bool m_bHorizontal;
		int m_moveOrigin;
		int m_moveOffset;
		int m_min;
		int m_max;
		bool m_bMoving;
		void UpdatePosition();
		void StopDragging();
		int GetSliderWidth();
		int GetSliderHeight();
		int GetSliderX();
		int GetSliderY();
		void SetSliderWidth(int w);
		void SetSliderHeight(int h);
		void SetSliderX(int x);
		void SetSliderY(int y);
		void SetSliderPos(int x, int y);
		void SetSliderSize(int w, int h);
	  public:
		WIScrollBarSlider();
		virtual ~WIScrollBarSlider() override = default;
		virtual void Initialize() override;
		virtual util::EventReply ScrollCallback(Vector2 offset, bool offsetAsPixels = false) override;
		virtual util::EventReply MouseCallback(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods) override;
		virtual void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd) override;
		void SetHorizontal(bool b);
		bool IsHorizontal();
		bool IsVertical();
		void SetLimits(int min, int max);
	};

	class DLLWGUI WIScrollBar : public WIBase {
	  protected:
		bool m_bHorizontal;
		unsigned int m_offset;
		unsigned int m_numElements;
		unsigned int m_numListed;
		unsigned int m_scrollAmount;
		WIHandle m_buttonUp;
		WIHandle m_buttonDown;
		WIHandle m_slider;
		int GetSliderWidth();
		int GetSliderHeight();
		int GetSliderX();
		int GetSliderY();
		void SetSliderWidth(int w);
		void SetSliderHeight(int h);
		void SetSliderX(int x);
		void SetSliderY(int y);
		void SetSliderPos(int x, int y);
		void SetSliderSize(int w, int h);
		void UpdateSliderSize();
		void UpdateSliderOffset();
	  public:
		WIScrollBar();
		virtual ~WIScrollBar() override;
		void SetUp(unsigned int numElementsListed, unsigned int numElementsTotal);
		unsigned int GetScrollAmount();
		void SetScrollAmount(unsigned int am);
		unsigned int GetScrollOffset();
		void SetScrollOffset(unsigned int offset);
		void AddScrollOffset(int scroll);
		uint32_t GetElementCount() const;
		uint32_t GetScrollElementCount() const;
		uint32_t GetBottomScrollOffset();
		virtual void Initialize() override;
		void SetHorizontal(bool b);
		bool IsHorizontal();
		bool IsVertical();
		virtual util::EventReply ScrollCallback(Vector2 offset, bool offsetAsPixels = false) override;
		virtual util::EventReply MouseCallback(pragma::platform::MouseButton button, pragma::platform::KeyState state, pragma::platform::Modifier mods) override;
		virtual void SetSize(int x, int y) override;
	};
};
