
#ifndef DS_H
#define DS_H

#include "opencv/cv.h"
#include <iostream>     /* C-PlusPlus library */


typedef struct RectThrd {

	int rectWidth;
	int rectHeight;
	int cntrArea;


}RectThrd;

/* for initialize rectThrd node */
CV_INLINE RectThrd rectThrd(const int rectWidth, const int rectHeight, const int cntrArea){

	RectThrd rt;

	rt.rectWidth = rectWidth;
	rt.rectHeight = rectHeight;
	rt.cntrArea = cntrArea;

	return rt;
}


/* Optical Flow feature points */
typedef struct Feature{
	CvPoint2D32f perv;
	CvPoint2D32f curr;
}Feature;

/* for initialize Feature node */
CV_INLINE Feature feature(const CvPoint2D32f prev, const CvPoint2D32f curr){

	Feature fr;

	fr.perv = prev;
	fr.curr = curr;

	return fr;
}


/* rect node(rect space) */
typedef struct OFRect{

	bool match;							// determine whether the rect is match or not
	int countCtrP;						// the pixel count of contour
	int countDetected;                  // the pixel count of contour which is only be detected
	CvRect rect;						// rect
	std::vector< Feature > vecFeature;	// optical flow feature points

}OFRect;

/* for initialize ofrect node */
CV_INLINE OFRect ofRect(CvRect rect, const int countCtrP){

	OFRect ofr;

	ofr.match = false;
	ofr.countCtrP = countCtrP;
	ofr.rect = rect;

	return ofr;
}



/* marker node */
typedef struct Centroid{

	int countFrame;									 // how many frame the centroid keeping in the region
	CvPoint centroid;								 // first detected centroid
	std::vector< CvRect > vecRect;					 // rect information
	std::deque< std::vector< Feature > > dOFRect;  // optical flow feature points

}Centroid;

/* for initailize the new centroid node */
CV_INLINE Centroid centroid(OFRect ofrect){

	Centroid ctrd;

	ctrd.countFrame = 1;  // first node
	ctrd.centroid = cvPoint(ofrect.rect.x + (ofrect.rect.width >> 1), ofrect.rect.y + (ofrect.rect.height >> 1));  // centroid position
	ctrd.vecRect.push_back(ofrect.rect);            // push rect information 
	ctrd.dOFRect.push_back(ofrect.vecFeature);           // push contour optical flow feature(after optical flow) 

	return ctrd;
}



typedef struct DirectionsCount{

	int countUp;
	int countDown;
	int countLeft;
	int countRight;

}DirectionsCount;


CV_INLINE void zeroCount(DirectionsCount & count){
	count.countDown = count.countLeft = count.countRight = count.countUp = 0;
}



#endif