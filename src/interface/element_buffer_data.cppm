// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"

export module pragma.gui:element_buffer_data;

export import pragma.prosper;

export struct DLLWGUI WIElementVertexBufferData {
  public:
	WIElementVertexBufferData() = default;
	virtual ~WIElementVertexBufferData();
	std::shared_ptr<prosper::IBuffer> GetBuffer() const;
	void SetBuffer(const std::shared_ptr<prosper::IBuffer> &buffer);
	void ClearBuffer();
  private:
	std::shared_ptr<prosper::IBuffer> m_buffer = nullptr;
};
