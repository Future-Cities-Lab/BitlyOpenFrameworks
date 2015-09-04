//
//  DisplayCell.h
//  emptyExample
//
//  Created by Collin Schupman on 7/28/15.
//
//

#ifndef __emptyExample__DisplayCell__
#define __emptyExample__DisplayCell__

#include "ofMain.h"
#include <stdio.h>

class DisplayCell {
    public:
        DisplayCell();
        DisplayCell(ofColor inCellColor);
        ofColor getCellColor();
        void setCellColor(ofColor inColor);
        vector<ofColor> getPixelColors;
        ~DisplayCell();
    private:
        ofColor cellColor;
        vector<vector<ofColor> > pixelColors;
};

#endif /* defined(__emptyExample__DisplayCell__) */
