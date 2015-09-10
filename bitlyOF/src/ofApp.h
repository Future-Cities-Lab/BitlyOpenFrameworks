#pragma once

#include "ofMain.h"
#include "ofxUDPManager.h"
#include "ofxOsc.h"

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

private:
    ofMesh bitlyMesh;
    ofMesh movieMesh;
    ofMesh testMesh;


    int currentHour = ofGetHours();
    int previousHour = currentHour - 1;

    float globalCounter = 0.0f;

    ofColor globalLeft = ofColor(255.0, 255.0, 0.0);
    ofColor globalRight = ofColor(0.0, 0.0, 255.0);
    float leftPercent = 1.0;
    float rightPercent = 1.0;
    float finalAlpha = 0.5f;
    
    ofxUDPManager udpConnection;
    ofxOscReceiver oscReceiver;
    ofSerial serial;
    int baud = 57600;
    
    float noiseColumns[90];
    float noiseGrow = 10.0f;
    float noiseGrowChange = 1.0f;

    char msgs[20][3*324 + 2];
        
    int bitlyData[24][5][2];
    int bitlyDataNext[24][5][2];
    int regionStartBitly[24][5];
    int regionLengthsBitly[24][5];
    int minB = 100000;
    int maxB = -100000;
    int minNewB = 100000;
    int maxNewB = -100000;
    int *minBitly = &minB;
    int *maxBitly = &maxB;
    int *minNewBitly = &minNewB;
    int *maxNewBitly = &maxNewB;
    
    bool timeToUpdate = false;
    
    vector <ofVideoPlayer> movies;
    int moviePos = 0;
    int numMovies = 1;
    
    ofColor testColors[20] = {  ofColor(255, 0, 0), ofColor(0, 255, 0),
                                ofColor(0, 0, 255), ofColor(139, 0, 139),
                                ofColor(255, 165, 0), ofColor(255, 255, 0),
                                ofColor(255, 255, 255), ofColor(165, 42, 42),
                                ofColor(255, 0, 255), ofColor(255, 192, 203),
                                ofColor(46, 139, 87), ofColor(0, 255, 255),
                                ofColor(135, 206, 235), ofColor(240, 230, 140),
                                ofColor(128, 0, 0), ofColor(105, 105, 105),
                                ofColor(124, 252, 0), ofColor(255, 218, 185),
                                ofColor(0, 0, 128), ofColor(255, 215, 0)};

    
    enum VISUALIZATION { BITLY, MOVIE, TEST };
    enum VISUALIZATION state = BITLY;

    void loadGeometry(ofMesh *mesh);
    void downloadBitlyData(int data[24][5][2]);
    
    void setAllVerticesToColor(ofColor color, ofMesh *mesh);
    
    void drawLeadingEdge(int start, int end, float xNoiseColumn[], int hour, float length, ofMesh *mesh);
    void drawAlphaGradient(int start, float alpha, ofMesh *mesh);
    void drawRegions(int regionStart[24][5], int regionLength[24][5], ofMesh *mesh);
    void gradiateRegions(int data[24][5][2], int nextData[24][5][2], int (*regionStart)[5], int (*regionLength)[5], float counter);
    void updateNoiseValues();
    void updateGlobalCounter();
    void updateRegions(const int data[24][5][2], int (*regionStart)[5], int (*regionLength)[5]);
    void applyFinalAlpha(ofMesh *mesh);

    
    void sendMeshColorsToHardware(ofMesh mesh);
    void drawMeshOnScreen(int numColumns, int numRows, ofMesh mesh);
    
    void findMinMaxForData(const int data[24][5][2], int *min, int * max);

    float nonLinMap(float in, float inMin, float inMax, float outMin, float outMax, float shaper);
    void updateState(enum VISUALIZATION *state);

};


