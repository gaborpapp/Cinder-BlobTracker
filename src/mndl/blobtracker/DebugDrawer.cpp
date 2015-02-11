#include "cinder/Utilities.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"

#include "mndl/blobtracker/DebugDrawer.h"

using namespace ci;

namespace mndl { namespace blobtracker {

void DebugDrawer::draw( const BlobTrackerRef &blobTracker, const Area &bounds, const Options &options )
{
	if ( options.mDebugMode == Options::DebugMode::NONE )
	{
		return;
	}

	gl::TextureRef tex;
	switch ( options.mDrawMode )
	{
		case Options::DrawMode::ORIGINAL:
		{
			cv::Mat img = blobTracker->getImageInput();
			if ( img.data )
			{
				tex = gl::Texture::create( fromOcv( img ) );
			}
			break;
		}

		case Options::DrawMode::BLURRED:
		{
			cv::Mat img = blobTracker->getImageBlurred();
			if ( img.data )
			{
				tex = gl::Texture::create( fromOcv( img ) );
			}
			break;
		}

		case Options::DrawMode::THRESHOLDED:
		{
			 cv::Mat img = blobTracker->getImageThresholded();
			if ( img.data )
			{
				tex = gl::Texture::create( fromOcv( img ) );
			}
			break;
		}

		default:
			break;
	}

	if ( ! tex )
	{
		return;
	}

	ci::gl::disableDepthRead();
	ci::gl::disableDepthWrite();

	bool blendingEnabled = options.mDebugMode == Options::DebugMode::BLENDED;
	auto ctx = gl::context();
	ctx->pushBoolState( GL_BLEND, blendingEnabled );
	ctx->pushBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	gl::ScopedColor color( ColorA::gray( 1.0f, 0.5f ) );

	Area outputArea = bounds;
	if ( options.mDrawProportionalFit )
	{
		outputArea = Area::proportionalFit( tex->getBounds(), bounds, true, true );
	}

	gl::draw( tex, outputArea );

	{
		const auto &trackerOptions = blobTracker->getOptions();
		float s = trackerOptions.mNormalizationScale;
		RectMapping blobMapping( Rectf( 0.0f, 0.0f, s, s ), Rectf( outputArea ) );
		Rectf roi = trackerOptions.mNormalizedRegionOfInterest * s;
		gl::color( ColorA( 0.0f, 1.0f, 0.0f, 0.8f ) );
		gl::drawStrokedRect( blobMapping.map( roi ) );

		vec2 offset = outputArea.getUL();
		vec2 scale = vec2( outputArea.getSize() ) / vec2( s, s );
		for ( const auto &blob : blobTracker->getBlobs() )
		{
			gl::pushModelView();
			gl::translate( offset );
			gl::scale( scale );
			if ( trackerOptions.mBoundsEnabled )
			{
				gl::color( ColorA( 1.0f, 1.0f, 0.0f, 0.5f ) );
				gl::drawStrokedRect( blob->mBounds );
			}
			if ( trackerOptions.mConvexHullEnabled && blob->mConvexHull )
			{
				gl::color( ColorA( 1.0f, 0.0f, 1.0f, 0.5f ) );
				gl::draw( *blob->mConvexHull.get() );
			}
			gl::popModelView();
			vec2 pos = blobMapping.map( blob->mPos );
			gl::drawSolidCircle( pos, 2.0f );
			gl::drawString( toString< int32_t >( blob->mId ), pos + vec2( 3.0f, -3.0f ),
					ColorA( 1.0f, 0.0f, 0.0f, 0.9f ) );
		}
	}
	ctx->popBoolState( GL_BLEND );
	ctx->popBlendFuncSeparate();
}

} } // mndl::blobtracker
