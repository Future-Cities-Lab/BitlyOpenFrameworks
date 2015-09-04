#pragma once

#include "ofMain.h"
#include "ofxUDPManager.h"


class ofApp : public ofBaseApp {
    
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void exit();
    
    // VIDEO
    ofVideoGrabber 		vidGrabber;
    unsigned char * 	videoInverted;
    ofTexture			videoTexture;
    int 				camWidth;
    int 				camHeight;

    // SUPER VIDEO
    ofCamera cam; // add mouse controls for camera movement
    float extrusionAmount;
    ofVboMesh mainMesh;
    
    // VIDEO PLAYER
    ofVideoPlayer 		fingerMovie;
    bool                frameByframe;

    //PUFF-MAN
    ofImage puffImage;

};


