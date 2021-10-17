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

 Thanks to Community Core Vision, http://nuicode.com/projects/tbeta
 and Patricio Gonzalez Vivo for ofxBlobTracker,
 https://github.com/patriciogonzalezvivo/ofxBlobTracker
*/
#include <list>

#include "cinder/Area.h"
#include "cinder/Rect.h"
#include "cinder/app/App.h"
#include "cinder/ip/Fill.h"

#include "mndl/blobtracker/BlobTracker.h"

using namespace ci;
using namespace std;

namespace mndl { namespace blobtracker {

BlobTracker::BlobTracker( const Options &options ) :
	mIdCounter( 1 ),
	mOptions( options )
{}

void BlobTracker::update( const Channel8u &inputChannel )
{
	cv::Mat input( toOcv( inputChannel ) );
	if ( mOptions.mFlip )
	{
		cv::flip( input, input, 1 );
	}
	if ( mOptions.mBlankOutsideRoi )
	{
		int w = inputChannel.getWidth();
		int h = inputChannel.getHeight();
		Channel8u src( fromOcv( input ) );
		Area insideArea( mOptions.mNormalizedRegionOfInterest.scaled( inputChannel.getSize() ) );

		uint8_t fillColor = mOptions.mThresholdInvertEnabled ? 255 : 0;
		ci::ip::fill( &src, fillColor, Area( ivec2( 0, 0 ),
										  ivec2( w, insideArea.y1 ) ) );
		ci::ip::fill( &src, fillColor, Area( ivec2( 0, insideArea.y1 ),
										  ivec2( insideArea.x1, insideArea.y2 ) ) );
		ci::ip::fill( &src, fillColor, Area( ivec2( 0, insideArea.y2 ),
										  ivec2( w, h ) ) );
		ci::ip::fill( &src, fillColor, Area( ivec2( insideArea.x2, insideArea.y1 ),
										  ivec2( w, insideArea.y2 ) ) );
		input = toOcv( src );
	}

	mInput = input.clone();

	cv::Mat thresholded;
	cv::blur( input, mBlurred, cv::Size( mOptions.mBlurSize, mOptions.mBlurSize ) );
	cv::threshold( mBlurred, thresholded, mOptions.mThreshold, 255,
			mOptions.mThresholdInvertEnabled ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY );
	mThresholded = thresholded.clone();

	vector< vector< cv::Point > > contours;
	cv::findContours( thresholded, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE );

	float surfArea = inputChannel.getWidth() * inputChannel.getHeight();
	float minAreaLimit = surfArea * mOptions.mMinArea;
	float maxAreaLimit = surfArea * mOptions.mMaxArea;

	// normalizes blob coordinates from camera 2d coords to [0, mOptions.mNormalizationScale]
	ci::RectMapping normMapping( Rectf( inputChannel.getBounds() ),
								 Rectf( 0.f, 0.f, mOptions.mNormalizationScale, mOptions.mNormalizationScale ) );
	ci::Rectf roi = mOptions.mNormalizedRegionOfInterest * mOptions.mNormalizationScale;
	vector< BlobRef > newBlobs;
	for ( vector< cv::Point > contourPnts : contours )
	{
		BlobRef b = Blob::create();
		cv::Mat pmat = cv::Mat( contourPnts );
		cv::Rect cvRect = cv::boundingRect( pmat );
		float area = cvRect.width * cvRect.height;
		if ( ( minAreaLimit <= area ) && ( area < maxAreaLimit ) )
		{
			cv::Moments m = cv::moments( pmat );
			b->mPos = vec2( m.m10 / m.m00, m.m01 / m.m00 );
			b->mPos = b->mPrevPos = normMapping.map( b->mPos );
			if ( ! roi.contains( b->mPos ) )
			{
				continue;
			}

			if ( mOptions.mBoundsEnabled )
			{
				b->mBounds = normMapping.map( Rectf( cvRect.x, cvRect.y,
													 cvRect.x + cvRect.width, cvRect.y + cvRect.height ) );
			}

			if ( mOptions.mConvexHullEnabled )
			{
				vector< cv::Point > cvHull;
				cv::convexHull( contourPnts, cvHull );
				b->mConvexHull = std::make_shared< PolyLine2f >();
				for ( const cv::Point &pt : cvHull )
				{
					b->mConvexHull->push_back( normMapping.map( fromOcv( pt ) ) );
				}
				b->mConvexHull->setClosed();
			}
			newBlobs.push_back( b );
		}
	}

	trackBlobs( newBlobs );
}

void BlobTracker::trackBlobs( vector< BlobRef > newBlobs )
{
	// all new blob id's initialized with -1

	// step 1: match new blobs with existing nearest ones
	for ( size_t i = 0; i < mBlobs.size(); i++ )
	{
		int32_t winner = findClosestBlobKnn( newBlobs, mBlobs[ i ], 3, 0 );

		if ( winner == -1 ) // track has died
		{
			mBlobsEndedSig.emit( BlobEvent( mBlobs[ i ] ) );
			mBlobs[ i ]->mId = -1; // marked for deletion
		}
		else
		{
			// if winning new blob was labeled winner by another track
			// then compare with this track to see which is closer
			if ( newBlobs[ winner ]->mId != -1 )
			{
				// find the currently assigned blob
				int j; // j will be the index of it
				for ( j = 0; j < mBlobs.size(); j++ )
				{
					if ( mBlobs[ j ]->mId == newBlobs[ winner ]->mId )
					{
						break;
					}
				}

				if ( j == mBlobs.size() ) // got to end without finding it
				{
					newBlobs[ winner ]->mId = mBlobs[ i ]->mId;
					mBlobs[ i ] = newBlobs[ winner ];
				}
				else // found it, compare with current blob
				{
					vec2 p = newBlobs[ winner ]->mPos;
					vec2 pOld = mBlobs[ j ]->mPos;
					vec2 pNew = mBlobs[ i ]->mPos;
					// todo squaredistance calculate would be better
					float distOld = glm::distance( p, pOld );
					float distNew = glm::distance( p, pNew );

					// if this track is closer, update the Id of the blob
					// otherwise delete this track.. it's dead
					if ( distNew < distOld ) // update
					{
						newBlobs[ winner ]->mId = mBlobs[ i ]->mId;
						/* TODO
						   now the old winning blob has lost the win.
						   I should also probably go through all the newBlobs
						   at the end of this loop and if there are ones without
						   any winning matches, check if they are close to this
						   one. Right now I'm not doing that to prevent a
						   recursive mess. It'll just be a new track.
						 */
						mBlobsEndedSig.emit( BlobEvent( mBlobs[ j ] ) );
						// mark the blob for deletion
						mBlobs[ j ]->mId = -1;
					}
					else // delete
					{
						mBlobsEndedSig.emit( BlobEvent( mBlobs[ i ] ) );
						// mark the blob for deletion
						mBlobs[ i ]->mId = -1;
					}
				}
			}
			else // no conflicts, so simply update
			{
				newBlobs[ winner ]->mId = mBlobs[ i ]->mId;
			}
		}
	}

	// step 2: blob update
	//
	// update all current tracks
	// remove every track labeled as dead, id = -1
	// find every track that's alive and copy its data from newBlobs
	for ( size_t i = 0; i < mBlobs.size(); i++ )
	{
		if ( mBlobs[ i ]->mId == -1 ) // dead
		{
			// erase track
			mBlobs.erase( mBlobs.begin() + i, mBlobs.begin() + i + 1 );
			i--; // decrement one since we removed an element
		}
		else // living, so update its data
		{
			for ( int j = 0; j < newBlobs.size(); j++ )
			{
				if ( mBlobs[ i ]->mId == newBlobs[ j ]->mId )
				{
					// update track
					// store the last centroid
					newBlobs[ j ]->mPrevPos = mBlobs[ i ]->mPos;
					mBlobs[ i ] = newBlobs[ j ];

					vec2 tD = mBlobs[ i ]->mPos - mBlobs[ i ]->mPrevPos;

					// calculate the acceleration
					float posDelta = glm::length( tD );
					if ( posDelta > 0.001f )
					{
						mBlobsMovedSig.emit( BlobEvent( mBlobs[ i ] ) );
					}

					// TODO: add other blob features
				}
			}
		}
	}

	// step 3: add tracked blobs to touchevents
	// -- add new living tracks
	// now every new blob should be either labeled with a tracked id or
	// have id of -1. if the id is -1, we need to make a new track.
	for ( size_t i = 0; i < newBlobs.size(); i++ )
	{
		if ( newBlobs[ i ]->mId == -1 )
		{
			// add new track
			newBlobs[ i ]->mId = mIdCounter;
			mIdCounter++;

			mBlobs.push_back( newBlobs[ i ] );

			mBlobsBeganSig.emit( BlobEvent( newBlobs[ i ] ) );
		}
	}
}

/** Finds the blob in newBlobs that is closest to the blob \a track.
 * \param newBlobs list of blobs detected in the last frame
 * \param track current blob
 * \param k number of nearest neighbours, must be odd number (1, 3, 5 are common)
 * \param thres optimization threshold
 * Returns the closest blob id if found or -1
 */
int32_t BlobTracker::findClosestBlobKnn( const vector< BlobRef > &newBlobs, BlobRef track,
		int k, double thresh )
{
	int32_t winner = -1;
	if ( thresh > 0. )
		thresh *= thresh;

	// list of neighbour point index and respective distances
	list< pair< size_t, double > > nbors;
	list< pair< size_t, double > >::iterator iter;

	// find 'k' closest neighbors of testpoint
	vec2 p;
	vec2 pT = track->mPos;
	float distSquared;

	// search for blobs
	for ( size_t i = 0; i < newBlobs.size(); i++ )
	{
		p = newBlobs[ i ]->mPos;
		// todo squaredistance calculate would be better
		distSquared = glm::distance( p, pT );

		if ( distSquared <= thresh )
		{
			winner = i;
			return winner;
		}

		// check if this blob is closer to the point than what we've seen
		// so far and add it to the index/distance list if positive

		// search the list for the first point with a longer distance
		for ( iter = nbors.begin(); iter != nbors.end() && distSquared >= iter->second; ++iter );

		if ( ( iter != nbors.end() ) || ( nbors.size() < k ) )
		{
			nbors.insert( iter, 1, std::pair< size_t, double >( i, distSquared ));
			// too many items in list, get rid of farthest neighbor
			if ( nbors.size() > k )
			{
				nbors.pop_back();
			}
		}
	}

	/********************************************************************
	 * we now have k nearest neighbors who cast a vote, and the majority
	 * wins. we use each class average distance to the target to break any
	 * possible ties.
	 *********************************************************************/

	// a mapping from labels (IDs) to count/distance
	map< int32_t, pair< size_t, double > > votes;

	// remember:
	// iter->first = index of newBlob
	// iter->second = distance of newBlob to current tracked blob
	for ( iter = nbors.begin(); iter != nbors.end(); ++iter )
	{
		// add up how many counts each neighbor got
		size_t count = ++( votes[ iter->first ].first );
		double dist = ( votes[ iter->first ].second += iter->second );

		// check for a possible tie and break with distance
		if ( ( count > votes[ winner ].first ) ||
				( ( count == votes[ winner ].first ) &&
				  ( dist < votes[ winner ].second ) ) )
		{
			winner = iter->first;
		}
	}

	return winner;
}

} } // namspace mndl::blobtracker
