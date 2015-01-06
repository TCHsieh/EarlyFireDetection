#ifndef MOTIONDETECTION_H
#define MOTIONDETECTION_H

#include "opencv/cv.h"
#include "opencv/highgui.h"

/* Motion Detection
* purpose: For get the initialization background model and threshold(depends on standard deviation)
* support:
*          backgroundSubtraction(): | imgDiff | > threshold, get the foreground from mask(255)
*          coefficientThreshold():  coef * threshold
*          maskNegative(): 0->255; 255->0
*/
class motionDetection {
private:
	IplImage** mpFrame;
	IplImage* m_imgBackgroundModel;    // background model
	IplImage* m_imgStandardDeviation;  // standard deviation
	IplImage* m_imgThreshold;

	IplImage* mSum;
	IplImage* mTmp;
	IplImage* mTmp8U;
	IplImage* mFrame;
	const int mFrameNumber;            // the number of frame for calculate background model
	int mCount;
	CvSize mSize;                      // image size


	/* avoid copy & assignment */
	motionDetection(const motionDetection & bgs);
	void operator = (const motionDetection & bgs);

	/* tool function */
	void accFrameFromVideo(CvCapture* capture);
	void averageFrame(void);

public:

	/*
	* constructor
	* frameNumber: the number of frame that want to be processing as background model
	* frameSize: the size o frame
	*/
	motionDetection(const int frameNumber, const CvSize frameSize);

	/* destructor */
	~motionDetection();

	/* Need pass capture  ptr */
	IplImage* getBackgroundModel(CvCapture* capture);

	IplImage* getStandardDeviationFrame(void);
	IplImage* getThreshold(void);

	/* one channel & uchar only => imgDiff, imgThreshold, mask
	* the mask always needed to be reflash( cvZero(mask) ) first!!
	*/
	void backgroundSubtraction(const IplImage* imgDiff, const IplImage* imgThreshold, IplImage* mask);
	/* th = th * coefficient */
	void coefficientThreshold(IplImage* imgThreshold, const int coef);
	/* Negative processing, convert darkest areas to lightest and lightest to darkest */
	void maskNegative(IplImage* img);



	/* compare with cvAbsDiff()
	* Result: the result is as same as the cvAbsDiff()
	* one channel & 8U(unsigned char) only
	*/
	void tcAbsDiff(const IplImage* imgGray, const IplImage* imgBackgroundModel, IplImage* imgDiff);
};


#endif