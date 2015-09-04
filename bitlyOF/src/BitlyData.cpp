//
//  BitlyData.cpp
//  emptyExample
//
//  Created by Collin Schupman on 7/28/15.
//
//

#include "BitlyData.h"


BitlyData::BitlyData() {
    BitlyData(0.0f,0);
}

BitlyData::BitlyData(float inLinks, int inClicks) {
    links = inLinks;
    clicks = inClicks;
}

float BitlyData::getLinks() {
    return links;
}

void BitlyData::setLinks(float inLinks) {
    links = inLinks;
}

int BitlyData::getClicks() {
    return clicks;
}

BitlyData::~BitlyData() {
}
