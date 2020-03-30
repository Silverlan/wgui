/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHAPE_H__
#define __WISHAPE_H__
#include "wgui/wibase.h"
#include <materialmanager.h>
#include "wgui/wibufferbase.h"
#include "wiline.h"
#include <texturemanager/texture.h>

class DLLWGUI WIShape
	: public WIBufferBase
{
public:
	WIShape();
	~WIShape() override = default;
	virtual unsigned int AddVertex(Vector2 vert);
	void SetVertexPos(unsigned int vertID,Vector2 pos);
	void InvertVertexPositions(bool x=true,bool y=true);
	virtual void ClearVertices();
	virtual unsigned int GetVertexCount() override;
protected:
	virtual void DoUpdate() override;
	std::vector<Vector2> m_vertices;
	uint8_t m_vertexBufferUpdateRequired;
};

class DLLWGUI WIOutlinedShape
	: public WIShape,WILineBase
{
public:
	WIOutlinedShape();
	virtual ~WIOutlinedShape() override = default;
};

namespace prosper {class DescriptorSetGroup;};
class WGUIShaderTextured;
class DLLWGUI WITexturedShape
	: public WIShape
{
private:
	util::WeakHandle<prosper::Shader> m_shader = {};
	std::shared_ptr<prosper::DescriptorSetGroup> m_descSetTextureGroup = nullptr;
	void ClearTextureLoadCallback();
	void InitializeTextureLoadCallback(const std::shared_ptr<Texture> &texture);
protected:
	virtual void DoUpdate() override;
	MaterialHandle m_hMaterial;
	std::shared_ptr<prosper::Texture> m_texture = nullptr;
	std::shared_ptr<bool> m_texLoadCallback;
	std::shared_ptr<prosper::Buffer> m_uvBuffer = nullptr;
	std::vector<Vector2> m_uvs;
	bool m_bAlphaOnly;
	float m_lod = -1.f;

	void ReloadDescriptorSet();
	virtual void SetShader(prosper::Shader &shader,prosper::Shader *shaderCheap=nullptr) override;
public:
	WITexturedShape();
	virtual ~WITexturedShape() override;
	virtual unsigned int AddVertex(Vector2 vert) override;
	unsigned int AddVertex(Vector2 vert,Vector2 uv);
	void SetVertexUVCoord(unsigned int vertID,Vector2 uv);
	void InvertVertexUVCoordinates(bool x=true,bool y=true);
	virtual void ClearVertices() override;
	prosper::Buffer &GetUVBuffer() const;
	void SetUVBuffer(prosper::Buffer &buffer);
	void SetAlphaOnly(bool b);
	bool GetAlphaOnly() const;
	float GetLOD() const;
	void SetLOD(float lod);
	void SetMaterial(Material *material);
	void SetMaterial(const std::string &material);
	Material *GetMaterial();
	void SetTexture(prosper::Texture &tex,uint32_t layerIndex=0u);
	void ClearTexture();
	const std::shared_ptr<prosper::Texture> &GetTexture() const;
	virtual void Render(const DrawInfo &drawInfo,const Mat4 &matDraw) override;
	void SizeToTexture();
};

#endif
