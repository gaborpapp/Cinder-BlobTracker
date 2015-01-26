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

#include <memory>

#include "cinder/PolyLine.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

namespace mndl { namespace blobtracker {

typedef std::shared_ptr< struct Blob > BlobRef;

struct Blob
{
 public:
	static BlobRef create() { return BlobRef( new Blob() ); }

	int32_t mId;
	ci::Rectf mBounds;
	ci::vec2 mPos;
	ci::vec2 mPrevPos;
	std::shared_ptr< ci::PolyLine2f > mConvexHull;

 private:
	Blob() : mId( -1 ) {}
};

//! Represents a blob event
class BlobEvent
{
 public:
	BlobEvent( BlobRef blobRef ) : mBlobRef( blobRef ) {}

	//! Returns an ID unique for the lifetime of the blob.
	int32_t getId() const { return mBlobRef->mId; }
	//! Returns the position of the blob centroid normalized to the image resolution.
	ci::vec2 getPos() const { return mBlobRef->mPos; }
	//! Returns the previous position of the blob centroid normalized to the image resolution.
	ci::vec2 getPrevPos() const { return mBlobRef->mPrevPos; }
	//! Returns the bounding box of the blob.
	ci::Rectf & getBounds() const { return mBlobRef->mBounds; }
	//! Returns the blob object reference.
	const BlobRef & getBlob() const { return mBlobRef; }

private:
	BlobRef mBlobRef;
};

} } // namespace mndl::blobtracker

