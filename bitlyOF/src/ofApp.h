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

    int currentHour = ofGetHours();
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

    
    float xNoiseColumn1[17] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0};
    float xNoiseColumn2[17] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0};
    float xNoiseColumn3[18] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0};
    float xNoiseColumn4[17] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0};
    float xNoiseColumn5[17] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
        11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 0.0};
    
    float xGrowChange[5] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    float xGrow[5] = {40.0f, 10.0f, 5.0f, 20.0f, 15.0f};

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
    
    float lastTime = 0.0f;
    float waitTime = 30.0f;
    
    vector <ofVideoPlayer> movies;
    int moviePos = 0;
    int numMovies = 1;

    
    enum VISUALIZATION { BITLY, MOVIE, TEST };
    enum VISUALIZATION state = BITLY;

    void loadPointGeometry(ofMesh *mesh);
    void downloadBitlyData(int data[24][5][2]);
    void setAllVerticesToColor(ofColor color, ofMesh *mesh);
    void applyFinalAlpha(ofMesh *mesh);
    void sendMeshColorsToHardware(ofMesh mesh);
    void drawMeshOnScreen(int numColumns, int numRows, ofMesh mesh);
    void updateMesh(int start, int end, float xNoiseColumn[], int hour, float length, ofMesh *mesh);
    void updateAlphaGradient(int start, float alpha, ofMesh *mesh);
    void updateRegionBoundaries(int regionStart[24][5], int regionLength[24][5], ofMesh *mesh);
    void gradiateBoundariesForData(int data[24][5][2], int nextData[24][5][2], int (*regionStart)[5], int (*regionLength)[5], float counter);
    void updateLeadingEdge(int start);
    void updateNoiseValues();
    void updateGlobalCounter();
    void updateRegionBoundariesForData(const int data[24][5][2], int (*regionStart)[5], int (*regionLength)[5]);
    void findMinMaxForData(const int data[24][5][2], int *min, int * max);
    void randomize(float arr[], int n);
    void swap(float *a, float *b);
    float nonLinMap(float in, float inMin, float inMax, float outMin, float outMax, float shaper);
    void updateState(enum VISUALIZATION *state);

};


