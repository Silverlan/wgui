// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.flexbox;

export import :types.base;

export namespace pragma::gui::types {
	struct DLLWGUI BoxOffsets {
		int32_t left = 0;
		int32_t top = 0;
		int32_t right = 0;
		int32_t bottom = 0;
	};

	class DLLWGUI BaseBox : public WIBase {
	  public:
		enum class BoxStateFlags : uint8_t {
			None = 0,
			SkipSizeUpdateSchedule = 1,
			SizeUpdateRequired = SkipSizeUpdateSchedule << 1,
			FixedWidth = SizeUpdateRequired << 1,
			FixedHeight = FixedWidth << 1,
			AutoSizeActivated = FixedHeight << 1,
			AutoFillWidth = AutoSizeActivated << 1,
			AutoFillHeight = AutoFillWidth << 1,
		};

		BaseBox();

		void Initialize() override;

		bool IsBackgroundElement(const WIBase &el) const;

		void SetSpacing(int32_t spacing);
		int32_t GetSpacing() const { return m_spacing; }

		void SetPadding(int32_t left, int32_t top, int32_t right, int32_t bottom);
		const BoxOffsets &GetPadding() const { return m_padding; }

		void SetChildMargin(const WIBase &el, int32_t left, int32_t top, int32_t right, int32_t bottom);
		BoxOffsets GetChildMargin(const WIBase &el) const;

		void SetFixedWidth(bool fixed);
		void SetFixedHeight(bool fixed);
		void SetFixedSize(bool fixed);
		void SetAutoSizeActivated(bool activated, bool updateImmediately = true);

		void SetAutoFillContentsToWidth(bool autoFill);
		void SetAutoFillContentsToHeight(bool autoFill);
		void SetAutoFillContents(bool autoFill);
		void SetAutoFillTarget(WIBase *el);

		virtual bool IsHorizontalBox() const=0;
		virtual bool IsVerticalBox() const=0;
		virtual bool HasBoxAlignedAnchor(WIBase *el) const=0;
	  protected:
		void OnChildAdded(WIBase *child) override;
		void OnChildRemoved(WIBase *child) override;

		void OnChildSizeChanged(WIBase &child, const Vector2i &oldSize, ChangeSource changedSource) override;
		void OnChildDeleted(WIBase &child) override;
		void OnChildVisibilityChanged(WIBase &child, bool visible) override;

		void OnSizeChanged(const Vector2i &oldSize, ChangeSource changedSource) override;
		void OnRemove() override;

		void UpdateNonAnchoredSize(const Vector2i &curSize, const Vector2i &size);
		void UpdateSize(const Vector2i &size);
		void UpdateWidth(int32_t w);
		void UpdateHeight(int32_t h);

		void SetSkipSizeUpdateSchedule(bool set);
		bool GetSkipSizeUpdateSchedule() const;

		void SetSizeUpdateRequired(bool set);
		bool GetSizeUpdateRequired() const;

		void SetFixedWidthValue(bool set);
		bool GetFixedWidth() const;

		void SetFixedHeightValue(bool set);
		bool GetFixedHeight() const;

		void SetAutoSizeActivatedValue(bool set);
		bool GetAutoSizeActivated() const;

		void SetAutoFillWidth(bool set);
		bool GetAutoFillWidth() const;

		void SetAutoFillHeight(bool set);
		bool GetAutoFillHeight() const;
	  protected:
		BoxStateFlags m_boxStateFlags = BoxStateFlags::None;

		std::optional<std::pair<bool, bool>> m_autoSizeRestore;

		WIHandle m_autoFillTarget;

		int32_t m_spacing = 0;
		BoxOffsets m_padding;

		std::unordered_map<const WIBase *, BoxOffsets> m_childMargins;
	};

	enum class FlexDirection : uint8_t { Horizontal, Vertical };

	class DLLWGUI FlexBox : public BaseBox {
	  public:
		explicit FlexBox(FlexDirection direction);
		bool IsHorizontalBox() const override;
		bool IsVerticalBox() const override;
		bool HasBoxAlignedAnchor(WIBase *el) const override;
	  protected:
		void DoUpdate() override;
		FlexDirection m_direction;
	};
}
