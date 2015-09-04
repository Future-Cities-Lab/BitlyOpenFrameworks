/* Copyright 2015 Future Cities Lab */

#include "ofApp.h"
#include <math.h>
#include "ofxCsv.h"
#include "ofxOsc.h"
#include "ofxJSON.h"
#include <vector>
#include <algorithm>
#include <string>

// ALL
void loadPointGeometry(ofMesh *mesh);
void setAllVerticesToColor(ofMesh *mesh, ofColor color);
void applyFinalAlpha(ofMesh *mesh);
void sendMeshColorsToHardware(ofMesh mesh);

void updateMesh(ofMesh * mesh, int start, int end, float xNoiseColumn[], int hour, float length);
void updateAlphaGradient(ofMesh *mesh, int start, float alpha);
void updateRegionBoundaries(ofMesh *mesh, int regionStart[24][5], int regionLength[24][5]);
void updateColorGradient(const int data[24][5][2], const int regionStart[24][5], const int regionLength[24][5],const int min, const int max, ofMesh *mesh);
void gradiateBoundariesForData(int data[24][5][2], int nextData[24][5][2], int (*regionStart)[5], int (*regionLength)[5], float counter);
void gradiateColorGradient(ofMesh *mesh, int data[24][5][2], int dataNext[24][5][2], int regionStart[24][5], int regionLength[24][5], int min, int max,int minNew, int maxNew, float counter);
void updateLeadingEdge(int start);
void updateNoiseValues();
void updateGlobalCounter();

// FIRE
void updateFire();

// DATA
void updateRegionBoundariesForData(const int data[24][5][2], int (*regionStart)[5], int (*regionLength)[5]);
void findMinMaxForData(const int data[24][5][2], int *min, int * max);

// HELPERS
void randomize(float arr[], int n);
void swap(float *a, float *b);
float nonLinMap(float in, float inMin, float inMax, float outMin, float outMax, float shaper);

// TODO(COLLIN): COME UP WITH MORE MEMORY EFFECIENT DS

/* 
 
 TODO(COLLIN): MAKE THIS INTO A CLASS

 Class Something *Mesh Replacement
 
 ColorState
 Colors
 BitlyMesh
 
 members:
 
 vector ofColors colors
 vector int positions
 
 functions:
 
 Something(geometry)
 draw(int x, int y)
 clear(ofColor color);
 sendToHardware();
 
 updateColorGradient
 gradiateColorGradient
 updateAlphaGradient
 updateLeadingEdge
 updateRegionBoundaries
 */

ofMesh bitlyMesh;
ofMesh testMesh;
ofMesh fireMesh;
ofMesh videoMesh;
ofMesh playerMesh;
ofMesh puffMesh;


int currentHour = 0;
float globalCounter = 0.0f;

ofColor globalHigh;
ofColor globalLow;
float finalAlpha;

ofxUDPManager udpConnection;

ofxOscReceiver oscReceiver;


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

int bRow = 0;
int bColumn = 0;

char msgs[20][3*324 + 2];


/*
 
 TODO(COLLIN): MAKE THIS INTO A CLASS
 
 Class Something
 
 members:
 
 int data[24][5][2];
 int dataNext[24][5][2];
 int regionStart[24][5];
 int regionLength[24][5];
 int ma = 1000;
 int mi = -1000;
 int miNext = 1000;
 int maNext = -1000;
 int *min = &minW;
 int *max = &maxW;
 int *minNew = &minNewW;
 int *maxNew = &maxNewW;
 
 functions:
 
 Something();
 downloadData/update
 findMinMaxForData();
 updateRegionBoundariesForData();
 
 */

float bitlyNoise[24][5][2];


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

float squareNoises[24][5][2];

enum VISUALIZATION { BITLY, FIRE, TEST, VIDEO, SUPERVIDEO, PLAYER, IMAGE };

VISUALIZATION state = BITLY;

float lastTime = 0.0f;
float waitTime = 30.0f;

void downloadBitlyData(int data[24][5][2]) {
    ofxJSONElement json;
    char bitlyURL[] = "https://s3.amazonaws.com/dlottrulesok/current.json";
    string hours[24] = {"00", "01", "02", "03", "04", "05", "06", "07", "08",
        "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
        "21", "22", "23"};
    string regions[5] = {"Berlin", "London", "New York",
        "San Francisco", "Tokyo"};
    bool parsingSuccessful = json.open(bitlyURL);
    if (parsingSuccessful) {
        for (int i = 0;  i < 24; i++) {
            for (int j = 0; j < 5; j++) {
                data[i][j][0] = json[hours[i]][regions[j]]["decodes_current"].asInt64();
                data[i][j][1] = json[hours[i]][regions[j]]["decodes_last"].asInt64();
            }
        }
    } else {
        for (int i = 0; i < 24; i++) {
            for (int j = 0; j < 5; j++) {
                data[i][j][0] = ofRandom(10, 2000);
                data[i][j][1] = ofRandom(10, 20000);
            }
        }
    }
}

void ofApp::setup() {
    ofEnableAlphaBlending();
    ofSetVerticalSync(true);
    ofEnableSmoothing();
    ofSetFrameRate(12);
    
    bitlyMesh.setMode(OF_PRIMITIVE_POINTS);
    bitlyMesh.enableColors();
    fireMesh.setMode(OF_PRIMITIVE_POINTS);
    fireMesh.enableColors();
    testMesh.setMode(OF_PRIMITIVE_POINTS);
    testMesh.enableColors();
    mainMesh.setMode(OF_PRIMITIVE_POINTS);
    mainMesh.enableColors();
    playerMesh.setMode(OF_PRIMITIVE_POINTS);
    playerMesh.enableColors();
    puffMesh.setMode(OF_PRIMITIVE_POINTS);
    puffMesh.enableColors();
    
    /* TODO(COLLIN): THIS SHOULD BE THE CONSTRUCTOR FOR A CLASS */
    loadPointGeometry(&bitlyMesh);
    loadPointGeometry(&fireMesh);
    loadPointGeometry(&testMesh);
    loadPointGeometry(&videoMesh);
    loadPointGeometry(&playerMesh);
    loadPointGeometry(&puffMesh);

    
    globalHigh = ofColor(255, 0, 0);
    globalLow = ofColor(0, 0, 255);
    finalAlpha = 0.5f;
    
    udpConnection.Create();
    udpConnection.SetEnableBroadcast(true);
    udpConnection.Connect("192.168.2.255", 11999);
    udpConnection.SetNonBlocking(true);

    oscReceiver.setup(2009);
    
    randomize(xNoiseColumn1, 17);
    randomize(xNoiseColumn2, 17);
    randomize(xNoiseColumn3, 18);
    randomize(xNoiseColumn4, 17);
    randomize(xNoiseColumn5, 17);
    
    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 5; j++) {
            squareNoises[i][j][0] = ofRandom(0.0, 15.0);
            squareNoises[i][j][1] = ofRandom(0.0, 15.0);
        }
    }
    
    downloadBitlyData(bitlyData);
    findMinMaxForData(bitlyData, minBitly, maxBitly);
    updateRegionBoundariesForData(bitlyData, regionStartBitly, regionLengthsBitly);
    
    for (int i = 1; i <= 10; i+=2) {
        msgs[i-1][0] = i/2 + 1;
        msgs[i-1][1] = 1;
        msgs[i][0] = i/2 + 1;
        msgs[i][1] = 2;
        
        msgs[i+10-1][0] = i/2 + 1;
        msgs[i+10-1][1] = 3;
        cout << i/2 + 1 << endl;
        cout << 3 << endl;
        msgs[i+10][0] = i/2 + 1;
        msgs[i+10][1] = 4;
        cout << i/2 + 1 << endl;
        cout << 4 << endl;
        cout << "" << endl;
    }
    
    camWidth 		= 90;	// try to grab at this size.
    camHeight 		= 72;
    
    //we can now get back a list of devices.
    vector<ofVideoDevice> devices = vidGrabber.listDevices();
    
    for(int i = 0; i < devices.size(); i++){
        cout << devices[i].id << ": " << devices[i].deviceName;
        if( devices[i].bAvailable ){
            cout << endl;
        }else{
            cout << " - unavailable " << endl;
        }
    }
    
    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(60);
    vidGrabber.initGrabber(camWidth,camHeight);
    
    videoInverted 	= new unsigned char[camWidth*camHeight*3];
    videoTexture.allocate(camWidth,camHeight, GL_RGB);
    ofSetVerticalSync(true);
    
    
    //store the width and height for convenience
    int width = vidGrabber.getWidth();
    int height = vidGrabber.getHeight();
    
    //add one vertex to the mesh for each pixel
    for (int y = 0; y < height; y++){
        for (int x = 0; x<width; x++){
            mainMesh.addVertex(ofPoint(x,y,0));	// mesh index = x + y*width
												// this replicates the pixel array within the camera bitmap...
            mainMesh.addColor(ofFloatColor(0,0,0));  // placeholder for colour data, we'll get this from the camera
        }
    }
    
    for (int y = 0; y<height-1; y++){
        for (int x=0; x<width-1; x++){
            mainMesh.addIndex(x+y*width);				// 0
            mainMesh.addIndex((x+1)+y*width);			// 1
            mainMesh.addIndex(x+(y+1)*width);			// 10
            
            mainMesh.addIndex((x+1)+y*width);			// 1
            mainMesh.addIndex((x+1)+(y+1)*width);		// 11
            mainMesh.addIndex(x+(y+1)*width);			// 10
        }
    }
    
    //this is an annoying thing that is used to flip the camera
    cam.setScale(1,-1,1);
    
    
    //this determines how much we push the meshes out
    extrusionAmount = 300.0;
    
    
    // VIDEO
    string path = "movies";
    ofDirectory dir(path);
    dir.allowExt("mp4");
    dir.listDir();
    fingerMovie.loadMovie(dir.getPath(0));
    fingerMovie.play();
    
    puffImage.loadImage("images/chauncey.png");

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 5; j++) {
            bitlyNoise[i][j][0] = ofRandom(0.0,100.0);
            bitlyNoise[i][j][1] = ofRandom(0.0,100.0);
        }
    }
    
}

void findMinMaxForData(const int data[24][5][2], int *min, int * max) {
    *min = 10000000;
    *max = -10000000;
    for (int spot = 0; spot < 24; spot++) {
        float total = 0.0f;
        for (int region = 0; region < 5; region++) {
            total += data[spot][region][0];
            if (data[spot][region][0] > *max) {
                *max = data[spot][region][0];
            } else if (data[spot][region][0] < *min) {
                *min = data[spot][region][0];
            }
        }
    }
}

float nonLinMap(float in, float inMin, float inMax, float outMin, float outMax, float shaper) {
    float pct = ofMap (in, inMin, inMax, 0, 1, true);
    pct = powf(pct, shaper);
    float out = ofMap(pct, 0,1, outMin, outMax, true);
    return out;
}

ofColor heatColor(uint8_t temperature) {
    ofColor heatcolor;
    uint8_t t192 = ofMap(temperature, 0, 255, 0, 191);
    uint8_t heatramp = t192 & 0x3F;
    heatramp <<= 2;
    if (t192 & 0x80) {
        heatcolor.r = 255;
        heatcolor.g = 255;
        heatcolor.b = heatramp;
    } else if (t192 & 0x40) {
        heatcolor.r = 255;
        heatcolor.g = heatramp;
        heatcolor.b = 0;
    } else {
        heatcolor.r = heatramp;
        heatcolor.g = 0;
        heatcolor.b = 0;
    }
    return heatcolor;
}

void ofApp::update() {
    switch (state) {
        case VIDEO: {
            vidGrabber.update();
            if (vidGrabber.isFrameNew()){
                int totalPixels = camWidth*camHeight*3;
                unsigned char * pixels = vidGrabber.getPixels();
                for (int i = 0; i < totalPixels; i+=3) {
                    ofColor newColor(pixels[i], pixels[i+1], pixels[i+2]);
                    videoMesh.setColor(i/3, newColor);
                }
                applyFinalAlpha(&videoMesh);
            }
            break;
        }
        case BITLY: {
            setAllVerticesToColor(&bitlyMesh, ofColor(0.0, 0.0, 0.0));
            if (timeToUpdate) {
                gradiateBoundariesForData(bitlyData, bitlyDataNext, regionStartBitly, regionLengthsBitly, globalCounter);
                gradiateColorGradient(&bitlyMesh, bitlyData, bitlyDataNext, regionStartBitly, regionLengthsBitly, *minBitly, *maxBitly, *minNewBitly, *maxNewBitly, globalCounter);
                updateGlobalCounter();
            } else {
                updateColorGradient(bitlyData, regionStartBitly, regionLengthsBitly, *minBitly, *maxBitly, &bitlyMesh);
            }
            updateAlphaGradient(&bitlyMesh, currentHour*3, 1.0f);
            updateLeadingEdge(currentHour*3);
            updateNoiseValues();
            updateRegionBoundaries(&bitlyMesh, regionStartBitly, regionLengthsBitly);
            if ( ofGetElapsedTimef() - lastTime >= waitTime ) {
                lastTime = ofGetElapsedTimef();
                //downloadBitlyData(bitlyDataNext);
                for (int i = 0; i < 24; i++) {
                    for (int j = 0; j < 5; j++) {
                        bitlyNoise[i][j][0] += ofRandom(0.1, 1.0);
                        bitlyNoise[i][j][1] += ofRandom(0.1, 1.0);
                        cout << bitlyData[i][j][0] << endl;
                        cout << bitlyData[i][j][1] << endl;

                        bitlyDataNext[i][j][0] = bitlyData[i][j][0] + ofMap(ofSignedNoise(bitlyNoise[i][j][0]), -1.0, 1.0, 50000.0, 100000.0);
                        bitlyDataNext[i][j][1] = bitlyData[i][j][1] + ofMap(ofSignedNoise(bitlyNoise[i][j][1]), -1.0, 1.0, 50000.0, 100000.0);
                    }
                }
                findMinMaxForData(bitlyDataNext, minNewBitly, maxNewBitly);
                updateRegionBoundariesForData(bitlyDataNext, regionStartBitly, regionLengthsBitly);
                timeToUpdate = true;
            }
            applyFinalAlpha(&bitlyMesh);
            break;
        }
        case FIRE: {
            updateFire();
            applyFinalAlpha(&fireMesh);
            break;
        }
        case TEST: {
            setAllVerticesToColor(&testMesh, ofColor(0.0, 0.0, 0.0));
            int b = bRow;
            for (int i = (b*270); i < ((b*270)+18); i+=3) {
                for (int j = 0; j < 3; j++) {
                    int bSPos1 = i + (j*90+(bColumn*18));
                    testMesh.setColor(bSPos1, ofColor(255.0,0.0,0.0));
                    int bSPos2 = (i+1) + (j*90+(bColumn*18));
                    testMesh.setColor(bSPos2, ofColor(255.0,0.0,0.0));
                    int bSPos3 = (i+2) + (j*90+(bColumn*18));
                    testMesh.setColor(bSPos3, ofColor(255.0,0.0,0.0));
                }
            }
            bRow++;
            if (bRow == 24) {
                bRow = 0;
                bColumn++;
                bColumn %= 5;
            }
            applyFinalAlpha(&testMesh);
            break;
        }
        case SUPERVIDEO: {
            vidGrabber.update();
            if (vidGrabber.isFrameNew()){
                for (int i=0; i<vidGrabber.getWidth()*vidGrabber.getHeight(); i++){
                    ofFloatColor sampleColor(vidGrabber.getPixels()[i*3]/255.f,				// r
                                             vidGrabber.getPixels()[i*3+1]/255.f,			// g
                                             vidGrabber.getPixels()[i*3+2]/255.f);			// b
                    ofVec3f tmpVec = mainMesh.getVertex(i);
                    tmpVec.z = sampleColor.getBrightness() * extrusionAmount;
                    mainMesh.setVertex(i, tmpVec);
                    mainMesh.setColor(i, sampleColor);
                }
            }
            float rotateAmount = ofMap(ofGetMouseY(), 0, ofGetHeight(), 0, 360);
            ofVec3f camDirection(0,0,1);
            ofVec3f centre(vidGrabber.getWidth()/2.f,vidGrabber.getHeight()/2.f, 255/2.f);
            ofVec3f camDirectionRotated = camDirection.rotated(rotateAmount, ofVec3f(1,0,0));
            ofVec3f camPosition = centre + camDirectionRotated * extrusionAmount;
            cam.setPosition(camPosition);
            cam.lookAt(centre);
            break;
        }
        case PLAYER: {
            fingerMovie.update();
            unsigned char * pixels = fingerMovie.getPixels();
            for (int i = 0; i < 90*72*3; i+=3) {
                ofColor newColor(pixels[i], pixels[i+1], pixels[i+2]);
                playerMesh.setColor(i/3, newColor);
            }
            applyFinalAlpha(&playerMesh);
            break;
        }
        case IMAGE: {
            unsigned char* pixels = puffImage.getPixels();
            for (int i = 0; i < 90*72*4; i+=4) {
                puffMesh.setColor(i/4, ofColor(pixels[i], pixels[i+1], pixels[i+2]));
            }
            applyFinalAlpha(&puffMesh);
        }
    }

    while (oscReceiver.hasWaitingMessages()) {
        ofxOscMessage m;
        oscReceiver.getNextMessage(&m);
        if (m.getAddress() == "/alpha/"){
            cout << "alpha" << endl;
        } else if (m.getAddress() == "/high/"){
            cout << "high" << endl;
        } else if (m.getAddress() == "/low/"){
            cout << "low" << endl;
        } else if (m.getAddress() == "/state/"){
            cout << "state" << endl;
        } else if (m.getAddress() == "/onoff/"){
            cout << "onoff" << endl;
        }
    }
}

void drawMeshOnScreen(int numColumns, int numRows, ofMesh meshToDraw) {
    for (int i = 0; i < numColumns; i++) {
        for (int j = 0; j < numRows; j++) {
            int pos = i*90 + j;
            ofSetColor(meshToDraw.getColor(pos));
            ofFill();
            int x = meshToDraw.getVertex(pos).x;
            int y = meshToDraw.getVertex(pos).y;
            ofEllipse(x, y, 2, 2);
        }
    }
}

void ofApp::draw() {
    ofBackground(0.0, 0.0, 0.0);
    switch (state) {
        case VIDEO: {
            drawMeshOnScreen(72, 90, videoMesh);
            sendMeshColorsToHardware(videoMesh);
            break;
        }
        case BITLY: {
            drawMeshOnScreen(72, 90, bitlyMesh);
            sendMeshColorsToHardware(bitlyMesh);
            break;
        }
        case FIRE: {
            drawMeshOnScreen(72, 90, fireMesh);
            sendMeshColorsToHardware(fireMesh);
            break;
        }
        case TEST: {
            drawMeshOnScreen(72, 90, testMesh);
            sendMeshColorsToHardware(testMesh);
            break;
        }
        case SUPERVIDEO: {
            ofDisableDepthTest();
            ofEnableDepthTest();
            cam.begin();
            mainMesh.drawFaces();
            cam.end();
            break;
        }
        case PLAYER: {
            drawMeshOnScreen(72, 90, playerMesh);
            sendMeshColorsToHardware(playerMesh);
            break;
        }
        case IMAGE: {
            drawMeshOnScreen(72, 90, puffMesh);
            sendMeshColorsToHardware(puffMesh);
            break;
        }
    }
}

/* GEOMETRY */

void loadPointGeometry(ofMesh *mesh) {
    wng::ofxCsv geoTable;
    geoTable.loadFile(ofToDataPath("leds.csv"));
    for (int i = 1; i <= 24; i++) {
        for (int mul = 0; mul < 3; mul++) {
            for (int j = 1; j <= 30; j++) {
                // TODO(COLLIN): BETTER NAMES HERE
                string stuff = geoTable.data[i][j];
                vector<string> results = ofSplitString(stuff, "*");
                for (int k = mul*3; k < (mul*3)+3; k++) {
                    vector<string> info = ofSplitString(results[k], "/");
                    ofVec3f pos(ofToInt(info[0]), ofToInt(info[1]), 0.0);
                    (*mesh).addVertex(pos);
                    (*mesh).addColor(ofColor(0.0, 0.0, 0.0));
                }
            }
        }
    }
}

void gradiateColorGradient(ofMesh *mesh, int data[24][5][2], int dataNext[24][5][2], int regionStart[24][5], int regionLength[24][5], int min, int max, int minNew, int maxNew, float counter) {
    for (int region = 0; region < 5; region++) {
        for (int i = 0; i < 72; i+=3) {
            const int start = regionStart[i/3][region];
            const int stop = regionStart[i/3][region]+regionLength[i/3][region];
            for (int j = start; j < stop; j++) {
                int weatherPos = i/3;
                for (int k = 0; k < 3; k++) {
                    int drawingPos = (i+k)*90 + j;
                    float in1 = static_cast<float>(data[weatherPos][region][1]);
                    float in2 = static_cast<float>(dataNext[weatherPos][region][1]);
//                    float red1 = ofMap(in1, min, max, 0.0, 255.0);
//                    float red2 =  ofMap(in2, minNew, maxNew, 0.0, 255.0);
//                    float finalRed = ofMap(counter, 0.0, 1.0, red1, red2);
                    float blue1 = ofMap(in1, min, max, 200.0, 215.0);
                    float blue2 = ofMap(in2, minNew, maxNew, 200.0, 215.0);
                    float finalBlue = ofMap(counter, 0.0, 1.0, blue1, blue2);
                    (*mesh).setColor(drawingPos, ofColor(255.0, finalBlue, 0.0));
                }
            }
        }
    }
}

void updateColorGradient(const int data[24][5][2], const int regionStart[24][5], const int regionLength[24][5], const int min,  const int max, ofMesh *mesh) {
    for (int region = 0; region < 5; region++) {
        for (int i = 0; i < 72; i+=3) {
            const int start = regionStart[i/3][region];
            const int stop = regionStart[i/3][region]+regionLength[i/3][region];
            for (int j = start; j < stop; j++) {
                int weatherPos = i/3;
                for (int k = 0; k < 3; k++) {
                    int drawingPos = (i+k)*90 + j;
                    float in = static_cast<float>(data[weatherPos][region][1]);
                    //float red  = ofMap(in, min, max, 0.0, 255.0);
                    float red = ofMap(in, min, max, 200.0, 215.0);
                    (*mesh).setColor(drawingPos, ofColor(255.0, red, 0.0));
                }
            }
        }
    }
}

void gradiateBoundariesForData(int data[24][5][2], int nextData[24][5][2], int (*regionStart)[5], int (*regionLength)[5], float counter) {
    for (int spot = 0; spot < 24; spot++) {
        float total = 0.0f;
        for (int region = 0; region < 5; region++) {
            total += static_cast<int>(ofMap(counter, 0.0, 1.0, data[spot][region][0], nextData[spot][region][0]));
        }
        int currentPixels = 0;
        for (int region = 0; region < 5; region++) {
            float percent = (static_cast<int>(ofMap(counter, 0.0, 1.0, data[spot][region][0], nextData[spot][region][0]))/total * 86.0);
            regionLength[spot][region] = percent;
            regionStart[spot][region] = currentPixels;
            currentPixels += percent;
            currentPixels++;
        }
    }
}


void updateRegionBoundariesForData(const int data[24][5][2], int (*regionStart)[5], int (*regionLength)[5]) {
    for (int spot = 0; spot < 24; spot++) {
        float total = 0.0f;
        for (int region = 0; region < 5; region++) {
            total += data[spot][region][0];
        }
        int currentPixels = 0;
        for (int region = 0; region < 5; region++) {
            // TODO:(COLLIN) LARGER SPACES
            float percent = data[spot][region][0]/total * 86.0;
            regionLength[spot][region] = percent;
            regionStart[spot][region] = currentPixels;
            currentPixels += percent;
            currentPixels++;
        }
    }
}

/* COLOR/ALPHA UPDATES */

void updateMesh(ofMesh * mesh, int start, int end, float xNoiseColumn[], int hour, float length) {
    int butt = 0;
    for (int j = start; j <= end; j++) {
        for (int i = 0; i < floor(ofNoise(xNoiseColumn[butt])*length); i++) {
            int newI = (hour+i)%72;
            int pos = newI*90 + j;
            (*mesh).setColor(pos, ofColor(255, 215.0, 0, 255.0));
        }
        butt++;
    }
}

void setAllVerticesToColor(ofMesh *mesh, ofColor color) {
    for (int i = 0; i < 72; i++) {
        for (int j = 0; j < 90; j++) {
            (*mesh).setColor(i*90 + j, color);
        }
    }
}

void updateAlphaGradient(ofMesh * mesh, int start, float alpha) {
    float shaper = 1.0;
    for (int i = 0; i < 72; i++) {
        if (i >= start && i < start + 50) {
            float alpha1 = nonLinMap(i, start, start + 50, 0.0, 1.0, shaper);
            float alpha2 = 1.0f;
            if (i != start + 50) {
                alpha2 = nonLinMap(i+1, start + 1, start + 50, 0.0, 1.0, shaper);
            }
            alpha = nonLinMap(globalCounter, 0.0f, 1.0f, alpha2, alpha1, shaper);
            for (int j = 0; j < 90; j++) {
                int pos = i*90 + j;
                ofColor blah = (*mesh).getColor(pos);
                blah.a = 255.0*alpha;
                (*mesh).setColor(pos, blah);
            }
        } else if (start > 22 && i >= 0 && i < start-22) {
            float alpha1 = nonLinMap(i+72, start + 1, start + 50, 0.0, 1.0, shaper);
            float alpha2 = 1.0f;
            if (i+71+1 != start + 50) {
                alpha2 = nonLinMap(i+72+1, start + 1, start + 50, 0.0, 1.0, shaper);
            }
            alpha = nonLinMap(globalCounter, 0.0f, 1.0f, alpha2, alpha1, shaper);
            for (int j = 0; j < 90; j++) {
                int pos = i*90 + j;
                ofColor blah = (*mesh).getColor(pos);
                blah.a = 255.0*alpha;
                (*mesh).setColor(pos, blah);
            }
        }
    }
}

void updateRegionBoundaries(ofMesh *mesh, int regionStart[24][5], int regionLength[24][5]) {
    for (int region = 0; region < 5; region++) {
        for (int i = 0; i < 72; i+=3) {
            // TODO(COLLIN): UPDATE THE DIFF SO IT ALWAYS GURANTEES 3 (not 1) spaces
            int diff = 6;
            for (int j = 0; j < 6; j++) {
                if (region == 0) {
                    int pos = (i+j)*90 + regionStart[i/3][region]+regionLength[i/3][region];
                    for (int k = -diff; k <= 0; k++) {
                        ofColor c = (*mesh).getColor(pos+k);
                        (*mesh).setColor(pos+k, ofColor(c.r, c.g, c.b, 255.0*(*mesh).getColor(pos+k).a*ofMap(pos+k, pos-diff, pos, 1.0f, 0.0f)));
                    }
                } else if (region == 4) {
                    int pos = (i+j)*90 + regionStart[i/3][region-1]+regionLength[i/3][region-1];
                    for (int k = 0; k <= diff; k++) {
                        ofColor c = (*mesh).getColor(pos+k);
                        (*mesh).setColor(pos+k, ofColor(c.r, c.g, c.b, 255.0*(*mesh).getColor(pos+k).a*ofMap(pos+k, pos, pos+diff, 0.0f, 1.0f)));
                    }
                } else {
                    int pos = (i+j)*90 + regionStart[i/3][region-1]+regionLength[i/3][region-1];
                    for (int k = 0; k <= diff; k++) {
                        ofColor c = (*mesh).getColor(pos+k);
                        (*mesh).setColor(pos+k, ofColor(c.r, c.g, c.b, 255.0*(*mesh).getColor(pos+k).a*ofMap(pos+k, pos, pos+diff, 0.0f, 1.0f)));
                    }
                    pos = (i+j)*90 + + regionStart[i/3][region]+regionLength[i/3][region];
                    for (int k = -diff; k <= 0; k++) {
                        ofColor c = (*mesh).getColor(pos+k);
                        (*mesh).setColor(pos+k, ofColor(c.r, c.g, c.b, 255.0*(*mesh).getColor(pos+k).a*ofMap(pos+k, pos-diff, pos, 1.0f, 0.0f)));
                    }
                }
            }
        }
    }
}

void updateLeadingEdge(int start) {
    updateMesh(&bitlyMesh,  0, 16, xNoiseColumn1, start, xGrow[0]);
    updateMesh(&bitlyMesh, 18, 34, xNoiseColumn2, start, xGrow[1]);
    updateMesh(&bitlyMesh, 36, 53, xNoiseColumn3, start, xGrow[2]);
    updateMesh(&bitlyMesh, 55, 71, xNoiseColumn4, start, xGrow[3]);
    updateMesh(&bitlyMesh, 73, 89, xNoiseColumn5, start, xGrow[4]);
}

void updateNoiseValues() {
    for (int i = 0; i <= 17; i++) {
        if (i == 17) {
            xNoiseColumn3[i] += 0.01;
        } else {
            xNoiseColumn1[i] += 0.01;
            xNoiseColumn2[i] += 0.05;
            xNoiseColumn3[i] += 0.01;
            xNoiseColumn4[i] += 0.009;
            xNoiseColumn5[i] += 0.03;
        }
    }
    for (int i = 0; i < 5; i++) {
        xGrow[i] = ofNoise(xGrowChange[i])*40.0f;
        xGrowChange[i] += 0.01;
    }
    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 5; j++) {
            squareNoises[i][j][0] += 0.01;
            squareNoises[i][j][1] += 0.01;
        }
    }
}

void updateGlobalCounter() {
    globalCounter = globalCounter + 0.005f;
    if (globalCounter >= 1.0f) {
        globalCounter = 0.0f;
        currentHour++;
        currentHour %= 24;
        timeToUpdate = false;
        for (int i = 0; i < 24; i++) {
            for (int j = 0; j < 5; j++) {
                bitlyData[i][j][0] = bitlyDataNext[i][j][0];
                bitlyData[i][j][1] = bitlyDataNext[i][j][1];
            }
        }
        findMinMaxForData(bitlyData, minBitly, maxBitly);
        updateRegionBoundariesForData(bitlyData, regionStartBitly, regionLengthsBitly);
    }
}

void updateFire() {
    static uint8_t heat[6480];
    int COOLING = 25;
    int SPARKING = 250;
    for (int i = 0; i < 6480; i++) {
        uint8_t cooling = ofRandom(0, ((COOLING * 10) / 6480) + 2);
        heat[i] = (uint8_t)ofClamp(heat[i] - cooling, 0, 255);
    }
    for (int k= 6480 - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }
    if (static_cast<int>(ofRandom(0, 255)) < SPARKING) {
        int y = (uint8_t)ofRandom(7);
        heat[y] = (uint8_t)ofClamp(heat[y] + (uint8_t)ofRandom(160, 255), 0, 255);
    }
    for (int row = 0; row < 7; row++) {
        for (int column = 0; column < 90; column++) {
            int heatPos = row*90 + column;
            ofColor c = heatColor(heat[heatPos]);
            for (int numTimes = 1; numTimes <= 10; numTimes++) {
                int meshpos = ((row*10)+numTimes)*90 + column;
                fireMesh.setColor(meshpos, c);
            }
        }
    }
}

void sendMeshColorsToHardware(ofMesh mesh) {
    for (int col = 1; col <= 10; col+=2) {
        int count = 2;
        for (int row = 0; row < 24; row++) {
            int msgPos = col+10;
            if (row >= 0 && row < 6) {
                msgPos = col-1;
            } else if (row >= 6 && row < 12) {
                msgPos = col;
            } else if (row >= 12 && row < 18) {
                msgPos = col+10-1;
            }
            for (int i = (row*270); i < (row*270)+18; i+=3) {
                for (int j = 0; j < 3; j++) {
                    int bSPos1 = i + (j*90+(18*(col/2)));
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos1).r*255.0*mesh.getColor(bSPos1).a);
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos1).g*255.0*mesh.getColor(bSPos1).a);
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos1).b*255.0*mesh.getColor(bSPos1).a);
                    int bSPos2 = (i+1) + (j*90+(18*(col/2)));
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos2).r*255.0*mesh.getColor(bSPos1).a);
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos2).g*255.0*mesh.getColor(bSPos1).a);
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos2).b*255.0*mesh.getColor(bSPos1).a);
                    int bSPos3 = (i+2) + (j*90+(18*(col/2)));
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos3).r*255.0*mesh.getColor(bSPos1).a);
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos3).g*255.0*mesh.getColor(bSPos1).a);
                    msgs[msgPos][count++] = static_cast<int>(mesh.getColor(bSPos3).b*255.0*mesh.getColor(bSPos1).a);
                }
            }
            if (row == 5 || row == 11 || row == 17) {
                count = 2;
            }
        }
    }
    for (int i = 0; i < 20; i++) {
        udpConnection.Send(msgs[i], 3*324 + 2);
        ofSleepMillis(1);
    }
    udpConnection.Send("*", 1);
}

void applyFinalAlpha(ofMesh *mesh) {
    for (int i = 0; i < 72; i++) {
        for (int j = 0; j < 90; j++) {
            ofColor final = (*mesh).getColor(i*90 + j);
            (*mesh).setColor(i*90 + j, ofColor(final.r, final.g, final.b, final.a*finalAlpha));
        }
    }
}

void swap(float *a, float *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void randomize(float arr[], int n) {
    srand(time(NULL));
    for (int i = n-1; i > 0; i--) {
        int j = rand() % (i+1);
        swap(&arr[i], &arr[j]);
    }
}

void ofApp::keyPressed(int key) {
    if (key == 98) {
        state = BITLY;
    } else if (key == 116) {
        state = TEST;
    } else if (key == 102) {
        state = FIRE;
    } else if (key == 359) {
        if (finalAlpha > 0.0) {
            finalAlpha -= 0.1;
        }
    } else if (key == 357) {
        if (finalAlpha < 1.0) {
            finalAlpha += 0.1;
        }
    } else if (key == 118) {
        state = VIDEO;
    } else if (key == 115) {
        state = SUPERVIDEO;

    } else if (key == 112) {
        state = PLAYER;
    } else if (key == 105) {
        state = IMAGE;
    }
    cout << key << endl;
}

void ofApp::keyReleased(int key) {
}

void ofApp::mouseMoved(int x, int y) {
}

void ofApp::mouseDragged(int x, int y, int button) {
}

void ofApp::mousePressed(int x, int y, int button) {
}

void ofApp::mouseReleased(int x, int y, int button) {
}

void ofApp::windowResized(int w, int h) {
}

void ofApp::gotMessage(ofMessage msg) {
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
}

void ofApp::exit() {
}
