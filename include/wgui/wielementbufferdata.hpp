// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WIELEMENTBUFFERDATA_HPP__
#define __WIELEMENTBUFFERDATA_HPP__

#include "wguidefinitions.h"

struct DLLWGUI WIElementVertexBufferData {
  public:
	WIElementVertexBufferData() = default;
	virtual ~WIElementVertexBufferData();
	std::shared_ptr<prosper::IBuffer> GetBuffer() const;
	void SetBuffer(const std::shared_ptr<prosper::IBuffer> &buffer);
	void ClearBuffer();
  private:
	std::shared_ptr<prosper::IBuffer> m_buffer = nullptr;
};

#endif
