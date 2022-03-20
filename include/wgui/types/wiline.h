/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WILINE_H__
#define __WILINE_H__
#include "wgui/wibase.h"
#include "wgui/wibufferbase.h"
#include <sharedutils/property/util_property_vector.h>

class DLLWGUI WILineBase
{
private:
	unsigned int m_lineWidth;
protected:
	WILineBase();
public:
	unsigned int GetLineWidth();
	void SetLineWidth(unsigned int width);
};

class DLLWGUI WILine
	: public WIBufferBase,WILineBase
{
private:
	util::PVector2iProperty m_posStart = nullptr;
	util::PVector2iProperty m_posEnd = nullptr;
	Color m_colStart;
	Color m_colEnd;
	float m_dot;
	std::shared_ptr<prosper::IBuffer> m_bufColor = nullptr;
	void UpdateColorBuffer();
public:
	WILine();
	virtual ~WILine() override;
	virtual void SetColor(float r,float g,float b,float a=1.f) override;
	void SetLineWidth(unsigned int width);
	unsigned int GetLineWidth();
	std::pair<Vector2i,Vector2i> GetNormalizedLineBounds() const;

	const util::PVector2iProperty &GetStartPosProperty() const;
	const util::PVector2iProperty &GetEndPosProperty() const;

	Vector2i &GetStartPos() const;
	void SetStartPos(Vector2i pos);
	void SetStartPos(int x,int y);
	Vector2i &GetEndPos() const;
	void SetEndPos(Vector2i pos);
	void SetEndPos(int x,int y);
	void SetStartColor(const Color &col);
	void SetEndColor(const Color &col);
	const Color &GetStartColor() const;
	const Color &GetEndColor() const;
	virtual void SizeToContents(bool x=true,bool y=true) override;
	virtual unsigned int GetVertexCount() override;
	virtual void Render(const DrawInfo &drawInfo,wgui::DrawState &drawState,const Mat4 &matDraw,const Vector2 &scale={1.f,1.f},uint32_t testStencilLevel=0u,wgui::StencilPipeline stencilPipeline=wgui::StencilPipeline::Test) override;
	virtual Mat4 GetTransformPose(const Vector2i &origin,int w,int h,const Mat4 &poseParent,const Vector2 &scale={1.f,1.f}) const override;
};

#endif
