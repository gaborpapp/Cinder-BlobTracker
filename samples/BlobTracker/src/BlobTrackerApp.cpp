#include <unordered_map>

#include "cinder/PolyLine.h"
#include "cinder/Rand.h"
#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/params/Params.h"
#include "cinder/qtime/QuickTime.h"

#include "mndl/blobtracker/BlobTracker.h"
#include "mndl/blobtracker/DebugDrawer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BlobTrackerApp : public AppNative
{
 public:
	void prepareSettings( Settings *settings ) override;
	void setup() override;
	void update() override;
	void draw() override;

	void keyDown( KeyEvent event ) override;

 private:
	params::InterfaceGlRef mParams;
	void setupParams();

	mndl::blobtracker::BlobTracker::Options mBlobTrackerOptions;
	mndl::blobtracker::BlobTrackerRef mBlobTracker;
	mndl::blobtracker::DebugDrawer::Options mDebugOptions;

	qtime::MovieSurfaceRef mMovie;

	float mFps;

	void blobsBegan( mndl::blobtracker::BlobEvent event );
	void blobsMoved( mndl::blobtracker::BlobEvent event );
	void blobsEnded( mndl::blobtracker::BlobEvent event );

	std::unordered_map< int32_t, PolyLine2 > mStrokes;

	void loadMovie( const fs::path &moviePath );
};

void BlobTrackerApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
}

void BlobTrackerApp::setup()
{
	disableFrameRate();

	mBlobTracker = mndl::blobtracker::BlobTracker::create( mBlobTrackerOptions );
	mBlobTracker->connectBlobCallbacks( &BlobTrackerApp::blobsBegan,
										&BlobTrackerApp::blobsMoved,
										&BlobTrackerApp::blobsEnded, this );

	setupParams();
}

void BlobTrackerApp::loadMovie( const fs::path &moviePath )
{
	mStrokes.clear();

	mMovie = qtime::MovieSurface::create( moviePath );
	mMovie->setLoop();
	mMovie->play();
}

void BlobTrackerApp::setupParams()
{
	mParams = params::InterfaceGl::create( "Parameters", ivec2( 200, 320 ) );

	mParams->addParam( "Fps", &mFps );
	mParams->addSeparator();

	mParams->addText( "Blob tracker" );
	mParams->addParam( "Flip", &mBlobTrackerOptions.mFlip );
	mParams->addParam( "Threshold", &mBlobTrackerOptions.mThreshold ).min( 0 ).max( 255 );
	mParams->addParam( "Threshold inverts", &mBlobTrackerOptions.mThresholdInvertEnabled );
	mParams->addParam( "Blur size", &mBlobTrackerOptions.mBlurSize ).min( 1 ).max( 15 );
	mParams->addParam( "Min area", &mBlobTrackerOptions.mMinArea ).min( 0.f ).max( 1.f ).step( 0.0001f );
	mParams->addParam( "Max area", &mBlobTrackerOptions.mMaxArea ).min( 0.f ).max( 1.f ).step( 0.001f );
	mParams->addParam( "Convex hull", &mBlobTrackerOptions.mConvexHullEnabled );
	mParams->addParam( "Bounds", &mBlobTrackerOptions.mBoundsEnabled );
	mParams->addParam( "Top left x", &mBlobTrackerOptions.mNormalizedRegionOfInterest.x1 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParams->addParam( "Top left y", &mBlobTrackerOptions.mNormalizedRegionOfInterest.y1 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParams->addParam( "Bottom right x", &mBlobTrackerOptions.mNormalizedRegionOfInterest.x2 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParams->addParam( "Bottom right y", &mBlobTrackerOptions.mNormalizedRegionOfInterest.y2 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParams->addParam( "Blank outside Roi", &mBlobTrackerOptions.mBlankOutsideRoi )
		.group( "Region of Interest" );
	mParams->setOptions( "Region of Interest", "opened=false" );
	mParams->addSeparator();

	mParams->addText( "Debug" );

	std::vector< std::string > debugModeNames = { "none", "blended", "overwrite" };
	mDebugOptions.mDebugMode = mndl::blobtracker::DebugDrawer::Options::DebugMode::BLENDED;
	mParams->addParam( "Debug mode", debugModeNames, reinterpret_cast< int * >( &mDebugOptions.mDebugMode ) );

	std::vector< std::string > drawModeNames = { "original", "blurred", "thresholded" };
	mDebugOptions.mDrawMode = mndl::blobtracker::DebugDrawer::Options::DrawMode::ORIGINAL;
	mParams->addParam( "Draw mode", drawModeNames, reinterpret_cast< int * >( &mDebugOptions.mDrawMode ) );
	mParams->addParam( "Draw proportional fit", &mDebugOptions.mDrawProportionalFit );
	mParams->addSeparator();

	mParams->addButton( "Load movie", [ & ]()
			{
				fs::path moviePath = app::getOpenFilePath();
				if ( fs::exists( moviePath ) )
				{
					loadMovie( moviePath );
				}
			} );
}

void BlobTrackerApp::update()
{
	mFps = getAverageFps();

	if ( mMovie && mMovie->checkNewFrame() )
	{
		mBlobTracker->update( Channel8u( *mMovie->getSurface() ) );
	}
}

void BlobTrackerApp::draw()
{
	gl::viewport( getWindowSize() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear();

	for ( auto &stroke : mStrokes )
	{
		Rand::randSeed( stroke.first );
		gl::color( Color( Rand::randFloat(), Rand::randFloat(), Rand::randFloat() ) );
		gl::draw( stroke.second );

		auto &points = stroke.second.getPoints();
		size_t size = points.size();
		if ( size > 64 )
		{
			size_t eraseNum = size - 64;
			points.erase( points.begin(), points.begin() + eraseNum );
		}
	}

	mndl::blobtracker::DebugDrawer::draw( mBlobTracker, getWindowBounds(), mDebugOptions );
	mParams->draw();
}

void BlobTrackerApp::blobsBegan( mndl::blobtracker::BlobEvent event )
{
	int32_t id = event.getId();
	mStrokes[ id ] = PolyLine2();
	mStrokes[ id ].push_back( event.getPos() * vec2( getWindowSize() ) );
}

void BlobTrackerApp::blobsMoved( mndl::blobtracker::BlobEvent event )
{
	int32_t id = event.getId();
	auto strokeIt = mStrokes.find( id );
	if ( strokeIt != mStrokes.end() )
	{
		strokeIt->second.push_back( event.getPos() * vec2( getWindowSize() ) );
	}
	else
	{
		blobsBegan( event );
	}
}

void BlobTrackerApp::blobsEnded( mndl::blobtracker::BlobEvent event )
{
	int32_t id = event.getId();
	mStrokes.erase( id );
}

void BlobTrackerApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_NATIVE( BlobTrackerApp, RendererGl )
