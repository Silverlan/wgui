/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIELEMENTBUFFERDATA_HPP__
#define __WIELEMENTBUFFERDATA_HPP__

#include "wguidefinitions.h"

struct DLLWGUI WIElementVertexBufferData
{
public:
	WIElementVertexBufferData()=default;
	virtual ~WIElementVertexBufferData();
	std::shared_ptr<prosper::Buffer> GetBuffer() const;
	void SetBuffer(const std::shared_ptr<prosper::Buffer> &buffer);
	void ClearBuffer();
private:
	std::shared_ptr<prosper::Buffer> m_buffer = nullptr;
};

#endif
