/*
 Copyright (C) 2012-2015 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "cinder/Channel.h"
#include "cinder/FileSystem.h"
#include "cinder/Function.h"
#include "cinder/Vector.h"
#include "cinder/gl/Texture.h"

#include "BlobTracker.h"

namespace mndl { namespace blobtracker {

class DebugDrawer
{
 public:
	struct Options
	{
	 public:
		Options() {}

		enum class DebugMode : int
		{
			NONE = 0,
			BLENDED,
			OVERWRITE
		};
		DebugMode mDebugMode = DebugMode::NONE;

		enum class DrawMode : int
		{
			ORIGINAL = 0,
			BLURRED,
			THRESHOLDED
		};

		DrawMode mDrawMode = DrawMode::THRESHOLDED;
		bool mDrawProportionalFit = false;
	};

	static void draw( const BlobTrackerRef &blobTracker, const ci::Area &bounds, const Options &options = Options() );
};

} } // namespace mndl::blobtracker

