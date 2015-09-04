//
//  BitlyData.h
//  emptyExample
//
//  Created by Collin Schupman on 7/28/15.
//
//

#ifndef __emptyExample__BitlyData__
#define __emptyExample__BitlyData__

#include <stdio.h>
#include "ofMain.h"

class BitlyData {
    public:
        BitlyData();
        BitlyData(float inLinks, int inClicks);
        float getLinks();
        void setLinks(float inLinks);
        int getClicks();
        ~BitlyData();
    
    private:
        float links;
        int clicks;
};

#endif /* defined(__emptyExample__BitlyData__) */
