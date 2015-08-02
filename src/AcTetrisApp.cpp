#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

#include "pocket_tetris.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class AcTetrisApp : public AppNative {
public:
	void setup();
	void mouseDown( MouseEvent event );
    void mouseMove( MouseEvent event );
	void update();
	void draw();
private:
    PocketTetris _tetris;
};

void AcTetrisApp::setup()
{
    setWindowSize(480, 600);
    setFrameRate(60);
    if(!_tetris.init()) {
        console() << "Failed to init game." << endl;
    }
}

void AcTetrisApp::mouseDown( MouseEvent event )
{
    _tetris.mouseDown(event.isLeft()?1:0 | event.isRight()?2:0 | event.isMiddle()?3:0);
}

void AcTetrisApp::mouseMove( MouseEvent event )
{
    _tetris.mouseMove(event.getX(), event.getY());
}

void AcTetrisApp::update()
{
    _tetris.update();
}

void AcTetrisApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0.2, 0.2, 0.3 ) );
    
    _tetris.draw();
}

CINDER_APP_NATIVE( AcTetrisApp, RendererGl )
