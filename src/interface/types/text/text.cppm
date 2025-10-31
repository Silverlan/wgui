// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"

#undef DrawState



export module pragma.gui:types.text;

import :draw_info;
import :draw_state;
import :font_manager;
export import :handle;
import :shaders.text;
export import :text_tags;
export import :types.base;
export import pragma.string.formatted_text;
export import pragma.string.unicode;

export {
	class DLLWGUI WITextBase : public WIBase {
	public:
		struct SubBufferInfo {
			std::shared_ptr<prosper::IBuffer> buffer;
			std::shared_ptr<prosper::IBuffer> colorBuffer;
			const FontInfo *font = nullptr;
			util::text::TextLength numChars = 0u;
			util::text::CharOffset charOffset = 0u;
			util::text::LineIndex absLineIndex = 0;
			uint32_t width = 0u;
			uint32_t height = 0u;
			float sx = 0.f;
			float sy = 0.f;
		};

		virtual void Render(const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale = {1.f, 1.f}, uint32_t testStencilLevel = 0u, wgui::StencilPipeline stencilPipeline = wgui::StencilPipeline::Test) override;
		void InitializeTexture(prosper::Texture &tex, int32_t w, int32_t h);
		void SetTextElement(WIText &elText);
	private:
		bool RenderLines(std::shared_ptr<prosper::ICommandBuffer> &drawCmd, wgui::ShaderTextRect &shader, const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState, const Vector2i &absPos, const umath::ScaledTransform &transform, const Vector2 &scale, Vector2i &inOutSize,
		wgui::ShaderTextRect::PushConstants &inOutPushConstants, const std::function<void(prosper::ShaderBindState &, const SubBufferInfo &, prosper::IDescriptorSet &)> &fDraw, bool colorPass, wgui::StencilPipeline stencilPipeline) const;
		void RenderLines(std::shared_ptr<prosper::ICommandBuffer> &drawCmd, const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState, const Vector2i &absPos, const umath::ScaledTransform &transform, const Vector2 &scale, Vector2i &inOutSize, wgui::ShaderTextRect::PushConstants &inOutPushConstants,
		uint32_t testStencilLevel, wgui::StencilPipeline stencilPipeline) const;
		WIHandle m_hTexture = {};
		WIHandle m_hText = {};
	};

	class DLLWGUI WIText : public WIBase {
	public:
		struct DLLWGUI RangeInfo {
			int32_t offset = 0;
			int32_t length = 0;
			int32_t GetEndOffset() const { return offset + length - 1; } // Note: May be negative if length is 0
			void SetEndOffset(int32_t endOffset) { length = endOffset - offset + 1; }
		};
		friend WITextBase;
	public:
		enum class AutoBreak : int { NONE, ANY, WHITESPACE };
		enum class Flags : uint8_t {
			None = 0u,
			RenderTextScheduled = 1u,
			FullUpdateScheduled = RenderTextScheduled << 1u,

			Cache = FullUpdateScheduled << 1u,
			TextDirty = Cache << 1u,
			ApplySubTextTags = TextDirty << 1u,
			HideText = ApplySubTextTags << 1u // If enabled, text will be rendered as '*'
		};
		enum class TagType : uint32_t { None = 0u, Color, Link, Underline, Tooltip, Template };
		struct DLLWGUI LineInfo {
			LineInfo() = default;
			~LineInfo();
			void ClearBuffers();
			const std::vector<WITextBase::SubBufferInfo> &GetBuffers() const;
			size_t GetBufferCount() const;
			WITextBase::SubBufferInfo *GetBufferInfo(size_t idx);
			const WITextBase::SubBufferInfo *GetBufferInfo(size_t idx) const;
			void AddBuffer(prosper::IBuffer &buf);
			void ResizeBuffers(size_t size);

			std::weak_ptr<util::text::FormattedTextLine> wpLine;
			int32_t widthInPixels = 0u;
			bool bufferUpdateRequired = true;
			util::text::LineIndex subLineIndexOffset = 0;
			std::vector<util::text::TextLength> subLines = {};
		private:
			std::vector<WITextBase::SubBufferInfo> buffers = {};
		};
		static constexpr auto MAX_CHARS_PER_BUFFER = 32u;

		WIText();
		virtual ~WIText() override;
		WIText(const WIText &) = delete;
		void operator=(const WIText &) = delete;
		virtual void Initialize() override;
		WITextBase *GetBaseElement();
		const FontInfo *GetFont() const;
		uint32_t GetLineCount() const;
		uint32_t GetTextWidth() const;
		uint32_t GetTotalLineCount() const;
		const std::vector<LineInfo> &GetLines() const;
		std::vector<LineInfo> &GetLines();
		util::text::FormattedTextLine *GetLine(util::text::LineIndex lineIdx);
		int GetLineHeight() const;
		int GetBreakHeight();
		void SetBreakHeight(int breakHeight);
		const util::text::FormattedText &GetFormattedTextObject() const;
		util::text::FormattedText &GetFormattedTextObject();
		const pragma::string::Utf8String &GetText() const;
		const pragma::string::Utf8String &GetFormattedText() const;
		void SetText(const pragma::string::Utf8StringArg &text);
		void SetFont(const std::string_view &font);
		void SetFont(const FontInfo *font, bool reload = false);
		void ReloadFont();
		void SetAutoBreakMode(AutoBreak b);
		AutoBreak GetAutoBreakMode() const;
		using WIBase::SetSize;
		virtual void SetSize(int x, int y) override;
		int GetTextHeight();
		Vector2i CalcTextSize() const;
		std::shared_ptr<prosper::Texture> GetTexture() const;
		virtual std::string GetDebugInfo() const override;
		std::pair<Vector2i, Vector2i> GetCharacterPixelBounds(util::text::LineIndex lineIdx, util::text::CharOffset charOffset) const;
		void SetTagArgument(const std::string &tagLabel, uint32_t argumentIndex, const std::string &arg);
		void SetTagArgument(const std::string &tagLabel, uint32_t argumentIndex, const Vector4 &arg);
		void SetTagArgument(const std::string &tagLabel, uint32_t argumentIndex, const CallbackHandle &arg);
		void SetTagArgument(const std::string &tagLabel, uint32_t argumentIndex, const Color &arg);
		void SetTagsEnabled(bool bEnabled);
		bool AreTagsEnabled() const;
		Color GetCharColor(util::text::TextOffset offset) const;

		bool IsTextHidden() const;
		void HideText(bool hide = true);

		void UpdateSubLines();
		void AppendText(const pragma::string::Utf8StringArg &text);
		bool InsertText(const pragma::string::Utf8StringArg &text, util::text::LineIndex lineIdx, util::text::CharOffset charOffset = util::text::LAST_CHAR);
		void AppendLine(const pragma::string::Utf8StringArg &line);
		void PopFrontLine();
		void PopBackLine();
		void RemoveLine(util::text::LineIndex lineIdx);
		bool RemoveText(util::text::LineIndex lineIdx, util::text::CharOffset charOffset, util::text::TextLength len);
		bool RemoveText(util::text::TextOffset offset, util::text::TextLength len);
		bool MoveText(util::text::LineIndex lineIdx, util::text::CharOffset startOffset, util::text::TextLength len, util::text::LineIndex targetLineIdx, util::text::CharOffset targetCharOffset = util::text::LAST_CHAR);
		pragma::string::Utf8StringView Substr(util::text::TextOffset startOffset, util::text::TextLength len) const;
		void Clear();

		void SetTabSpaceCount(uint32_t numberOfSpaces);
		uint32_t GetTabSpaceCount() const;

		void SetAutoSizeToText(bool bAutoSize);
		bool ShouldAutoSizeToText() const;
		void UpdateTags();

		void SetDirty();
		bool IsDirty() const;

		// Shadow
		void EnableShadow(bool b);
		bool IsShadowEnabled();
		void SetShadowOffset(Vector2i offset);
		virtual void SetShadowOffset(int x, int y);
		Vector2i *GetShadowOffset();
		void SetShadowColor(Vector4 col);
		void SetShadowColor(const Color &col);
		virtual void SetShadowAlpha(float alpha);
		float GetShadowAlpha();
		Vector4 *GetShadowColor();
		virtual void SetShadowColor(float r, float g, float b, float a = 1.f);
		virtual void SetShadowBlurSize(float size);
		float GetShadowBlurSize();
		//

		virtual void SelectShader();
		virtual void Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd) override;
		virtual void SizeToContents(bool x = true, bool y = true) override;

		void SetCacheEnabled(bool bEnabled);
		bool IsCacheEnabled() const;

		template<class TDecorator, typename... TARGS>
		std::shared_ptr<WITextDecorator> AddDecorator(TARGS &&...args)
		{
			auto pDecorator = std::make_shared<TDecorator>(*this, std::forward<TARGS>(args)...);
			if constexpr(std::is_base_of_v<WITextTag, TDecorator>) {
				InitializeTagArguments(*pDecorator, pDecorator->GetArguments());
			}
			pDecorator->Initialize();
			auto startOffset = pDecorator->GetStartTextCharOffset();
			auto it = std::find_if(m_tagInfos.begin(), m_tagInfos.end(), [startOffset](const std::shared_ptr<WITextDecorator> &decoratorOther) { return decoratorOther->GetStartTextCharOffset() >= startOffset; });
			m_tagInfos.insert(it, pDecorator);
			pDecorator->SetDirty();
			m_flags = static_cast<Flags>(umath::to_integral(m_flags) | umath::to_integral(Flags::ApplySubTextTags));
			return pDecorator;
		}
		void RemoveDecorator(const WITextDecorator &decorator);

		static void InitializeTextBuffer(prosper::IPrContext &context);
		static void ClearTextBuffer();
	private:
		struct WITextShadowInfo {
			WITextShadowInfo() : enabled(false), blurSize(0.f) {}
			Vector2i offset;
			bool enabled;
			Vector4 color;
			float blurSize;
		};
	private:
		static std::shared_ptr<prosper::IUniformResizableBuffer> s_textBuffer;
		util::WeakHandle<prosper::Shader> m_shader = {};
		std::shared_ptr<util::text::FormattedText> m_text = nullptr;
		std::vector<LineInfo> m_lineInfos = {};

		std::vector<std::shared_ptr<WITextDecorator>> m_tagInfos = {};
		std::unordered_map<std::string, std::vector<std::weak_ptr<WITextTag>>> m_labelToDecorators = {};

		std::unordered_map<std::string, std::unordered_map<uint32_t, WITextTagArgument>> m_tagArgumentOverrides = {};
		WIHandle m_baseEl;
		std::shared_ptr<const FontInfo> m_font;
		int m_breakHeight;
		AutoBreak m_autoBreak;
		bool m_bAutoSizeToText = false;
		uint32_t m_tabSpaceCount = 4u;

		// Shadow
		WIHandle m_baseTextShadow;
		WITextShadowInfo m_shadow;
		//

		// Render Text
		std::shared_ptr<prosper::RenderTarget> m_renderTarget = nullptr;
		std::shared_ptr<prosper::RenderTarget> m_shadowRenderTarget = nullptr;
		std::shared_ptr<prosper::BlurSet> m_shadowBlurSet = nullptr;
		//

		Flags m_flags = Flags::FullUpdateScheduled;
		unsigned int m_wTexture;
		unsigned int m_hTexture;

		bool BreakLineByWidth(uint32_t lineIndex, util::text::ShiftOffset &lineShift);

		void PerformTextPostProcessing();
		void AutoSizeToText();

		void SetFlag(Flags flag, bool enabled = true);
		void InitializeTagArguments(const WITextTag &tag, std::vector<WITextTagArgument> &args) const;
		void SetTagArgument(const std::string &tagLabel, uint32_t argumentIndex, const WITextTagArgument &arg);
		void ApplySubTextTags();
		void ApplySubTextTag(WITextDecorator &tag);
		void InitializeTextBuffers(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd);
		void InitializeTextBuffers(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd, LineInfo &lineInfo, util::text::LineIndex lineIndex);
		void UpdateRenderTexture(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd);
		void GetTextSize(int *w, int *h, const pragma::string::Utf8StringView *inText = nullptr, const FontInfo *font = nullptr);
		void RenderText();
		void RenderText(Mat4 &mat);
		void InitializeShadow(bool bReload = false);
		void DestroyShadow();
		void InitializeBlur(bool bReload = false);
		void DestroyBlur();
		void ScheduleRenderUpdate(bool bFull = false);
	};
	namespace umath::scoped_enum::bitwise {
		template<>
		struct enable_bitwise_operators<WIText::Flags> : std::true_type {};
	}
};
