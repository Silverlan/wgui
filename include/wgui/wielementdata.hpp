/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WIELEMENTDATA_HPP__
#define __WIELEMENTDATA_HPP__

#include <mathutil/uvec.h>

namespace wgui
{
#pragma pack(push,1)
	struct ElementData
	{
		Mat4 modelMatrix;
		Vector4 color;
	};
#pragma pack(pop)
};

#endif
