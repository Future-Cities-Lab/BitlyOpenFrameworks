/* Copyright 2015 Future Cities Lab */

#include "ofApp.h"
#include <math.h>
#include "ofxCsv.h"
#include "ofxJSON.h"
#include <vector>
#include <algorithm>
#include <string>
#import "ofSerial.h"

void ofApp::setup() {
    ofEnableAlphaBlending();
    ofSetVerticalSync(true);
    ofEnableSmoothing();
    ofSetFrameRate(24);
    
    bitlyMesh.setMode(OF_PRIMITIVE_POINTS);
    bitlyMesh.enableColors();
    movieMesh.setMode(OF_PRIMITIVE_POINTS);
    movieMesh.enableColors();
    testMesh.setMode(OF_PRIMITIVE_POINTS);
    testMesh.enableColors();
    
    loadGeometry(&bitlyMesh);
    loadGeometry(&movieMesh);
    loadGeometry(&testMesh);

    
    udpConnection.Create();
    udpConnection.SetEnableBroadcast(true);
    udpConnection.Connect("192.168.2.255", 11999);
    udpConnection.SetNonBlocking(true);
    
    oscReceiver.setup(2009);
    
    //TODO(COLLIN): MAKE SMARTER TO CATCH CORRECT
    vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
    serial.setup(0, baud);
    
    for (int i = 0; i < 90; i++) {
        noiseColumns[i] = ofRandom(1.0, 20.0);
    }
    
    //TODO(COLLIN): FIGURE OUT WHAT WE'RE VISUALIZING
    downloadBitlyData(bitlyData);
    findMinMaxForData(bitlyData, minBitly, maxBitly);
    updateRegions(bitlyData, regionStartBitly, regionLengthsBitly);
    
    for (int i = 1; i <= 10; i+=2) {
        msgs[i-1][0] = i/2 + 1;
        msgs[i-1][1] = 1;
        msgs[i][0] = i/2 + 1;
        msgs[i][1] = 2;
        msgs[i+10-1][0] = i/2 + 1;
        msgs[i+10-1][1] = 3;
        msgs[i+10][0] = i/2 + 1;
        msgs[i+10][1] = 4;
    }
    
    //TODO(COLLIN): MAKE MOVIE PLAYER UPDATE WITHOUT RESTARTING
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
}

void ofApp::update() {

    setAllVerticesToColor(ofColor(0.0, 0.0, 0.0), &bitlyMesh);
    if (timeToUpdate) {
        gradiateRegions(bitlyData, bitlyDataNext, regionStartBitly, regionLengthsBitly, globalCounter);
        updateGlobalCounter();
    }
    for (int i = 0; i < 72; i++) {
        for (int j = 0; j < 90; j++) {
            int drawingPos = i*90 + j;
            float newR = ofMap(j, 0, 90, ofMap(leftPercent, 0.0, 1.0, globalRight.r, globalLeft.r), ofMap(rightPercent, 0.0, 1.0, globalLeft.r, globalRight.r));
            float newG = ofMap(j, 0, 90, ofMap(leftPercent, 0.0, 1.0, globalRight.g, globalLeft.g), ofMap(rightPercent, 0.0, 1.0, globalLeft.g, globalRight.g));
            float newB = ofMap(j, 0, 90, ofMap(leftPercent, 0.0, 1.0, globalRight.b, globalLeft.b), ofMap(rightPercent, 0.0, 1.0, globalLeft.b, globalRight.b));
            bitlyMesh.setColor(drawingPos, ofColor(newR, newG, newB));
        }
    }
    drawAlphaGradient(currentHour*3, 1.0f, &bitlyMesh);
    drawLeadingEdge(0, 89, noiseColumns, currentHour*3, noiseGrow, &bitlyMesh);
    updateNoiseValues();
    drawRegions(regionStartBitly, regionLengthsBitly, &bitlyMesh);
    if (currentHour != ofGetHours()) {
        cout << "switching" << endl;
        if (ofGetHours() == 24) {
            serial.writeByte('h');
            cout << "turn off" << endl;
        } else if (ofGetHours() == 6) {
            serial.writeByte('l');
            cout << "turn on" << endl;
        } else {
            cout << "not time" << endl;
        }
        previousHour = currentHour;
        currentHour = ofGetHours();
        downloadBitlyData(bitlyDataNext);
        findMinMaxForData(bitlyDataNext, minNewBitly, maxNewBitly);
        updateRegions(bitlyDataNext, regionStartBitly, regionLengthsBitly);
        timeToUpdate = true;
    }

    movies[moviePos].update();
    unsigned char * pixels = movies[moviePos].getPixels();
    for (int i = 0; i < 90*72*3; i+=3) {
        ofColor newColor(pixels[i], pixels[i+1], pixels[i+2]);
        movieMesh.setColor(i/3, newColor);
    }

    int currentBust = 0;
    for (int column = 0; column < 90; column += 18) {
        for (int row = 0; row < 72; row += 18) {
            for (int k = 0; k < 18; k++) {
                for (int l = 0; l < 18; l++) {
                    int newCol = column+k;
                    int newRow = row+l;
                    testMesh.setColor(newRow*90 + newCol, testColors[currentBust]);
                }
            }
            currentBust++;
        }
    }

    applyFinalAlpha(&bitlyMesh);
    applyFinalAlpha(&movieMesh);
    applyFinalAlpha(&testMesh);

    //TODO(COLLIN): MAKE FUNCTION
//    while (oscReceiver.hasWaitingMessages()) {
//        ofxOscMessage m;
//        oscReceiver.getNextMessage(&m);
//        if (m.getAddress() == "/alpha/"){
//            finalAlpha = ofMap(m.getArgAsInt32(0), 0.0, 1024.0, 0.0, 1.0);
//        } else if (m.getAddress() == "/left/") {
//            leftPercent = ofMap(m.getArgAsInt32(0), 0.0, 1024.0, 0.0, 1.0);
//        } else if (m.getAddress() == "/right/") {
//            rightPercent = ofMap(m.getArgAsInt32(0), 0.0, 1024.0, 0.0, 1.0);
//        } else if (m.getAddress() == "/state/") {
//            updateState(&state);
//        } else if (m.getAddress() == "/switch/") {
//            int arg = m.getArgAsInt32(0);
//            if (arg == 0) {
//                serial.writeByte('l');
//            } else {
//                serial.writeByte('h');
//            }
//        }
//    }
}

void ofApp::draw() {
    ofBackground(0.0, 0.0, 0.0);
    switch (state) {
        case BITLY: {
            drawMeshOnScreen(72, 90, bitlyMesh);
            sendMeshColorsToHardware(bitlyMesh);
            break;
        }
        case MOVIE: {
            drawMeshOnScreen(72, 90, movieMesh);
            sendMeshColorsToHardware(movieMesh);
            break;
        }
        case TEST: {
            drawMeshOnScreen(72, 90, testMesh);
            sendMeshColorsToHardware(testMesh);
            break;

        }
            
    }
}

void ofApp::downloadBitlyData(int data[24][5][2]) {
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

void ofApp::loadGeometry(ofMesh *mesh) {
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

void ofApp::gradiateRegions(int data[24][5][2], int nextData[24][5][2], int (*regionStart)[5], int (*regionLength)[5], float counter) {
    for (int spot = 0; spot < 24; spot++) {
        float total = 0.0f;
        for (int region = 0; region < 5; region++) {
            total += static_cast<int>(ofMap(counter, 0.0, 1.0, abs(data[spot][region][1]), abs(nextData[spot][region][1])));
        }
        int currentPixels = 0;
        for (int region = 0; region < 5; region++) {
            float percent = (static_cast<int>(ofMap(counter, 0.0, 1.0, abs(data[spot][region][1]), abs(nextData[spot][region][1])))/total * 86.0);
            regionLength[spot][region] = percent;
            regionStart[spot][region] = currentPixels;
            currentPixels += percent;
            currentPixels++;
        }
    }
}


void ofApp::updateRegions(const int data[24][5][2], int (*regionStart)[5], int (*regionLength)[5]) {
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

void ofApp::drawLeadingEdge(int start, int end, float xNoiseColumn[], int hour, float length, ofMesh *mesh) {
    int butt = 0;
    for (int j = start; j <= end; j++) {
        for (int i = 0; i < floor(ofNoise(xNoiseColumn[butt])*length); i++) {
            int newI = (hour+i)%72;
            int pos = newI*90 + j;
            float newR = ofMap(j, 0, 90, ofMap(leftPercent, 0.0, 1.0, globalRight.r, globalLeft.r), ofMap(rightPercent, 0.0, 1.0, globalLeft.r, globalRight.r));
            float newG = ofMap(j, 0, 90, ofMap(leftPercent, 0.0, 1.0, globalRight.g, globalLeft.g), ofMap(rightPercent, 0.0, 1.0, globalLeft.g, globalRight.g));
            float newB = ofMap(j, 0, 90, ofMap(leftPercent, 0.0, 1.0, globalRight.b, globalLeft.b), ofMap(rightPercent, 0.0, 1.0, globalLeft.b, globalRight.b));
            (*mesh).setColor(pos, ofColor(newR, newG, newB));
        }
        butt++;
    }
}

void ofApp::setAllVerticesToColor(ofColor color, ofMesh *mesh) {
    for (int i = 0; i < 72; i++) {
        for (int j = 0; j < 90; j++) {
            (*mesh).setColor(i*90 + j, color);
        }
    }
}

void ofApp::drawAlphaGradient(int start, float alpha, ofMesh *mesh) {
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

void ofApp::drawRegions(int regionStart[24][5], int regionLength[24][5], ofMesh *mesh) {
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

void ofApp::updateNoiseValues() {
    for (int i = 0; i <= 90; i++) {
        noiseColumns[i] += 0.01;
    }
        noiseGrow = ofNoise(noiseGrowChange)*40.0f;
        noiseGrowChange += 0.01;
}

void ofApp::updateGlobalCounter() {
    globalCounter = globalCounter + 0.005f;
    if (globalCounter >= 1.0f) {
        globalCounter = 0.0f;
        timeToUpdate = false;
        for (int i = 0; i < 24; i++) {
            for (int j = 0; j < 5; j++) {
                bitlyData[i][j][0] = bitlyDataNext[i][j][0];
                bitlyData[i][j][1] = bitlyDataNext[i][j][1];
            }
        }
        findMinMaxForData(bitlyData, minBitly, maxBitly);
        updateRegions(bitlyData, regionStartBitly, regionLengthsBitly);
    }
}

void ofApp::drawMeshOnScreen(int numColumns, int numRows,ofMesh mesh) {
    for (int i = 0; i < numColumns; i++) {
        for (int j = 0; j < numRows; j++) {
            int pos = i*90 + j;
            ofSetColor(mesh.getColor(pos));
            ofFill();
            int x = mesh.getVertex(pos).x;
            int y = mesh.getVertex(pos).y;
            ofEllipse(x, y, 2, 2);
        }
    }
}

void ofApp::sendMeshColorsToHardware(ofMesh mesh) {
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
                    ofColor c = mesh.getColor(bSPos1);
                    msgs[msgPos][count++] = static_cast<int>(c.r*255.0*c.a);
                    msgs[msgPos][count++] = static_cast<int>(c.g*255.0*c.a);
                    msgs[msgPos][count++] = static_cast<int>(c.b*255.0*c.a);
                    int bSPos2 = (i+1) + (j*90+(18*(col/2)));
                    msgs[msgPos][count++] = static_cast<int>(c.r*255.0*c.a);
                    msgs[msgPos][count++] = static_cast<int>(c.g*255.0*c.a);
                    msgs[msgPos][count++] = static_cast<int>(c.b*255.0*c.a);
                    int bSPos3 = (i+2) + (j*90+(18*(col/2)));
                    msgs[msgPos][count++] = static_cast<int>(c.r*255.0*c.a);
                    msgs[msgPos][count++] = static_cast<int>(c.g*255.0*c.a);
                    msgs[msgPos][count++] = static_cast<int>(c.b*255.0*c.a);
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

void ofApp::applyFinalAlpha(ofMesh *mesh) {
    for (int i = 0; i < 72; i++) {
        for (int j = 0; j < 90; j++) {
            ofColor c = (*mesh).getColor(i*90 + j);
            (*mesh).setColor(i*90+j, ofColor(c.r, c.g, c.b, c.a*finalAlpha));
        }
    }
}

void ofApp::findMinMaxForData(const int data[24][5][2], int *min, int * max) {
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

void ofApp::updateState(enum VISUALIZATION *state) {
    if (*state == BITLY) {
        *state = MOVIE;
    } else if (*state == MOVIE) {
        moviePos++;
        if (moviePos == numMovies) {
            moviePos = 0;
            *state = BITLY;
        }
    } else if (*state == TEST) {
        *state = BITLY;
    }
}

float ofApp::nonLinMap(float in, float inMin, float inMax, float outMin, float outMax, float shaper) {
    float pct = ofMap (in, inMin, inMax, 0, 1, true);
    pct = powf(pct, shaper);
    float out = ofMap(pct, 0,1, outMin, outMax, true);
    return out;
}

void ofApp::keyPressed(int key) {
    if (key == 98) {
        updateState(&state);
    } else if (key == 359) {
        if (finalAlpha > 0.0) {
            finalAlpha -= 0.1;
        }
    } else if (key == 357) {
        if (finalAlpha < 1.0) {
            finalAlpha += 0.1;
        }
    } else if (key == 116) {
        state = TEST;
    } else if (key == 49) {
        float red = globalLeft.r;
        if (red < 255.0) {
            red += 5.0;
        }
        globalLeft = ofColor(red, globalLeft.g, globalLeft.b);
    } else if (key == 50) {
        float red = globalLeft.r;
        if (red > 0.0) {
            red -= 5.0;
        }
        globalLeft = ofColor(red, globalLeft.g, globalLeft.b);
    } else if (key == 51) {
        float green = globalLeft.g;
        if (green < 255.0) {
            green += 5.0;
        }
        globalLeft = ofColor(globalLeft.r, green, globalLeft.b);
    } else if (key == 52) {
        float green = globalLeft.g;
        if (green > 0.0) {
            green -= 5.0;
        }
        globalLeft = ofColor(globalLeft.r, green, globalLeft.b);
    } else if (key == 53) {
        float blue = globalLeft.b;
        if (blue < 255.0) {
            blue += 5.0;
        }
        globalLeft = ofColor(globalLeft.r, globalLeft.g, blue);
    } else if (key == 54) {
        float blue = globalLeft.b;
        if (blue > 0.0) {
            blue -= 5.0;
        }
        globalLeft = ofColor(globalLeft.r, globalLeft.g, blue);
    } else if (key == 113) {
        float red = globalRight.r;
        if (red < 255.0) {
            red += 5.0;
        }
        globalRight = ofColor(red, globalRight.g, globalRight.b);
    } else if (key == 119) {
        float red = globalRight.r;
        if (red > 0.0) {
            red -= 5.0;
        }
        globalRight = ofColor(red, globalRight.g, globalRight.b);
    } else if (key == 101) {
        float green = globalRight.g;
        if (green < 255.0) {
            green += 5.0;
        }
        globalRight = ofColor(globalRight.r, green, globalRight.b);
    } else if (key == 114) {
        float green = globalRight.g;
        if (green > 0.0) {
            green -= 5.0;
        }
        globalRight = ofColor(globalRight.r, green, globalRight.b);
    } else if (key == 116) {
        float blue = globalRight.b;
        if (blue < 255.0) {
            blue += 5.0;
        }
        globalRight = ofColor(globalRight.r, globalRight.g, blue);
    } else if (key == 121) {
        float blue = globalRight.b;
        if (blue > 0.0) {
            blue -= 5.0;
        }
        globalRight = ofColor(globalRight.r, globalRight.g, blue);
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
