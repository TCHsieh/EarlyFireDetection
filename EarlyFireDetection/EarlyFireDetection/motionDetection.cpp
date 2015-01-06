

#include "motionDetection.h"


/* Create buffer for image */
motionDetection::motionDetection(const int frameNumber, const CvSize frameSize) : mFrameNumber(frameNumber), mSize(frameSize), mCount(0){

	// create n IplImage pointer, and assign for mpFrame
	mpFrame = new IplImage*[mFrameNumber];
	for (int i = 0; i < mFrameNumber; ++i){
		mpFrame[i] = cvCreateImage(mSize, IPL_DEPTH_8U, 1);
	}
	// create memory for background model
	m_imgBackgroundModel = cvCreateImage(mSize, IPL_DEPTH_8U, 1);
	// create memory for Standard Deviation(threshold)
	m_imgStandardDeviation = cvCreateImage(mSize, IPL_DEPTH_8U, 1);

	m_imgThreshold = cvCreateImage(mSize, IPL_DEPTH_8U, 1);

	mSum = cvCreateImage(mSize, IPL_DEPTH_32F, 1);
	mTmp = cvCreateImage(mSize, IPL_DEPTH_32F, 1);
	mTmp8U = cvCreateImage(mSize, IPL_DEPTH_8U, 1);
	cvZero(mSum);
}
/* Release memory */
motionDetection::~motionDetection(){
	for (int i = 0; i < mFrameNumber; ++i){
		cvReleaseImage(&mpFrame[i]);
	}
	delete[] mpFrame;
	cvReleaseImage(&m_imgBackgroundModel);
	cvReleaseImage(&m_imgStandardDeviation);
	cvReleaseImage(&mSum);
	cvReleaseImage(&mTmp);
	cvReleaseImage(&mTmp8U);

	cvReleaseImage(&m_imgThreshold);

}

/* Tool function */
void motionDetection::accFrameFromVideo(CvCapture* capture){

	//cvNamedWindow( "Video", CV_WINDOW_AUTOSIZE ); // Create a window to display the video 

	while (mCount != mFrameNumber)
	{
		if (cvGrabFrame(capture))
		{
			mFrame = cvRetrieveFrame(capture);
			// convert rgb to gray 
			cvCvtColor(mFrame, mpFrame[mCount], CV_BGR2GRAY);
			// accumulate each frame
			cvAdd(mSum, mpFrame[mCount], mSum);
			//cvShowImage( "Video", mpFrame[mCount] );  // display current frame 

			++mCount;
			if (cvWaitKey(10) >= 0) {
				break;
			}
		}
		else {
			break;
		}
	}
	//cvDestroyWindow( "Video" );
}
/* Tool function */
void motionDetection::averageFrame(void){
	int tmp = 0;
	// average
	for (int i = 0; i < mSize.height; ++i) {
		for (int j = 0; j < mSize.width; ++j) {
			tmp = static_cast< int >(((float*)(mSum->imageData + i*mSum->widthStep))[j] / mFrameNumber);
			if (tmp > 255) {
				tmp = 255;
			}
			if (tmp < 0) {
				tmp = 0;
			}
			((uchar*)(m_imgBackgroundModel->imageData + i*m_imgBackgroundModel->widthStep))[j] = tmp;
		}
	}
}

/* Calculate Background Model */
IplImage* motionDetection::getBackgroundModel(CvCapture* capture) {

	// accumulate frame from video
	accFrameFromVideo(capture);
	// average the frame series as background model
	averageFrame();

	return m_imgBackgroundModel;
}

/* Standard Deviation */
IplImage* motionDetection::getStandardDeviationFrame(void) {

	// Initialize
	cvZero(mSum);
	for (int i = 0; i < mFrameNumber; ++i) {
		// frame[i] <= | frame[i] - Background Model |
		cvAbsDiff(mpFrame[i], m_imgBackgroundModel, mTmp8U);
		// uchar->float
		cvConvert(mTmp8U, mTmp);
		// mTmp = mTmp * mTmp 
		cvPow(mTmp, mTmp, 2.0);
		// add mSum += mTmp
		cvAdd(mSum, mTmp, mSum);
	}

	// variance: mTmp <= mSum / (mFrameNumber-1)
	for (int i = 0; i < mSize.height; ++i) {
		for (int j = 0; j < mSize.width; ++j) {
			((float*)(mTmp->imageData + i*mTmp->widthStep))[j] = ((float*)(mSum->imageData + i*mSum->widthStep))[j] / (mFrameNumber - 1);
		}
	}

	// standard deviation
	cvPow(mTmp, mTmp, 0.5);

	// float->uchar
	cvConvert(mTmp, m_imgStandardDeviation);

	return m_imgStandardDeviation;
}

/* Standard Deviation */
IplImage* motionDetection::getThreshold(void) {

	int tmp = 0;

	getStandardDeviationFrame();

	for (int i = 0; i < mSize.height; ++i) {
		for (int j = 0; j < mSize.width; ++j) {
			tmp += ((uchar*)(m_imgStandardDeviation->imageData + i*m_imgStandardDeviation->widthStep))[j];
		}
	}

	tmp /= mSize.height * mSize.width;

	for (int i = 0; i < mSize.height; ++i) {
		for (int j = 0; j < mSize.width; ++j) {

			if (tmp > 255){
				tmp = 255;
			}
			if (tmp < 0){
				tmp = 0;
			}

			((uchar*)(m_imgThreshold->imageData + i*m_imgThreshold->widthStep))[j] = tmp;
		}
	}

	return m_imgThreshold;
}


/* Negative processing, convert darkest areas to lightest and lightest to darkest */
void motionDetection::maskNegative(IplImage* img)
{
	static int i = 0, j = 0;

	for (i = 0; i < img->height; ++i) {
		for (j = 0; j < img->width; ++j) {
			if (((uchar*)(img->imageData + i * img->widthStep))[j] == 0) {
				((uchar*)(img->imageData + i * img->widthStep))[j] = 255;
			}
			else {
				((uchar*)(img->imageData + i * img->widthStep))[j] = 0;
			}
		}
	}
}


/* th = th * coefficient */
void motionDetection::coefficientThreshold(IplImage* imgThreshold, const int coef) {

	uchar* dataThreshold = NULL;
	dataThreshold = reinterpret_cast<uchar*>(imgThreshold->imageData);
	int step = static_cast<int>(imgThreshold->widthStep / sizeof(uchar));
	int i, j, tmp;

	for (i = 0; i < imgThreshold->height; ++i) {
		for (j = 0; j < imgThreshold->width; ++j) {
			tmp = dataThreshold[i * step + j] * coef;
			if (tmp > 255){
				tmp = 255;
			}
			if (tmp < 0){
				tmp = 0;
			}
			dataThreshold[i * step + j] = tmp;
		}
	}
}


/* one channel & uchar only => imgDiff, imgThreshold, mask
* the mask always needed to be reflash( cvZero(mask) ) first!!
*/
void motionDetection::backgroundSubtraction(const IplImage* imgDiff, const IplImage* imgThreshold, IplImage* mask){

	static uchar* dataMask = NULL;
	static uchar* dataDiff = NULL;
	static uchar* dataThreshold = NULL;

	dataMask = reinterpret_cast<uchar*>(mask->imageData);
	dataDiff = reinterpret_cast<uchar*>(imgDiff->imageData);
	dataThreshold = reinterpret_cast<uchar*>(imgThreshold->imageData);

	static const int step = static_cast<int>(mask->widthStep / sizeof(uchar));
	static int i, j;

	for (i = 0; i < imgDiff->height; ++i){
		for (j = 0; j < imgDiff->width; ++j){
			// foreground(255)
			if (dataDiff[i * step + j] > dataThreshold[i * step + j]){
				dataMask[i * step + j] = 255;
			}
			// else background(0)
		}
	}
}


/* compare with cvAbsDiff()
* Result: the result is as same as the cvAbsDiff()
* one channel & 8U(unsigned char) only
*/
void motionDetection::tcAbsDiff(const IplImage* imgGray, const IplImage* imgBackgroundModel, IplImage* imgDiff){

	static uchar* dataGray = NULL;
	static uchar* dataDiff = NULL;
	static uchar* dataBackgroundModel = NULL;

	dataGray = reinterpret_cast<uchar*>(imgGray->imageData);
	dataDiff = reinterpret_cast<uchar*>(imgDiff->imageData);
	dataBackgroundModel = reinterpret_cast<uchar*>(imgBackgroundModel->imageData);

	static const int step = static_cast<int>(imgGray->widthStep / sizeof(uchar));
	static int i, j, tmp;

	for (i = 0; i < imgDiff->height; ++i){
		for (j = 0; j < imgDiff->width; ++j){

			// | imgGray - imgBackgroundModel |
			tmp = abs(dataGray[i * step + j] - dataBackgroundModel[i * step + j]);
			if (tmp < 0){
				tmp = 0;
			}
			if (tmp > 255){
				tmp = 255;
			}
			// assign value
			dataDiff[i * step + j] = tmp;
		}
	}
}