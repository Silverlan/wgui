// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:shaders.textured_nine_slice;

export import :shaders.textured;

export namespace pragma::gui::shaders {
	class DLLWGUI ShaderTexturedNineSlice : public ShaderTextured {
	  public:
		static constexpr auto PUSH_CONSTANT_BASE_SIZE = sizeof(PushConstants);
		static constexpr size_t PUSH_CONSTANT_BASE_PADDING = 0;
		static constexpr auto PUSH_CONSTANT_TOTAL_BASE_SIZE = PUSH_CONSTANT_BASE_SIZE + PUSH_CONSTANT_BASE_PADDING;

#pragma pack(push, 1)
		struct PushConstants {
			Vector4 borderSizes; // left, right, top, bottom
			Vector2 uiElementSize;
			Vector2 textureSize;
		};
#pragma pack(pop)

		ShaderTexturedNineSlice(prosper::IPrContext &context, const std::string &identifier);
		bool RecordSetNineSliceSettings(prosper::ShaderBindState &bindState, const Vector2 &uiElementSize, const Vector2 &textureSize, const Vector4 &borderSizes) const;
	  protected:
		void InitializePushConstantRanges() override;
	};
};
