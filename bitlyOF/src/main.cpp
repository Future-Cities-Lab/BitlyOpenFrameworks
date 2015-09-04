#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
    int boxDiff = 50;
    ofSetupOpenGL(640+(2*boxDiff), 720+(2*boxDiff), OF_WINDOW);
	ofRunApp(new ofApp());

}
