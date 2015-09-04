//
//  DisplayCell.cpp
//  emptyExample
//
//  Created by Collin Schupman on 7/28/15.
//
//

#include "DisplayCell.h"

DisplayCell::DisplayCell() {
    DisplayCell( ofColor(0, 0,0) );
}

DisplayCell::DisplayCell(ofColor inCellColor) {
    cellColor = inCellColor;
}

ofColor DisplayCell::getCellColor() {
    return cellColor;
}

void DisplayCell::setCellColor(ofColor inColor) {
    cellColor = inColor;
}

DisplayCell::~DisplayCell() {
    
}