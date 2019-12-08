/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXT_H__
#define __WITEXT_H__

#include "wgui/wibase.h"
#include "wgui/fontmanager.h"
#include "wgui/wihandle.h"
#include "wgui/shaders/wishader_text.hpp"
#include <image/prosper_render_target.hpp>
#include <sharedutils/property/util_property.hpp>
#include <sharedutils/util_shared_handle.hpp>
#include <util_formatted_text_types.hpp>
#include <string_view>
#include <queue>
#ifdef __linux__
	#include "wgui/types/witext_tags.hpp"
#endif

namespace prosper
{
	class Buffer;
	class Shader;
	class BlurSet;
	class UniformResizableBuffer;
};

namespace util{namespace text{class FormattedText; class TextTag; class FormattedTextLine;};};
class WGUIShaderText;
class WITextTag;
class WITextDecorator;
struct WITextTagArgument;

class WIText;
class DLLWGUI WITextBase
	: public WIBase
{
public:
	struct SubBufferInfo
	{
		std::shared_ptr<prosper::Buffer> buffer;
		std::shared_ptr<prosper::Buffer> colorBuffer;
		util::text::TextLength numChars = 0u;
		util::text::CharOffset charOffset = 0u;
		util::text::LineIndex absLineIndex = 0;
		uint32_t width = 0u;
		uint32_t height = 0u;
		float sx = 0.f;
		float sy = 0.f;
	};

	virtual void Render(int w,int h,const Mat4 &mat,const Vector2i &origin,const Mat4 &matParent) override;
	void InitializeTexture(prosper::Texture &tex,int32_t w,int32_t h);
	void SetTextElement(WIText &elText);
private:
	bool RenderLines(
		wgui::ShaderTextRect &shader,int32_t width,int32_t height,
		const Vector2i &absPos,const Mat4 &transform,
		const Vector2i &origin,const Mat4 &matParent,Vector2i &inOutSize,
		wgui::ShaderTextRect::PushConstants &inOutPushConstants,
		const std::function<void(const SubBufferInfo&,Anvil::DescriptorSet&)> &fDraw,
		bool colorPass
	) const;
	void RenderLines(
		int32_t width,int32_t height,
		const Vector2i &absPos,const Mat4 &transform,
		const Vector2i &origin,const Mat4 &matParent,Vector2i &inOutSize,
		wgui::ShaderTextRect::PushConstants &inOutPushConstants
	) const;
	WIHandle m_hTexture = {};
	WIHandle m_hText = {};
};

class DLLWGUI WIText
	: public WIBase
{
public:
	struct DLLWGUI RangeInfo
	{
		int32_t offset = 0;
		int32_t length = 0;
		int32_t GetEndOffset() const {return offset +length -1;} // Note: May be negative if length is 0
		void SetEndOffset(int32_t endOffset) {length = endOffset -offset +1;}
	};
	friend WITextBase;
public:
	enum class AutoBreak : int
	{
		NONE,
		ANY,
		WHITESPACE
	};
	enum class Flags : uint8_t
	{
		None = 0u,
		RenderTextScheduled = 1u,
		FullUpdateScheduled = RenderTextScheduled<<1u,

		Cache = FullUpdateScheduled<<1u,
		TextDirty = Cache<<1u,
		ApplySubTextTags = TextDirty<<1u
	};
	enum class TagType : uint32_t
	{
		None = 0u,
		Color,
		Link,
		Underline,
		Tooltip,
		Template
	};
	struct DLLWGUI LineInfo
	{
		LineInfo()=default;
		std::weak_ptr<util::text::FormattedTextLine> wpLine;
		int32_t widthInPixels = 0u;
		bool bufferUpdateRequired = true;
		util::text::LineIndex subLineIndexOffset = 0;
		std::vector<WITextBase::SubBufferInfo> buffers = {};
		std::vector<util::text::TextLength> subLines = {};
	};
	static const auto MAX_CHARS_PER_BUFFER = 32u;

	WIText();
	virtual ~WIText() override;
	WIText(const WIText&)=delete;
	void operator=(const WIText&)=delete;
	virtual void Initialize() override;
	WITextBase *GetBaseElement();
	const FontInfo *GetFont() const;
	uint32_t GetLineCount() const;
	uint32_t GetTotalLineCount() const;
	const std::vector<LineInfo> &GetLines() const;
	std::vector<LineInfo> &GetLines();
	util::text::FormattedTextLine *GetLine(util::text::LineIndex lineIdx);
	int GetLineHeight() const;
	int GetBreakHeight();
	void SetBreakHeight(int breakHeight);
	const util::text::FormattedText &GetFormattedTextObject() const;
	util::text::FormattedText &GetFormattedTextObject();
	const std::string &GetText() const;
	const std::string &GetFormattedText() const;
	void SetText(const std::string_view &text);
	void SetFont(const std::string_view &font);
	void SetFont(const FontInfo *font);
	void SetAutoBreakMode(AutoBreak b);
	AutoBreak GetAutoBreakMode() const;
	using WIBase::SetSize;
	virtual void SetSize(int x,int y) override;
	int GetTextHeight();
	std::shared_ptr<prosper::Texture> GetTexture() const;
	virtual std::string GetDebugInfo() const override;
	std::pair<Vector2i,Vector2i> GetCharacterPixelBounds(util::text::LineIndex lineIdx,util::text::CharOffset charOffset) const;
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const std::string &arg);
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Vector4 &arg);
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const CallbackHandle &arg);
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Color &arg);
	void SetTagsEnabled(bool bEnabled);
	bool AreTagsEnabled() const;
	Color GetCharColor(util::text::TextOffset offset) const;

	void AppendText(const std::string_view &text);
	bool InsertText(const std::string_view &text,util::text::LineIndex lineIdx,util::text::CharOffset charOffset=util::text::LAST_CHAR);
	void AppendLine(const std::string_view &line);
	void PopFrontLine();
	void PopBackLine();
	void RemoveLine(util::text::LineIndex lineIdx);
	bool RemoveText(util::text::LineIndex lineIdx,util::text::CharOffset charOffset,util::text::TextLength len);
	bool RemoveText(util::text::TextOffset offset,util::text::TextLength len);
	bool MoveText(util::text::LineIndex lineIdx,util::text::CharOffset startOffset,util::text::TextLength len,util::text::LineIndex targetLineIdx,util::text::CharOffset targetCharOffset=util::text::LAST_CHAR);
	std::string Substr(util::text::TextOffset startOffset,util::text::TextLength len) const;
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
	virtual void SetShadowOffset(int x,int y);
	Vector2i *GetShadowOffset();
	void SetShadowColor(Vector4 col);
	void SetShadowColor(const Color &col);
	virtual void SetShadowAlpha(float alpha);
	float GetShadowAlpha();
	Vector4 *GetShadowColor();
	virtual void SetShadowColor(float r,float g,float b,float a=1.f);
	virtual void SetShadowBlurSize(float size);
	float GetShadowBlurSize();
	//

	virtual void SelectShader();
	virtual void Think() override;
	virtual void SizeToContents(bool x=true,bool y=true) override;
	virtual Mat4 GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) const override;

	void SetCacheEnabled(bool bEnabled);
	bool IsCacheEnabled() const;

	template<class TDecorator,typename... TARGS>
		std::shared_ptr<WITextDecorator> AddDecorator(TARGS&& ...args);
	void RemoveDecorator(const WITextDecorator &decorator);

	static void InitializeTextBuffer(prosper::Context &context);
	static void ClearTextBuffer();
private:
	struct WITextShadowInfo
	{
		WITextShadowInfo()
			: enabled(false),blurSize(0.f)
		{}
		Vector2i offset;
		bool enabled;
		Vector4 color;
		float blurSize;
	};
private:
	static std::shared_ptr<prosper::UniformResizableBuffer> s_textBuffer;
	util::WeakHandle<prosper::Shader> m_shader = {};
	std::shared_ptr<util::text::FormattedText> m_text = nullptr;
	std::vector<LineInfo> m_lineInfos = {};

	std::vector<std::shared_ptr<WITextDecorator>> m_tagInfos = {};
	std::unordered_map<std::string,std::vector<std::weak_ptr<WITextTag>>> m_labelToDecorators = {};

	std::unordered_map<std::string,std::unordered_map<uint32_t,WITextTagArgument>> m_tagArgumentOverrides = {};
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

	bool BreakLineByWidth(uint32_t lineIndex,util::text::ShiftOffset &lineShift);
	void UpdateSubLines();
	
	void PerformTextPostProcessing();
	void AutoSizeToText();

	void InitializeTagArguments(const WITextTag &tag,std::vector<WITextTagArgument> &args) const;
	void SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const WITextTagArgument &arg);
	void ApplySubTextTags();
	void ApplySubTextTag(WITextDecorator &tag);
	void InitializeTextBuffers();
	void InitializeTextBuffers(LineInfo &lineInfo,util::text::LineIndex lineIndex);
	void UpdateRenderTexture();
	void GetTextSize(int *w,int *h,const std::string_view *inText=nullptr);
	void RenderText();
	void RenderText(Mat4 &mat);
	void InitializeShadow(bool bReload=false);
	void DestroyShadow();
	void InitializeBlur(bool bReload=false);
	void DestroyBlur();
	void ScheduleRenderUpdate(bool bFull=false);
};
REGISTER_BASIC_BITWISE_OPERATORS(WIText::Flags)

template<class TDecorator,typename... TARGS>
	std::shared_ptr<WITextDecorator> WIText::AddDecorator(TARGS&& ...args)
{
	auto pDecorator = std::make_shared<TDecorator>(*this,std::forward<TARGS>(args)...);
	if constexpr(std::is_base_of_v<WITextTag,TDecorator>)
	{
		InitializeTagArguments(*pDecorator,pDecorator->GetArguments());
	}
	pDecorator->Initialize();
	auto startOffset = pDecorator->GetStartTextCharOffset();
	auto it = std::find_if(m_tagInfos.begin(),m_tagInfos.end(),[startOffset](const std::shared_ptr<WITextDecorator> &decoratorOther) {
		return decoratorOther->GetStartTextCharOffset() >= startOffset;
	});
	m_tagInfos.insert(it,pDecorator);
	pDecorator->SetDirty();
	m_flags |= Flags::ApplySubTextTags;
	return pDecorator;
}

#endif
