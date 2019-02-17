/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIBUFFERBASE_H__
#define __WIBUFFERBASE_H__

#include <sharedutils/util_weak_handle.hpp>
#include <config.h>
#include <wrappers/buffer.h>
#include "wielementbufferdata.hpp"

namespace prosper
{
	class Shader;
};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WIBufferBase
	: public WIBase
{
public:
	virtual ~WIBufferBase() override;
	virtual unsigned int GetVertexCount();
	virtual void Render(int w,int h,const Mat4 &mat,const Vector2i &origin,const Mat4 &matParent) override;
protected:
	WIBufferBase();

	virtual void SetShader(prosper::Shader &shader,prosper::Shader *shaderCheap=nullptr);
	prosper::Shader *GetShader();
	prosper::Shader *GetCheapShader();

	void InitializeBufferData(prosper::Buffer &buffer);

	// If this isn't set, the standard square vertex buffer (with optimized shaders) will be used instead
	std::unique_ptr<WIElementVertexBufferData> m_vertexBufferData = nullptr;

	util::WeakHandle<prosper::Shader> m_shader = {};
	util::WeakHandle<prosper::Shader> m_shaderCheap = {};
};
#pragma warning(pop)

#endif
