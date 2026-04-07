// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.segmented_rect;

export import :shaders.textured;
export import :types.rect;

export namespace pragma::gui::types {
	class DLLWGUI WISegmentedRectSegment : public WITexturedRect {
	  public:
		WISegmentedRectSegment();
		virtual void Initialize() override;
		void SetRenderImageOffset(const Vector2 &offset) { m_renderOffset = offset; }
		void SetRenderImageScale(const Vector2 &scale) { m_renderScale = scale; }
		const Vector2 &GetRenderImageOffset() const { return m_renderOffset; }
		const Vector2 &GetRenderImageScale() const { return m_renderScale; }

		virtual void BindShader(shaders::ShaderTextured &shader, prosper::ShaderBindState &bindState, DrawState &drawState) override;
	  private:
		Vector2 m_renderOffset {};
		Vector2 m_renderScale {1.f, 1.f};
	};

	template<typename TBase>
	class DLLWGUI WIBaseSegmentedRect : public TBase {
	  public:
		enum class Segment : uint8_t {
			TopLeftCorner = 0,
			TopRightCorner,
			BottomLeftCorner,
			BottomRightCorner,
			TopEdge,
			BottomEdge,
			LeftEdge,
			RightEdge,
			Center,
			Count,
		};

		struct NineSlice {
			uint32_t left = 0;
			uint32_t right = 0;
			uint32_t top = 0;
			uint32_t bottom = 0;
		};

		WIBaseSegmentedRect();
		virtual void Initialize() override;
		std::pair<int32_t, int32_t> GetSegmentSize(Segment segment) const;
		std::pair<int32_t, int32_t> GetSegmentOffset(Segment segment) const;
	  protected:
		virtual material::Material *GetSegmentMaterial() = 0;
		const material::Material *GetSegmentMaterial() const { return const_cast<WIBaseSegmentedRect<TBase> *>(this)->GetSegmentMaterial(); }
		virtual void UpdateMaterial(material::Material &mat);
		bool GetImageSize(uint32_t &w, uint32_t &h) const;
		std::pair<int32_t, int32_t> GetSegmentSize(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const;
		std::pair<int32_t, int32_t> GetSegmentOffset(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const;
		std::tuple<float, float, float, float> GetSegmentAnchor(Segment segment) const;
		NineSlice m_nineSlice;
	};

	class DLLWGUI WISegmentedRect : public WIBaseSegmentedRect<WIBase> {
	  public:
		WISegmentedRect();
		void Initialize() override;
		material::Material *GetMaterial();
		void SetMaterial(const std::string &matPath);
		void SetMaterial(material::Material &mat);
	  protected:
		material::Material *GetSegmentMaterial() override;
		void UpdateMaterial(material::Material &mat) override;
		void UpdateSegments();
		std::array<WIHandle, math::to_integral(Segment::Count)> m_segmentElements;
		material::MaterialHandle m_material;
	};
};

export namespace pragma::gui::types {
	template<typename TBase>
	pragma::gui::types::WIBaseSegmentedRect<TBase>::WIBaseSegmentedRect() : TBase {}
	{
	}

	template<typename TBase>
	void pragma::gui::types::WIBaseSegmentedRect<TBase>::Initialize()
	{
		TBase::Initialize();

		TBase::SetSize(512, 512);
	}

	template<typename TBase>
	std::tuple<float, float, float, float> pragma::gui::types::WIBaseSegmentedRect<TBase>::GetSegmentAnchor(Segment segment) const
	{
		switch(segment) {
		case Segment::TopLeftCorner:
			return {0.f, 0.f, 0.f, 0.f};
		case Segment::TopRightCorner:
			return {1.f, 0.f, 1.f, 0.f};
		case Segment::BottomLeftCorner:
			return {0.f, 1.f, 0.f, 1.f};
		case Segment::BottomRightCorner:
			return {1.f, 1.f, 1.f, 1.f};
		case Segment::TopEdge:
			return {0.f, 0.f, 1.f, 0.f};
		case Segment::BottomEdge:
			return {0.f, 1.f, 1.f, 1.f};
		case Segment::LeftEdge:
			return {0.f, 0.f, 0.f, 1.f};
		case Segment::RightEdge:
			return {1.f, 0.f, 1.f, 1.f};
		case Segment::Center:
			return {0.f, 0.f, 1.f, 1.f};
		}
		return {0.f, 0.f, 0.f, 0.f};
	}

	template<typename TBase>
	std::pair<int32_t, int32_t> pragma::gui::types::WIBaseSegmentedRect<TBase>::GetSegmentOffset(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const
	{
		uint32_t x = 0;
		switch(segment) {
		case Segment::TopLeftCorner:
		case Segment::LeftEdge:
		case Segment::BottomLeftCorner:
			x = 0;
			break;
		case Segment::TopRightCorner:
		case Segment::RightEdge:
		case Segment::BottomRightCorner:
			x = imgWidth - m_nineSlice.right;
			break;
		case Segment::TopEdge:
		case Segment::BottomEdge:
		case Segment::Center:
			x = m_nineSlice.left;
			break;
		}

		uint32_t y = 0;
		switch(segment) {
		case Segment::TopLeftCorner:
		case Segment::TopEdge:
		case Segment::TopRightCorner:
			y = 0;
			break;
		case Segment::BottomLeftCorner:
		case Segment::BottomEdge:
		case Segment::BottomRightCorner:
			y = imgHeight - m_nineSlice.bottom;
			break;
		case Segment::LeftEdge:
		case Segment::RightEdge:
		case Segment::Center:
			y = m_nineSlice.top;
			break;
		}
		return {x, y};
	}

	template<typename TBase>
	std::pair<int32_t, int32_t> pragma::gui::types::WIBaseSegmentedRect<TBase>::GetSegmentSize(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const
	{
		uint32_t w = 0;
		switch(segment) {
		case Segment::TopLeftCorner:
		case Segment::BottomLeftCorner:
		case Segment::LeftEdge:
			w = m_nineSlice.left;
			break;
		case Segment::TopRightCorner:
		case Segment::BottomRightCorner:
		case Segment::RightEdge:
			w = m_nineSlice.right;
			break;
		case Segment::TopEdge:
		case Segment::BottomEdge:
		case Segment::Center:
			w = imgWidth - (m_nineSlice.left + m_nineSlice.right);
			break;
		}

		uint32_t h = 0;
		switch(segment) {
		case Segment::TopLeftCorner:
		case Segment::TopRightCorner:
		case Segment::TopEdge:
			h = m_nineSlice.top;
			break;
		case Segment::BottomLeftCorner:
		case Segment::BottomRightCorner:
		case Segment::BottomEdge:
			h = m_nineSlice.bottom;
			break;
		case Segment::LeftEdge:
		case Segment::RightEdge:
		case Segment::Center:
			h = imgHeight - (m_nineSlice.top + m_nineSlice.bottom);
			break;
		}
		return {w, h};
	}

	template<typename TBase>
	std::pair<int32_t, int32_t> pragma::gui::types::WIBaseSegmentedRect<TBase>::GetSegmentSize(Segment segment) const
	{
		uint32_t imgWidth;
		uint32_t imgHeight;
		if(!GetImageSize(imgWidth, imgHeight))
			return {0, 0};
		return GetSegmentSize(segment, imgWidth, imgHeight);
	}
	template<typename TBase>
	std::pair<int32_t, int32_t> pragma::gui::types::WIBaseSegmentedRect<TBase>::GetSegmentOffset(Segment segment) const
	{
		uint32_t imgWidth;
		uint32_t imgHeight;
		if(!GetImageSize(imgWidth, imgHeight))
			return {0, 0};
		return GetSegmentOffset(segment, imgWidth, imgHeight);
	}

	template<typename TBase>
	bool pragma::gui::types::WIBaseSegmentedRect<TBase>::GetImageSize(uint32_t &w, uint32_t &h) const
	{
		auto *mat = GetSegmentMaterial();
		if(!mat)
			return false;
		auto *albedoMap = mat->GetAlbedoMap();
		if(!albedoMap || !albedoMap->texture)
			return false;
		auto &tex = *static_cast<material::Texture *>(albedoMap->texture.get());
		w = tex.GetWidth();
		h = tex.GetHeight();
		return true;
	}

	template<typename TBase>
	void pragma::gui::types::WIBaseSegmentedRect<TBase>::UpdateMaterial(material::Material &mat)
	{
		auto *albedoMap = mat.GetAlbedoMap();
		if(!albedoMap || !albedoMap->texture)
			return;
		auto &tex = *static_cast<material::Texture *>(albedoMap->texture.get());
		m_nineSlice = {};
		auto &slice = m_nineSlice;
		auto texWidth = tex.GetWidth();
		if(!mat.GetProperty<uint32_t>("9slice/leftInset", &slice.left))
			slice.left = texWidth / 4;
		if(!mat.GetProperty<uint32_t>("9slice/rightInset", &slice.right))
			slice.right = slice.left;

		auto texHeight = tex.GetHeight();
		if(!mat.GetProperty<uint32_t>("9slice/topInset", &slice.top))
			slice.top = texHeight / 4;
		if(!mat.GetProperty<uint32_t>("9slice/bottomInset", &slice.bottom))
			slice.bottom = slice.top;
	}
}
