/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WITEXT_H__
#define __WITEXT_H__
#include "wgui/wibase.h"
#include "wgui/fontmanager.h"
#include "wgui/wihandle.h"
#include <image/prosper_render_target.hpp>
#include <sharedutils/property/util_property.hpp>

namespace prosper
{
	class Shader;
	class BlurSet;
};

class WGUIShaderText;
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WIText
	: public WIBase
{
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

		Cache = FullUpdateScheduled<<1u
	};

	WIText();
	virtual ~WIText() override;
	WIText(const WIText&)=delete;
	void operator=(const WIText&)=delete;
	virtual void Initialize() override;
	const FontInfo *GetFont() const;
	int GetLineCount();
	void GetLines(std::vector<std::string> **lines);
	std::string *GetLine(int line);
	int GetLineHeight();
	int GetBreakHeight();
	void SetBreakHeight(int breakHeight);
	const util::PStringProperty &GetTextProperty() const;
	const std::string &GetText() const;
	void SetText(std::string text);
	void SetFont(const std::string &font);
	void SetFont(const FontInfo *font);
	void SetAutoBreakMode(AutoBreak b);
	using WIBase::SetSize;
	virtual void SetSize(int x,int y) override;
	int GetTextHeight();
	std::shared_ptr<prosper::Texture> GetTexture() const;
	virtual std::string GetDebugInfo() const override;

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
	virtual void Render(int w,int h,const Mat4 &mat,const Vector2i &origin,const Mat4 &matParent) override;
	virtual void Think() override;
	virtual void Update() override;
	virtual void SizeToContents() override;
	virtual Mat4 GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) override;

	void SetCacheEnabled(bool bEnabled);
	bool IsCacheEnabled() const;

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
	struct TextBufferInfo
	{
		struct SubBufferInfo
		{
			std::shared_ptr<prosper::Buffer> buffer;
			size_t subStringHash;
			uint32_t numChars = 0u;
		};
		std::vector<SubBufferInfo> glyphInfoBufferInfos = {};
		std::string subString = {};
		uint32_t width = 0u;
		uint32_t height = 0u;
		uint32_t numChars = 0u;
		float sx = 0.f;
		float sy = 0.f;
	};
	TextBufferInfo m_textBufferInfo = {};
	util::WeakHandle<prosper::Shader> m_shader = {};
	util::PStringProperty m_text;
	std::vector<std::string> m_lines;
	WIHandle m_baseText;
	std::shared_ptr<const FontInfo> m_font;
	int m_breakHeight;
	AutoBreak m_autoBreak;

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

	void InitializeTextBuffers();
	void UpdateRenderTexture();
	void GetTextSize(int *w,int *h);
	void RenderText();
	void RenderText(Mat4 &mat);
	void InitializeShadow(bool bReload=false);
	void DestroyShadow();
	void InitializeBlur(bool bReload=false);
	void DestroyBlur();
	void ScheduleRenderUpdate(bool bFull=false);
};
REGISTER_BASIC_BITWISE_OPERATORS(WIText::Flags)
#pragma warning(pop)

#endif
