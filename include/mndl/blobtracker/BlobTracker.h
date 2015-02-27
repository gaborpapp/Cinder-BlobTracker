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

#include <vector>

#include "cinder/params/Params.h"

#include "cinder/Channel.h"
#include "cinder/Function.h"
#include "cinder/Vector.h"

#include "CinderOpenCV.h"

#include "mndl/blobtracker/Blob.h"

namespace mndl { namespace blobtracker {

typedef std::shared_ptr< class BlobTracker > BlobTrackerRef;

class BlobTracker
{
 public:
	struct Options
	{
	 public:
		Options() {}

		//! Enables or disables the calculation of the blobs' bounding box.
		void enableBounds( bool enableBounds = true ) { mBoundsEnabled = enableBounds; }
		//! Enables or disables the calculation of the blobs' convex hull.
		void enableConvexHull( bool enableConvexHull = true ) { mConvexHullEnabled = enableConvexHull; }
		//! Sets normalization scale. All coordinates are normalized to [ 0, 0, \a normalizationScale, \a normalizationScale ]. 1.0 by default.
		void setNormalizationScale( float normalizationScale ) { mNormalizationScale = normalizationScale; }

		// Returns whether the calculation of the blobs' bounding box is enabled.
		bool isBoundsEnabled() const { return mBoundsEnabled; }
		// Returns whether the calculation of the blobs' convex hull is enabled.
		bool isConvexHullEnabled() const { return mConvexHullEnabled; }
		//! Returns normalization scale. All coordinates are normalized to [ 0, 0, normalizationScale, normalizationScale ]. 1.0 by default.
		float getNormalizationScale() const { return mNormalizationScale; }

		void setFlip( bool flip ) { mFlip = flip; }
		void setThreshold( int threshold ) { mThreshold = threshold; }
		void setBlurSize( int blurSize ) { mBlurSize = blurSize; }
		void setMinArea( float minArea ) { mMinArea = minArea; }
		void setMaxArea( float maxArea ) { mMaxArea = maxArea; }

		void setNormalizedRoi( const ci::Rectf &normalizedRoi )
		{ mNormalizedRegionOfInterest = normalizedRoi; }
		void enableBlankOutsideRoi( bool blankOutsideRoi = true )
		{ mBlankOutsideRoi = blankOutsideRoi; }

		//! If /a enableInvert is true the thresholding pass returns an inverted image.
		void enableThresholdInvert( bool enableInvert = true ) { mThresholdInvertEnabled = enableInvert; }
		//! Returns whether thresholding inverts the image.
		bool isThresholdInvert() const { return mThresholdInvertEnabled; }

		bool mBoundsEnabled = true;
		bool mConvexHullEnabled = false;
		float mNormalizationScale = 1.f;

		bool mFlip = false;
		int mThreshold = 150;
		int mBlurSize = 10;
		float mMinArea = 0.01f;
		float mMaxArea = 0.45f;
		ci::Rectf mNormalizedRegionOfInterest = ci::Rectf( 0.f, 0.f, 1.0f, 1.0f );
		bool mBlankOutsideRoi = false;
		bool mThresholdInvertEnabled = false;
	};

	static BlobTrackerRef create( const Options &options = Options() )
	{ return BlobTrackerRef( new BlobTracker( options ) ); }

	void update( const ci::Channel8u &inputChannel );

	typedef void( BlobCallback )( BlobEvent );
	typedef ci::signals::signal< BlobCallback > BlobSignal;

	template< typename T, typename Y >
	ci::signals::connection connectBlobsBegan( T fn, Y *inst )
	{ return mBlobsBeganSig.connect( std::bind( fn, inst, std::placeholders::_1 ) ); }

	template< typename T, typename Y >
	ci::signals::connection connectBlobsMoved( T fn, Y *inst )
	{ return mBlobsMovedSig.connect( std::bind( fn, inst, std::placeholders::_1 ) ); }

	template< typename T, typename Y >
	ci::signals::connection connectBlobsEnded( T fn, Y *inst )
	{ return mBlobsEndedSig.connect( std::bind( fn, inst, std::placeholders::_1 ) ); }

	template< typename T, typename Y >
	void connectBlobCallbacks( T fnBegan, T fnMoved, T fnEnded, Y *inst )
	{
		connectBlobsBegan( fnBegan, inst );
		connectBlobsMoved( fnMoved, inst );
		connectBlobsEnded( fnEnded, inst );
	}

	void disconnectBlobCallbacks()
	{
		mBlobsBeganSig.disconnect_all_slots();
		mBlobsMovedSig.disconnect_all_slots();
		mBlobsEndedSig.disconnect_all_slots();
	}

	void reset() { mBlobs.clear(); }

	const Options &getOptions() const { return mOptions; }

	cv::Mat getImageInput() const { return mInput; }
	cv::Mat getImageBlurred() const { return mBlurred; }
	cv::Mat getImageThresholded() const { return mThresholded; }

	size_t getNumBlobs() const { return mBlobs.size(); }
	const std::vector< BlobRef > & getBlobs() const { return mBlobs; }

 protected:
	BlobTracker( const Options &options );

	const Options &mOptions;

	std::vector< BlobRef > mBlobs;
	void trackBlobs( std::vector< BlobRef > newBlobs );
	int32_t findClosestBlobKnn( const std::vector< BlobRef > &newBlobs,
			BlobRef track, int k, double thresh );
	int32_t mIdCounter;

	// signals
	BlobSignal mBlobsBeganSig;
	BlobSignal mBlobsMovedSig;
	BlobSignal mBlobsEndedSig;

	cv::Mat mInput;
	cv::Mat mBlurred;
	cv::Mat mThresholded;
};

} } // namespace mndl::blobtracker
