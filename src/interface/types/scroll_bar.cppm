// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.scroll_bar;

import :types.rect;

export namespace pragma::gui::types {
	class WIScrollBar;
	class DLLWGUI WIScrollBarSlider : public WIBase {
	  public:
		friend WIScrollBar;
	  public:
		WIScrollBarSlider();
		~WIScrollBarSlider() override = default;
		void Initialize() override;
		util::EventReply ScrollCallback(Vector2 offset, bool offsetAsPixels = false) override;
		util::EventReply MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods) override;
		void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd) override;
		void SetHorizontal(bool b);
		bool IsHorizontal() const;
		bool IsVertical() const;
	protected:
		bool m_bHorizontal;
		int m_moveOrigin;
		int m_moveOffset;
		bool m_bMoving;
		void UpdatePosition();
		void StopDragging();
		int GetSliderWidth() const;
		int GetSliderHeight() const;
		int GetSliderX() const;
		int GetSliderY() const;
		void SetSliderWidth(int w);
		void SetSliderHeight(int h);
		void SetSliderX(int x);
		void SetSliderY(int y);
		void SetSliderPos(int x, int y);
		void SetSliderSize(int w, int h);
	};

	class DLLWGUI WIScrollBar : public WIBase {
	  public:
		WIScrollBar();
		~WIScrollBar() override;
		void SetUp(unsigned int numElementsListed, unsigned int numElementsTotal);
		unsigned int GetScrollAmount() const;
		void SetScrollAmount(unsigned int am);
		unsigned int GetScrollOffset() const;
		void SetScrollOffset(unsigned int offset);
		void AddScrollOffset(int scroll);
		uint32_t GetElementCount() const;
		uint32_t GetScrollElementCount() const;
		uint32_t GetBottomScrollOffset() const;
		void Initialize() override;
		void SetHorizontal(bool b);
		bool IsHorizontal() const;
		bool IsVertical() const;
		util::EventReply ScrollCallback(Vector2 offset, bool offsetAsPixels = false) override;
		util::EventReply MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods) override;
	  protected:
		bool m_bHorizontal;
		unsigned int m_offset;
		unsigned int m_numElements;
		unsigned int m_numListed;
		unsigned int m_scrollAmount;
		WIHandle m_slider;
		WIHandle m_sliderContainer;
		WIHandle m_flexBox;
		int GetSliderWidth() const;
		int GetSliderHeight() const;
		int GetSliderX() const;
		int GetSliderY() const;
		void SetSliderWidth(int w);
		void SetSliderHeight(int h);
		void SetSliderX(int x);
		void SetSliderY(int y);
		void SetSliderPos(int x, int y);
		void SetSliderSize(int w, int h);
		void UpdateSliderSize();
		void UpdateSliderOffset();
	};
};
