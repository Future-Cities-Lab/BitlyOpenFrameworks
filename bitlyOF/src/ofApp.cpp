/* Copyright 2015 Future Cities Lab */

#include "ofApp.h"
#include <math.h>
#include "ofxCsv.h"
#include "ofxOsc.h"
#include "ofxJSON.h"
#include <vector>
#include <algorithm>
#include <string>
ofArduino	ard;
bool		bSetupArduino;


// ALL
void loadPointGeometry(ofMesh *mesh);
void setAllVerticesToColor(ofMesh *mesh, ofColor color);
void applyFinalAlpha(ofMesh *mesh);
void sendMeshColorsToHardware(ofMesh mesh);

void updateMesh(ofMesh * mesh, int start, int end, float xNoiseColumn[], int hour, float length);
void updateAlphaGradient(ofMesh *mesh, int start, float alpha);
void updateRegionBoundaries(ofMesh *mesh, int regionStart[24][5], int regionLength[24][5]);
void gradiateBoundariesForData(int data[24][5][2], int nextData[24][5][2], int (*regionStart)[5], int (*regionLength)[5], float counter);
void updateLeadingEdge(int start);
void updateNoiseValues();
void updateGlobalCounter();


// DATA
void updateRegionBoundariesForData(const int data[24][5][2], int (*regionStart)[5], int (*regionLength)[5]);
void findMinMaxForData(const int data[24][5][2], int *min, int * max);

// HELPERS
void randomize(float arr[], int n);
void swap(float *a, float *b);
float nonLinMap(float in, float inMin, float inMax, float outMin, float outMax, float shaper);


ofMesh bitlyMesh;
ofMesh testMesh;
ofMesh fireMesh;
ofMesh videoMesh;
ofMesh playerMesh;
ofMesh puffMesh;


int currentHour;
int previousHour;
float globalCounter = 0.0f;

ofColor globalLeft;
ofColor globalCenter;
ofColor globalRight;
float leftPercent;
float rightPercent;
float finalAlpha;

ofxUDPManager udpConnection;

ofxOscReceiver oscReceiver;

ofSerial serial;

float noiseColumns[90];
float noiseGrow = 10.0f;
float noiseGrowChange = 1.0f;

int bRow = 0;
int bColumn = 0;

char msgs[20][3*324 + 2];

ofColor recievedTestColor = ofColor(255,255,255);
ofColor alphaTestColor = ofColor(255,255,255);
ofColor leftTestColor = ofColor(255,255,255);
ofColor rightTestColor = ofColor(255,255,255);
ofColor stateTestColor = ofColor(255,255,255);
ofColor switchTestColor = ofColor(255,255,255);
int switchState = -1;


ofColor testColors[20] = {  ofColor(255, 0, 0), ofColor(0, 255, 0),
    ofColor(0, 0, 255), ofColor(139, 0, 139),
    ofColor(255, 165, 0), ofColor(255, 255, 0),
    ofColor(77.0, 159.0, 222.0), ofColor(165, 42, 42),
    ofColor(255, 0, 255), ofColor(255, 192, 203),
    ofColor(46, 139, 87), ofColor(0, 255, 255),
    ofColor(135, 206, 235), ofColor(240, 230, 140),
    ofColor(128, 0, 0), ofColor(105, 105, 105),
    ofColor(124, 252, 0), ofColor(255, 218, 185),
    ofColor(0, 0, 128), ofColor(255, 215, 0)};


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

enum VISUALIZATION { BITLY, PLAYER, TEST };

VISUALIZATION state = BITLY;
int statePos = 1;

vector <ofVideoPlayer> movies;

int moviePos;
int numMovies;

int shutdownHour = 23;
int startupHour = 4;

bool gotIt;
bool isOn;


#define LED1     5
#define LED2     6


void downloadBitlyData(int data[24][5][2]) {
    ofxJSONElement json;
    char bitlyURL[] = "https://s3.amazonaws.com/dlottrulesok/current.json";
    string hours[24] = {"00", "01", "02", "03", "04", "05", "06", "07", "08",
        "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
        "21", "22", "23"};
    string regions[5] = {"Tokyo", "San Francisco", "New York",
        "London", "Berlin"};
    bool parsingSuccessful = json.open(bitlyURL);
    if (parsingSuccessful) {
        for (int i = 0;  i < 24; i++) {
            for (int j = 0; j < 5; j++) {
                data[i][j][0] = json[hours[i]][regions[j]]["decodes_current"].asInt64();
                data[i][j][1] = json[hours[i]][regions[j]]["decodes_diff"].asInt64();
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

void updateState(enum VISUALIZATION *state) {
    if (*state == BITLY) {
        *state = PLAYER;
        statePos = 2;
    } else if (*state == PLAYER) {
        moviePos++;
        statePos++;
        if (moviePos == numMovies) {
            moviePos = 0;
            *state = BITLY;
            statePos = 1;
        }
        
    } else if (*state == TEST) {
        *state = BITLY;
        statePos = 1;
    }
}

void ofApp::setup() {
    ofEnableAlphaBlending();
    ofSetVerticalSync(true);
    ofEnableSmoothing();
    ofSetFrameRate(12);
    
    gotIt = false;
    
    bitlyMesh.setMode(OF_PRIMITIVE_POINTS);
    bitlyMesh.enableColors();
    fireMesh.setMode(OF_PRIMITIVE_POINTS);
    fireMesh.enableColors();
    testMesh.setMode(OF_PRIMITIVE_POINTS);
    testMesh.enableColors();

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
    
    
    globalLeft = ofColor(255.0, 105.0, 0.0);
    globalCenter = ofColor(255.0, 255.0, 255.0);
    globalRight = ofColor(77.0, 159.0, 222.0);
    
    
    leftPercent = 1.0;
    rightPercent = 1.0;
    finalAlpha = 0.5f;
    
    udpConnection.Create();
    udpConnection.SetEnableBroadcast(true);
    udpConnection.Connect("192.168.2.255", 11999);
    udpConnection.SetNonBlocking(true);
    
    oscReceiver.setup(2009);
    
    for (int i = 0; i < 90; i++) {
        noiseColumns[i] = ofRandom(1.0, 20.0);
    }
    
    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 5; j++) {
            squareNoises[i][j][0] = ofRandom(0.0, 15.0);
            squareNoises[i][j][1] = ofRandom(0.0, 15.0);
        }
    }
    
    currentHour = ofGetHours();
    previousHour = currentHour - 1;
    
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
    
    vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
    
    serial.setup(0, 57600);
    
    // VIDEO
    
    moviePos = 0;

    string path = "movies";
    ofDirectory dir(path);
    dir.allowExt("mp4");
    dir.listDir();
    
    numMovies = dir.numFiles();
    
    for (int i = 0; i < dir.numFiles(); i++) {
        
        ofVideoPlayer player;
        player.loadMovie(dir.getPath(i));
        player.play();
        movies.push_back(player);
    }
    
    
    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 5; j++) {
            bitlyNoise[i][j][0] = ofRandom(0.0,100.0);
            bitlyNoise[i][j][1] = ofRandom(0.0,100.0);
        }
    }
    serial.writeByte('l');
    isOn = true;

    int correctPosition = 0;
    
    for (int i = 0; i < deviceList.size(); i++) {
        if (ofIsStringInString(deviceList[i].getDeviceName(), "1411")) {
            correctPosition = i;
            cout << "GOT IT!" << endl;
            gotIt = true;
        }
    }
    
    ard.connect(deviceList[correctPosition].getDevicePath(), 57600);
    

    ofAddListener(ard.EInitialized, this, &ofApp::setupArduino);
    bSetupArduino	= false;	// flag so we setup arduino when its ready, you don't need to touch this :)
    
}

void findMinMaxForData(const int data[24][5][2], int *min, int * max) {
    *min = 10000000;
    *max = -10000000;
    for (int spot = 0; spot < 24; spot++) {
        float total = 0.0f;
        for (int region = 0; region < 5; region++) {
            total += abs(data[spot][region][1]);
            if (abs(data[spot][region][1]) > *max) {
                *max = abs(data[spot][region][1]);
            } else if (abs(data[spot][region][1]) < *min) {
                *min = abs(data[spot][region][1]);
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

void ofApp::update() {
    updateArduino();

    switch (state) {
        case BITLY: {
            setAllVerticesToColor(&bitlyMesh, ofColor(0.0, 0.0, 0.0));
            if (timeToUpdate) {
                gradiateBoundariesForData(bitlyData, bitlyDataNext, regionStartBitly, regionLengthsBitly, globalCounter);
                updateGlobalCounter();
            }
            for (int i = 0; i < 72; i++) {
                for (int j = 0; j < 90; j++) {
                    int drawingPos = i*90 + j;
                    float newR;
                    float newG;
                    float newB;
                    int shape = 5;
                    if (j < 45) {
                        
                        newR = nonLinMap(j, 0, 45, globalLeft.r, globalCenter.r, shape);
                        newG = nonLinMap(j, 0, 45, globalLeft.g, globalCenter.g, shape);
                        newB = nonLinMap(j, 0, 45, globalLeft.b, globalCenter.b, shape);
                    } else {
                        newR = nonLinMap(j, 45, 90, globalRight.r, globalCenter.r, shape);
                        newG = nonLinMap(j, 45, 90, globalRight.g, globalCenter.g, shape);
                        newB = nonLinMap(j, 45, 90, globalRight.b, globalCenter.b, shape);
                    }
                    bitlyMesh.setColor(drawingPos, ofColor(newR, newG, newB));
                }
            }
            updateAlphaGradient(&bitlyMesh, currentHour*3, 1.0f);
            updateLeadingEdge(currentHour*3);
            updateNoiseValues();
            updateRegionBoundaries(&bitlyMesh, regionStartBitly, regionLengthsBitly);
            
//            if (currentHour != ofGetHours()) {
//                cout << "switching" << endl;
//                if (ofGetHours() == shutdownHour) {
//                    serial.writeByte('h');
//                    cout << "turn off" << endl;
//                } else if (ofGetHours() == startupHour) {
//                    serial.writeByte('l');
//                    cout << "turn on" << endl;
//                } else {
//                    cout << "not time" << endl;
//                }
//                
//                previousHour = currentHour;
//                currentHour = ofGetHours();
//                downloadBitlyData(bitlyDataNext);
//                findMinMaxForData(bitlyDataNext, minNewBitly, maxNewBitly);
//                updateRegionBoundariesForData(bitlyDataNext, regionStartBitly, regionLengthsBitly);
//                timeToUpdate = true;
//            }
            
            applyFinalAlpha(&bitlyMesh);
            break;
        }
        case TEST: {
            int count = 0;
            for (int column = 0; column < 90; column += 18) {
                for (int row = 0; row < 72; row += 18) {
                    for (int k = 0; k < 18; k++) {
                        for (int l = 0; l < 18; l++) {
                            int newCol = column+k;
                            int newRow = row+l;
                            testMesh.setColor(newRow*90 + newCol, testColors[count]);
                        }
                    }
                    count++;
                }
            }
            applyFinalAlpha(&testMesh);
            break;
        }

        case PLAYER: {
            movies[moviePos].update();
            unsigned char * pixels = movies[moviePos].getPixels();
            for (int i = 0; i < 90*72*3; i+=3) {
                ofColor newColor(pixels[i], pixels[i+1], pixels[i+2]);
                playerMesh.setColor(i/3, newColor);
            }
            applyFinalAlpha(&playerMesh);
            break;
        }

    }
    
//   TODO(COLLIN): MAKE FUNCTION
    while (oscReceiver.hasWaitingMessages()) {
        recievedTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
        ofxOscMessage m;
        oscReceiver.getNextMessage(&m);
        if (m.getAddress() == "/alpha/"){
            alphaTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
            finalAlpha = ofMap(m.getArgAsInt32(0), 0.0, 1024.0, 0.0, 1.0);
        } else if (m.getAddress() == "/left/") {
            leftTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
            leftPercent = ofMap(m.getArgAsInt32(0), 0.0, 1024.0, 0.0, 1.0);
        } else if (m.getAddress() == "/right/") {
            rightTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
            rightPercent = ofMap(m.getArgAsInt32(0), 0.0, 1024.0, 0.0, 1.0);
        } else if (m.getAddress() == "/state/") {
            stateTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
            updateState(&state);
        } else if (m.getAddress() == "/switch/") {
            switchTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
            switchState *= -1;
            int arg = m.getArgAsInt32(0);
            if (arg == 0) {
                serial.writeByte('l');
            } else {
                serial.writeByte('h');
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::setupArduino(const int & version) {
    
    // remove listener because we don't need it anymore
    ofRemoveListener(ard.EInitialized, this, &ofApp::setupArduino);
    
    bSetupArduino = true;
    
    ofLogNotice() << ard.getFirmwareName();
    ofLogNotice() << "firmata v" << ard.getMajorFirmwareVersion() << "." << ard.getMinorFirmwareVersion();
    
    ard.sendAnalogPinReporting(2, ARD_ANALOG);
    ard.sendAnalogPinReporting(1, ARD_ANALOG);
    ard.sendAnalogPinReporting(0, ARD_ANALOG);
    
    ard.sendDigitalPinMode(10, ARD_INPUT);
    ard.sendDigitalPinMode(12, ARD_INPUT);
    ard.sendDigitalPinMode(LED1, ARD_OUTPUT);
    ard.sendDigitalPinMode(LED2, ARD_PWM);
    
    ofAddListener(ard.EDigitalPinChanged, this, &ofApp::digitalPinChanged);
    ofAddListener(ard.EAnalogPinChanged, this, &ofApp::analogPinChanged);
}

//--------------------------------------------------------------
void ofApp::updateArduino(){
    
    // update the arduino, get any data or messages.
    // the call to ard.update() is required
    ard.update();
    
    // do not send anything until the arduino has been set up
    if (bSetupArduino) {
        // fade the led connected to pin D11
        ard.sendPwm(11, (int)(128 + 128 * sin(ofGetElapsedTimef())));   // pwm...
    }
    
}

// digital pin event handler, called whenever a digital pin value has changed
// note: if an analog pin has been set as a digital pin, it will be handled
// by the digitalPinChanged function rather than the analogPinChanged function.

//--------------------------------------------------------------
void ofApp::digitalPinChanged(const int & pinNum) {
    
    cout << "digital pin: " + ofToString(pinNum) + " = " + ofToString(ard.getDigital(pinNum)) << endl;
    if (pinNum == 10 && ard.getDigital(pinNum) == 1) {
        
        stateTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
        updateState(&state);
        ard.sendString(ofToString(statePos));
        // sene byte
        
    } else if (pinNum == 12 && ard.getDigital(pinNum) == 1) {
        switchTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
        
        if (switchState == -1) {
            switchState *= -1;
            ard.sendDigital(LED1, 0);
            serial.writeByte('l');
            isOn = true;
        } else {
            ard.sendDigital(LED1, 1);
            switchState *= -1;
            serial.writeByte('h');
            isOn = false;
        }
    }
}

// analog pin event handler, called whenever an analog pin value has changed

//--------------------------------------------------------------
void ofApp::analogPinChanged(const int & pinNum) {

    if (pinNum == 2){
        alphaTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
        finalAlpha = ofMap(ard.getAnalog(pinNum), 1024.0, 0.0, 0.0, 1.0);
        ard.sendPwm(LED2, ofMap(ard.getAnalog(pinNum), 1024.0, 0.0, 0.0, 255.0));

    } else if (pinNum == 1) {
        leftTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
        leftPercent = ofMap(ard.getAnalog(pinNum), 0.0, 1024.0, 0.0, 1.0);
    } else if (pinNum == 0) {
        rightTestColor = ofColor(ofRandom(255.0),ofRandom(255.0),ofRandom(255.0));
        rightPercent = ofMap(ard.getAnalog(pinNum), 0.0, 1024.0, 0.0, 1.0);
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

//    ofSetColor(alphaTestColor);
//    ofFill();
//    ofEllipse(100, 100, 100, 100);
//    ofDrawBitmapString(ofToString(finalAlpha), 200, 100);
//
//    ofSetColor(leftTestColor);
//    ofFill();
//    ofEllipse(100, 200, 100, 100);
//    ofDrawBitmapString(ofToString(leftPercent), 200, 200);
//
//    
//    ofSetColor(rightTestColor);
//    ofFill();
//    ofEllipse(100, 300, 100, 100);
//    ofDrawBitmapString(ofToString(rightPercent), 200, 300);
//
//
//    ofSetColor(stateTestColor);
//    ofFill();
//    ofEllipse(100, 400, 100, 100);
//    ofDrawBitmapString(ofToString(state), 200, 400);
//
//    
//    ofSetColor(switchTestColor);
//    ofFill();
//    ofEllipse(100, 500, 100, 100);
//    ofDrawBitmapString(ofToString(switchState), 200, 500);
//    
//    ofSetColor(recievedTestColor);
//    ofFill();
//    ofEllipse(100, 600, 100, 100);


    switch (state) {
        case BITLY: {
            drawMeshOnScreen(72, 90, bitlyMesh);
            sendMeshColorsToHardware(bitlyMesh);
            //sendMeshColorsToHardwareTest(bitlyMesh);
            break;
        }
        case TEST: {
            drawMeshOnScreen(72, 90, testMesh);
            sendMeshColorsToHardware(testMesh);
            break;
        }
        case PLAYER: {
            drawMeshOnScreen(72, 90, playerMesh);
            sendMeshColorsToHardware(playerMesh);
            break;
        }

    }
    ofSetColor(ofColor(20.0, 234.0, 23.0));
    ofFill();
    if (gotIt) {
        ofEllipse(10, 10, 20, 20);
    }
    ofSetColor(ofColor(20.0, 234.0, 0.0));
    ofFill();
    if (isOn) {
        ofEllipse(60, 60, 20, 20);
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


void gradiateBoundariesForData(int data[24][5][2], int nextData[24][5][2], int (*regionStart)[5], int (*regionLength)[5], float counter) {
    for (int spot = 0; spot < 24; spot++) {
        float total = 0.0f;
        for (int region = 0; region < 5; region++) {
            int diffC = abs(data[spot][region][1]);
            int diffN = abs(nextData[spot][region][1]);
            total += static_cast<int>(ofMap(counter, 0.0, 1.0, diffC, diffN));
        }
        int currentPixels = 0;
        for (int region = 0; region < 5; region++) {
            int diffC = abs(data[spot][region][1]);
            int diffN = abs(nextData[spot][region][1]);
            float pMap = ofMap(counter, 0.0, 1.0, diffC, diffN);
            float percent = (static_cast<int>(pMap)/total * 86.0);
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
            total += abs(data[spot][region][1]);
        }
        int currentPixels = 0;
        for (int region = 0; region < 5; region++) {
            float percent = abs(data[spot][region][1])/total * 86.0;
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
            int shape = 5;
            float newR;
            float newG;
            float newB;
            if (j < 45) {
                
                newR = nonLinMap(j, 0, 45, globalLeft.r, globalCenter.r, shape);
                newG = nonLinMap(j, 0, 45, globalLeft.g, globalCenter.g, shape);
                newB = nonLinMap(j, 0, 45, globalLeft.b, globalCenter.b, shape);
            } else {
                newR = nonLinMap(j, 45, 90, globalRight.r, globalCenter.r, shape);
                newG = nonLinMap(j, 45, 90, globalRight.g, globalCenter.g, shape);
                newB = nonLinMap(j, 45, 90, globalRight.b, globalCenter.b, shape);
            }
            
            (*mesh).setColor(pos, ofColor(newR, newG, newB));
        }
        butt++;
    }
    for (int i = 0; i < 72; i++) {
        for (int j = 45; j <= 67; j++) {
            int here = j;
            int otherSide = 134-j;
            int drawingPos = i*90 + here;
            int drawingPos2 = i*90 + otherSide;
            ofColor temp = bitlyMesh.getColor(drawingPos);
            (*mesh).setColor(drawingPos, bitlyMesh.getColor(drawingPos2));
            (*mesh).setColor(drawingPos2, temp);
        }
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
    updateMesh(&bitlyMesh,  0, 89, noiseColumns, start, noiseGrow);
}

void updateNoiseValues() {
    for (int i = 0; i < 90; i++) {
        noiseColumns[i] += 0.01;
    }
    for (int i = 0; i < 5; i++) {
        noiseGrow = ofNoise(noiseGrowChange)*40.0f;
        noiseGrowChange += 0.01;
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
    cout << key << endl;
    if (key == 98) {
        updateState(&state);
    } else if (key == 359) {
        if (finalAlpha > 0.1) {
            finalAlpha -= 0.1;
        }
    } else if (key == 357) {
        if (finalAlpha < 1.0) {
            finalAlpha += 0.1;
        }
    } else if (key == 116) {
        state = TEST;
    } else if (key == 104) {
        serial.writeByte('h');
        isOn = false;
    } else if (key == 108) {
        serial.writeByte('l');
        isOn = true;
    }
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