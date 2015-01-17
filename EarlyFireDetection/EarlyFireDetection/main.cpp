/**
* @ Purpose: Early Fire Detection Based On Video Sequences
*
*
* @ Auther: TC, Hsieh
* @ Date: 2014.05.03
*/

/* OpenCV Library */
#include "opencv/cv.h"
#include "opencv/cxcore.h"
#include "opencv/highgui.h"

/* Self-Developed Library */
#include "ds.h"
#include "colorModel.h"
#include "fileStream.h"
#include "opticalFlowTool.h"
#include "motionDetection.h"
#include "fireBehaviorAnalysis.h"

/* C-PlusPlus Library */
#include <iostream> 

/* STL Library */
#include <map>
#include <list>
#include <deque>
#include <vector>

/* Switch */
#define ON (-1)
#define OFF (-2)

/* Debug Mode */
#define DEBUG_MODE (OFF)

/* Background Subtraction */
#define BGS_MODE (ON)

/* Optical Flow Motion Vector */
#define OFMV_DISPLAY (OFF)

/* Halting While Fire Alarm */
#define HALT_MODE (OFF)


using namespace std;
using namespace cv;


/* Non-named namespace, global constants */
namespace{

	/* Background Mode */
#if defined (BGS_MODE) && (BGS_MODE == ON)
	const int BGM_FRAME_COUNT = 20;
#else
	const int BGM_FRAME_COUNT = 0;
#endif 

	/* Optical Flow Parameters */
	const int MAX_CORNERS = 10000;
	const int WIN_SIZE = 5;

	/* Processing Window Size (Frame) */
	const unsigned int PROCESSING_WINDOWS = 15;

	/* Background Model Update Coeeficient */
	const double ACCUMULATE_WEIGHTED_ALPHA_BGM = 0.1;
	const double ACCUMULATE_WEIGHTED_ALPHA_THRESHOLD = 0.05;
	const int THRESHOLD_COEEFICIENT = 5;


	/* Fire-like Region Threshold */
	const int RECT_WIDTH_THRESHOLD = 5;
	const int RECT_HEIGHT_THRESHOLD = 5;
	const int CONTOUR_AREA_THRESHOLD = 12;
	const int CONTOUR_POINTS_THRESHOLD = 12;

}


/* File Path (Resource and Results ) */
namespace{

	const char* InputVideoPath = "test.mp4";
	const char* OutputVideoPath = "tout.avi";

	const char* RectImgFilePath = "[Fire RectInfo][Rocket Engin]\\[Frame_%d][DetectedAt_%d].bmp";         // rect image
	const char* RectInfoFilePath = "[Fire RectInfo][Rocket Engin]\\[Frame_%d][Rect_%d].xls";              // rect information
	char ofInfoFileFolder[100] = "[Fire OFInfo]";              // Optical Flow information


	/* save as img */
	char outfile[50];  // tmp string buffer
	char ImgRGBSavePath[50] = "[Rocket Engin]\\rgb\\%d.bmp";
	char ImgHSISavePath[50] = "[Rocket Engin]\\hsi\\%d.bmp";
	char ImgRectSavePath[50] = "[Rocket Engin]\\rect\\%d.bmp";

	char MaskHSISavePath[50] = "[Rocket Engin]\\maskHSI\\%d.bmp";
	char MaskMorphologySavePath[50] = "[Rocket Engin]\\Morphology\\%d.bmp";
	char ImgSourceSavePath[50] = "[Rocket Engin]\\ImgSource\\%d.bmp";
	char ImgForegroundSavePath[50] = "[Rocket Engin]\\ImgForeground\\%d.bmp";

}




/* Main DS & Algorithms */
#if 1   



/*
deque< vector<feature> >
----------------
|   --------   |
|   |  f1  |   |
|   |------|   |
|   |  f2  |   |   <===   frame 1
|   |------|   |
|   |   .  |   |
|   |   .  |   |
|   |   .  |   |
|   |------|   |
|   |  fi  |   |
|   |------|   |
----------------
|   --------   |
|   |  f1  |   |
|   |------|   |
|   |  f2  |   |   <===   frame 2
|   |------|   |
|   |   .  |   |
|   |   .  |   |
|   |   .  |   |
|   |------|   |
|   |  fj  |   |
|   |------|   |
----------------
.
.
.
*/


/* the contour points in each frame must more then thrdcp and more then processingwindows/3
input:
strd    : centroid of candiadate
thrdcp  : threshold of contourpoints
pwindows: processing windows0
output  : true or flase（legal or not）
*/
bool checkContourPoints(Centroid & ctrd, const unsigned int thrdcp, const unsigned int pwindows) {

	std::deque< std::vector< Feature > >::iterator itrDeq = ctrd.dOFRect.begin();

	unsigned int countFrame = 0;

	// contour points of each frame
	for (; itrDeq != ctrd.dOFRect.end(); ++itrDeq){

		if ((*itrDeq).size() < thrdcp) {
			++countFrame;
		}
	}

	return (countFrame < pwindows / 3) ? true : false;
}

/* write the contour motion vector for debug & analysis
input:
ofInfoFilePath: the file folder name
ctrd          : centroid want to analysis
frame         : current frame
contour       : contour No. of each frame
output        : in the folder ofInfoFilePath
*/
void writeContourPointsInfo(char ofInfoFileFolder[], Centroid & ctrd, const int frame, const int contour) {

	std::deque< std::vector< Feature > >::iterator itrDeq = ctrd.dOFRect.begin();

	std::vector< Feature >::iterator itrVecFeature;

	/* first frame */
	int fcount = frame - PROCESSING_WINDOWS + 1;
	fileStream< Feature > fsOut;

	char outfile[100]; // will corrupt while the index is too large 
	char tmp[100];

	strcpy(tmp, ofInfoFileFolder);
	strcat(tmp, "\\[DetectedAt_%d][Frame_%d][Contour_%d].xls");

	/* contour motion vector of each frame */
	for (; itrDeq != ctrd.dOFRect.end(); ++itrDeq){

		sprintf(outfile, tmp, frame, fcount++, contour);
		fsOut.WriteXls((*itrDeq), outfile);
	}
}

/* accumulate the motin vector depends on its orientation( based on 4 directions )
input:
vecFeature : Contour Features
orien      : accumulate array
output     : orien[4]
*/
void motionOrientationHist(std::vector< Feature > & vecFeature, unsigned int orien[4]){

	std::vector< Feature >::iterator itrVecFeature;

	/* each point of contour  */
	for (itrVecFeature = vecFeature.begin(); itrVecFeature != vecFeature.end(); ++itrVecFeature) {

		/* orientation */
		if ((*itrVecFeature).perv.x >= (*itrVecFeature).curr.x){
			if ((*itrVecFeature).perv.y >= (*itrVecFeature).curr.y){
				++orien[0];   // up-left 
			}
			else{
				++orien[2];   // down-left
			}
		}
		else{
			if ((*itrVecFeature).perv.y >= (*itrVecFeature).curr.y){
				++orien[1];   // up-right
			}
			else{
				++orien[3];   // down-right
			}
		}
	}
}

/* calculate the energy of fire contour based on motion vector
input:
vecFeature : Contour Features
staticCount: centroid want to analysis
totalPoints: current frame

output:
staticCount: the feature counts who's energy is lower than 1.0
totalPoints: the feature counts that energy is between 1.0 ~ 100.0
return: energy
*/
double getEnergy(std::vector< Feature > & vecFeature, unsigned int & staticCount, unsigned int & totalPoints){

	std::vector< Feature >::iterator itrVecFeature;

	/* initialization */
	double tmp, energy = 0.0;

	/* each contour point */
	for (itrVecFeature = vecFeature.begin(); itrVecFeature != vecFeature.end(); ++itrVecFeature) {

		/* energy */
		tmp = pow(abs((*itrVecFeature).curr.x - (*itrVecFeature).perv.x), 2) + pow(abs((*itrVecFeature).curr.y - (*itrVecFeature).perv.y), 2);
		if (tmp < 1.0){
			++staticCount;
		}
		else if (tmp < 100.0){
			energy += tmp;
			++totalPoints;
		}
	}
	return energy;
}

/* Analysis the contour motion vector
input:
ctrd    : cadidate fire object
pwindows: processing window
return  : fire-like or not
*/
bool checkContourEnergy(Centroid & ctrd, const unsigned int pwindows) {

	std::deque< std::vector< Feature > >::iterator itrDeq = ctrd.dOFRect.begin();

	std::vector< Feature >::iterator itrVecFeature;

	unsigned int staticCount = 0, orienCount = 0, orienFrame = 0, totalPoints = 0, passFrame = 0, staticFrame = 0;
	unsigned int orien[4] = { 0 };

	/* contour motion vector of each frame */
	for (; itrDeq != ctrd.dOFRect.end(); ++itrDeq){

		/* flash */
		staticCount = staticFrame = orienCount = staticFrame = staticCount = totalPoints = 0;
		/* energy analysis */
		if (getEnergy((*itrDeq), staticCount, totalPoints) > totalPoints >> 1){
			++passFrame;
		}
		if (staticCount > (*itrDeq).size() >> 1){
			++staticFrame;
		}

		/* flash */
		memset(orien, 0, sizeof(unsigned int) << 2);

		/* orientation analysis */
		motionOrientationHist((*itrDeq), orien);

		for (int i = 0; i < 4; ++i){
			if (orien[i] == 0){
				++orienCount;
			}
		}
		if (orienCount >= 1){
			++orienFrame;
		}
	}

	/* by experience */
	static const unsigned int thrdPassFrame = pwindows >> 1, thrdStaticFrame = pwindows >> 2, thrdOrienFrame = (pwindows >> 3) + 1;

	return (passFrame > thrdPassFrame && staticFrame < thrdStaticFrame && orienFrame < thrdOrienFrame) ? true : false;
}



/* compare the mulMapOFRect space with listCentroid space, if matching insert to listCentroid space as candidate fire-like obj
input:
mulMapOFRect:	new candidate fire-like obj in current frame(with rectangle and motion vector information)
currentFrame:   current processing frame
thrdcp      :   threshold of contour points
pwindows    :	processing windows

output:
imgDisplay  :	boxing the alarm region
listCentroid:	candidate fire-like obj those matching with mulMapOFRect's obj

*/
void matchCentroid(IplImage* imgCentriod, IplImage* imgFireAlarm, std::list< Centroid > & listCentroid, std::multimap< int, OFRect > & mulMapOFRect, int currentFrame, const int thrdcp, const unsigned int pwindows){

	static std::list< Centroid >::iterator itCentroid;		             // iterator of listCentroid
	static std::multimap< int, OFRect >::iterator itOFRect, itOFRectUp;  // iterator of multimapBRect

	static bool flagMatch = false;                                       // checking the list(centroid) and map(rect) match or not
	static CvRect* pRect = NULL;
	static CvRect rectFire = cvRect(0, 0, 0, 0);


	/* Check listCentroid node by node */
	itCentroid = listCentroid.begin();
	static unsigned int rectCount = 0;

	while (itCentroid != listCentroid.end()) {

		/* setup mulMapBRect upper bound */
		itOFRectUp = mulMapOFRect.upper_bound((*itCentroid).centroid.x);

		flagMatch = false;

		/* visit mulMapOFRect between range [itlow,itup) */
		for (itOFRect = mulMapOFRect.begin(); itOFRect != itOFRectUp; ++itOFRect) {

			/* for easy access info */
#ifndef CENTROID
#define CENTROID (*itCentroid).centroid 				
#endif			
			/* for quick access info */
			pRect = &(*itOFRect).second.rect;

			/* matched */
			if (CENTROID.y >= (*pRect).y && ((*pRect).x + (*pRect).width) >= CENTROID.x && ((*pRect).y + (*pRect).height) >= CENTROID.y) {

				/* push rect to the matched listCentorid node */
				(*itCentroid).vecRect.push_back(*pRect);

				/* push vecFeature to matched listCentorid node */
				(*itCentroid).dOFRect.push_back((*itOFRect).second.vecFeature);

				/* Update countFrame and judge the threshold of it */
				if (++((*itCentroid).countFrame) == pwindows) {
					/* GO TO PROCEESING DIRECTION MOTION */
					if (judgeDirectionsMotion((*itCentroid).vecRect, rectFire) && checkContourPoints(*itCentroid, thrdcp, pwindows) && checkContourEnergy(*itCentroid, pwindows)) {

						/* recting the fire region */
						cvRectangle(imgFireAlarm, cvPoint(rectFire.x, rectFire.y), cvPoint((rectFire.x) + (rectFire.width), (rectFire.y) + (rectFire.height)), CV_RGB(0, 100, 255), 3);

						cout << "Alarm: " << currentFrame << endl;
						cvShowImage("Video", imgFireAlarm);

						/* halt */

						/* HALT_MODE */
#if defined (HALT_MODE) && (HALT_MODE == ON)
						cvWaitKey(0);
#endif 
						/* checking the optical flow information */
						//writeContourPointsInfo( ofInfoFileFolder, *itCentroid, currentFrame, rectCount );

						/* save as image */
						//sprintf( outfile, RectImgFilePath, currentFrame-PROCESSING_WINDOWS+1, currentFrame );
						//cvSaveImage( outfile, imgDisplay );

						/* save rect information */
						//fileStream< CvRect > fsOut;
						//sprintf( outfile, RectInfoFilePath, currentFrame-PROCESSING_WINDOWS+1, ++rectCount );
						//fsOut.WriteXls( (*itCentroid).vecRect, outfile, currentFrame-PROCESSING_WINDOWS+1 );

						// then go to erase it
					}
					else{
						break;   // if not on fire go to erase it
					}
				}

				/* mark this rect as matched */
				(*itOFRect).second.match = true;
				flagMatch = true;
				++itCentroid;
				break;    // if mateched break the inner loop
			}
			// if ended the maprect and not mateched anyone go to erase it

		} // for (multimapBRect)

		/* if not found erase node */
		if (!flagMatch) {
			itCentroid = listCentroid.erase(itCentroid);
		}
	}

	//cout << "======================================================================================" << endl;
	/* push new rect to listCentroid */
	for (itOFRect = mulMapOFRect.begin(); itOFRect != mulMapOFRect.end(); ++itOFRect) {

		if (!(*itOFRect).second.match) {
			/* push new node to listCentroid */
			listCentroid.push_back(centroid((*itOFRect).second));
			//cout << "after rect: " << endl;
			//cout << (*itBRect).second << endl;	x
		}
	}

	//cout <<"after list count: "<< listCentroid.size() << endl;

	/* check the list node with image */
	for (itCentroid = listCentroid.begin(); itCentroid != listCentroid.end(); ++itCentroid){
		cvRectangle(imgCentriod, cvPoint((*itCentroid).centroid.x, (*itCentroid).centroid.y), cvPoint(((*itCentroid).centroid.x) + 2, ((*itCentroid).centroid.y) + 2), CV_RGB(0, 0, 0), 3);
		cvShowImage("Centroid checking ", imgCentriod);
		//cout << "after list node: " << endl;
		//cout << (*itCentroid).centroid.x <<"   "<< (*itCentroid).centroid.y << endl;
	}

	/* clear up container */
	mulMapOFRect.clear();
}




#endif  /* Main DS & Algorithms */ 






int main(void) {

#if 1
	// capture from video
	CvCapture* capture = cvCaptureFromAVI(InputVideoPath);

	// from webcam device
	//CvCapture* capture = cvCreateCameraCapture(0);  

	IplImage* imgSrc = cvQueryFrame(capture);

	// Check   
	if (!capture) {
		fprintf(stderr, "Cannot open video!\n");
		return 1;
	}

	// Get the fps
	const int FPS = static_cast<int>(cvGetCaptureProperty(capture, CV_CAP_PROP_FPS));

	cout << "Video fps: " << FPS << endl;

	// set frame size
	CvSize sizeImg = cvGetSize(imgSrc);

	// Fire-like pixels count
	unsigned int fireLikeCount = 0;

	/************************Get Initialization BGModel & Threshold(Standard Deviation)*************************/

	// create motionDetection object
	motionDetection bgs(BGM_FRAME_COUNT, sizeImg);

	// get background model
	IplImage* imgBackgroundModel = bgs.getBackgroundModel(capture);

	// get standard deviation
	IplImage* imgStandardDeviation = bgs.getStandardDeviationFrame();


	/* save as image */
	//cvSaveImage( "Background Model.bmp", imgBackgroundModel );
	//cvSaveImage( "Standard Deviation.bmp", imgStandardDeviation );

	/* show image */
	//cvShowImage( "Background Model", imgBackgroundModel );
	//cvShowImage( "Standard Deviation", imgStandardDeviation );

	IplImage* img32FBackgroundModel = cvCreateImage(sizeImg, IPL_DEPTH_32F, 1);
	IplImage* img32FStandardDeviation = cvCreateImage(sizeImg, IPL_DEPTH_32F, 1);

	/************************Motion Detection*************************/
	// gray
	IplImage* imgGray = cvCreateImage(sizeImg, IPL_DEPTH_8U, 1);
	IplImage* imgDiff = cvCreateImage(sizeImg, IPL_DEPTH_8U, 1);


	// coefficient * Threshold
	bgs.coefficientThreshold(imgStandardDeviation, THRESHOLD_COEEFICIENT);           // cvShowImage( "Standard Deviation", imgStandardDeviation );
	//cvSaveImage( "Cefficient Standard Deviation.bmp", imgStandardDeviation );

	// mask motion
	IplImage* maskMotion = cvCreateImage(sizeImg, IPL_DEPTH_8U, 1);

	// for rgb image display copy from src
	IplImage* imgRGB = cvCreateImage(sizeImg, IPL_DEPTH_8U, 3);
	IplImage* imgHSI = cvCreateImage(sizeImg, IPL_DEPTH_8U, 3);

	// mask rgb
	IplImage* maskRGB = cvCreateImage(sizeImg, IPL_DEPTH_8U, 1);
	// mask hsi
	IplImage* maskHSI = cvCreateImage(sizeImg, IPL_DEPTH_8U, 1);
	IplImage* bufHSI = cvCreateImage(sizeImg, IPL_DEPTH_64F, 3);

	// Optical FLow
#if 1
	IplImage* imgPrev = cvCreateImage(sizeImg, IPL_DEPTH_8U, 1);
	IplImage* imgCurr = cvCreateImage(sizeImg, IPL_DEPTH_8U, 1);
	IplImage* imgDisplay = cvCreateImage(sizeImg, IPL_DEPTH_8U, 3);
	IplImage* imgDisplay2 = cvCreateImage(sizeImg, IPL_DEPTH_8U, 3);
	IplImage* imgFireAlarm = cvCreateImage(sizeImg, IPL_DEPTH_8U, 3);

	// Buffer for Pyramid image  
	CvSize sizePyr = cvSize(sizeImg.width + 8, sizeImg.height / 3);
	IplImage* pyrPrev = cvCreateImage(sizePyr, IPL_DEPTH_32F, 1);
	IplImage* pyrCurr = cvCreateImage(sizePyr, IPL_DEPTH_32F, 1);
	CvPoint2D32f* featuresPrev = new CvPoint2D32f[MAX_CORNERS];
	CvPoint2D32f* featuresCurr = new CvPoint2D32f[MAX_CORNERS];
	CvSize sizeWin = cvSize(WIN_SIZE, WIN_SIZE);
	IplImage* imgEig = cvCreateImage(sizeImg, IPL_DEPTH_32F, 1);
	IplImage* imgTemp = cvCreateImage(sizeImg, IPL_DEPTH_32F, 1);

	// Pyramid Lucas-Kanade 
	char   featureFound[MAX_CORNERS];
	float featureErrors[MAX_CORNERS];
	unsigned int cornerCount = MAX_CORNERS;

	// Go to the end of the AVI 
	cvSetCaptureProperty(capture, CV_CAP_PROP_POS_AVI_RATIO, 1.0);

	// Now that we're at the end, read the AVI position in frames 
	long NumberOfFrames = static_cast<int>(cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES) - 1);

	// Return to the beginning 
	cvSetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES, 0.0);

	cout << NumberOfFrames << endl;

	// notify the current frame 
	unsigned long currentFrame = 0;

	// write as video
	CvVideoWriter* writer = cvCreateVideoWriter(OutputVideoPath, -1, FPS, sizeImg, 1);

#endif  // Optical FLow

	/* Morphology */

	// create morphology mask
	IplConvKernel* maskMorphology = cvCreateStructuringElementEx(
		3,
		5,
		1,
		2,
		CV_SHAPE_RECT,
		0);


	/* Contour */
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contour = NULL;


	/* Rect Motion */
	std::list< Centroid > listCentroid;							  // Centroid container
	std::vector< OFRect > vecOFRect;							  // tmp container for ofrect
	std::multimap< int, OFRect > mulMapOFRect;					  // BRect container

	int ContourFeaturePointCount = 0;
	RectThrd rThrd = rectThrd(RECT_WIDTH_THRESHOLD, RECT_HEIGHT_THRESHOLD, CONTOUR_AREA_THRESHOLD);


#endif


	int key = 0;
	while (key != 'x') {    // exit if user presses 'x' 

		// flash
		cvZero(maskMotion);
		cvZero(maskRGB);
		cvZero(maskHSI);


#if 1
		// set frame
		cvSetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES, currentFrame);

		imgSrc = cvQueryFrame(capture);  // get the first frame   

		if (!imgSrc) {
			break;   // exit if unsuccessful or Reach the end of the video
		}

		// convert rgb to gray 
		cvCvtColor(imgSrc, imgGray, CV_BGR2GRAY);

		// copy for display 
		cvCopy(imgSrc, imgDisplay);
		cvCopy(imgSrc, imgDisplay2);
		cvCopy(imgSrc, imgFireAlarm);

		imgSrc = cvQueryFrame(capture); // get the second frame

		if (!imgSrc) {
			break;
		}

		// the second frame ( gray level )
		cvCvtColor(imgSrc, imgCurr, CV_BGR2GRAY);

		cvShowImage("Video", imgDisplay);
#endif



		/* Step1: Motion Detection */ //base on gray level

		// convert rgb to gray 
		// cvCvtColor( imgSrc, imgGray, CV_BGR2GRAY );                       cvShowImage( "Gray Level", imgGray );  

		// diff = | frame - backgroundModel |
		cvAbsDiff(imgGray, imgBackgroundModel, imgDiff);                       //      cvShowImage( "cvAbsDiff", imgDiff );  
		// imgDiff > standarDeviationx
		bgs.backgroundSubtraction(imgDiff, imgStandardDeviation, maskMotion);    // cvShowImage( "maskMotion", maskMotion ); 

		//sprintf( outfile, ImgForegroundSavePath, currentFrame );
		//cvSaveImage( outfile, maskMotion );	


		/* Step2: Chromatic Filtering */

		/* RGB */
		cvCopy(imgDisplay, imgRGB);
		checkByRGB(imgDisplay, maskMotion, maskRGB);
		// markup the fire-like region
		regionMarkup(imgDisplay, imgRGB, maskRGB);

#if defined( DEBUG_MODE ) && ( DEBUG_MODE == ON )
		cvShowImage("Chromatic Filtering-RGB Model", imgRGB);
#endif

		/* HSI */
		cvCopy(imgDisplay, imgHSI);
		// convert rgb to hsi
		RGB2HSIMask(imgDisplay, bufHSI, maskRGB);
		checkByHSI(imgDisplay, bufHSI, maskRGB, maskHSI);
		regionMarkup(imgDisplay, imgHSI, maskHSI);

#if defined( DEBUG_MODE ) && ( DEBUG_MODE == ON )
		cvShowImage("Chromatic Filtering- HSI Model", imgHSI);
#endif 
		cvCopy(maskHSI, maskRGB);




		/* Step3: Background Model & Threshold update */

		// flip maskMotion 0 => 255, 255 => 0
		bgs.maskNegative(maskMotion);

		/* Background update */

		// 8U -> 32F
		cvConvertScale(imgBackgroundModel, img32FBackgroundModel);
		// B( x, y; t+1 ) = ( 1-alpha )B( x, y; t ) + ( alpha )Src( x, y; t ), if the pixel is stationary 
		accumulateWeighted(Mat(imgGray, 0), Mat(img32FBackgroundModel, 0), ACCUMULATE_WEIGHTED_ALPHA_BGM, Mat(maskMotion, 0));
		// 32F -> 8U
		cvConvertScale(img32FBackgroundModel, imgBackgroundModel);         //cvShowImage( "Background update", imgBackgroundModel );   


		/* Threshold update */
		// 8U -> 32F
		cvConvertScale(imgStandardDeviation, img32FStandardDeviation);
		// T( x, y; t+1 ) = ( 1-alpha )T( x, y; t ) + ( alpha ) | Src( x, y; t ) - B( x, y; t ) |, if the pixel is stationary 
		accumulateWeighted(Mat(imgDiff, 0), Mat(img32FStandardDeviation, 0), ACCUMULATE_WEIGHTED_ALPHA_THRESHOLD, Mat(maskMotion, 0));
		// 32F -> 8U
		cvConvertScale(img32FStandardDeviation, imgStandardDeviation);


		/* Step4: Morphology */

		cvDilate(maskHSI, maskHSI, maskMorphology, 1);

#if defined( DEBUG_MODE ) && ( DEBUG_MODE == ON )
		cvShowImage("Morphology-Dilate", maskHSI);
#endif


		/* Step5: matching rire-like object */

		/* find contour */
		cvFindContours(maskHSI, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));

		/* assign feature points and get the number of feature */
		ContourFeaturePointCount = getContourFeatures(imgDisplay2, imgDisplay, contour, vecOFRect, rThrd, featuresPrev, featuresCurr);


		// Pyramid L-K Optical Flow 
		cvCalcOpticalFlowPyrLK(
			imgGray,						                                 // the first frame (gray level)
			imgCurr,						                                 // the second frame
			pyrPrev,						                                 // pyramid tmep buffer for first frame
			pyrCurr,					                                     // pyramid tmep buffer for second frame
			featuresPrev,				                                     // the feature points that needed to be found(trace)
			featuresCurr,					                                 // the feature points that be traced
			ContourFeaturePointCount,                                        // the number of feature points
			sizeWin,														 // searching window size
			2,																 // using pyramid layer 2: will be 3 layers
			featureFound,													 // notify whether the feature points be traced or not
			featureErrors,													 // 
			cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 20, 0.3),   // iteration criteria
			0																 //
			);


		// Display the flow field
#if defined( OFMV_DISPLAY ) && ( OFMV_DISPLAY == ON )

		drawArrow(imgDisplay2, featuresPrev, featuresCurr, ContourFeaturePointCount, featureFound);
		cvShowImage("Optical Flow", imgDisplay2);

#endif 		

		/* Save the OFMV image */
#if 0
		char str[30];
		sprintf(str, "MotionVector//%d.bmp", currentFrame);
		cvSaveImage(str, imgDisplay2);
#endif 

		/* assign feature points to fire-like obj and then push to multimap */
		assignFeaturePoints(mulMapOFRect, vecOFRect, featureFound, featuresPrev, featuresCurr);

		/* compare the mulMapOFRect space with listCentroid space, if matching insert to listCentroid space as candidate fire-like obj */
		matchCentroid(imgDisplay, imgFireAlarm, listCentroid, mulMapOFRect, currentFrame, CONTOUR_POINTS_THRESHOLD, PROCESSING_WINDOWS);

		cvWriteFrame(writer, imgFireAlarm);

		cout << "< Frame >: " << currentFrame++ << endl;

		key = cvWaitKey(5);

		/* Don't run past the end of the AVI. */
		if (currentFrame == NumberOfFrames) {
			break;
		}
	}



	// halt
	cvWaitKey(0);


	// release memory
	cvReleaseImage(&imgFireAlarm);
	cvReleaseImage(&imgTemp);
	cvReleaseImage(&imgEig);
	cvReleaseImage(&imgPrev);
	cvReleaseImage(&imgCurr);
	cvReleaseImage(&imgDisplay);
	cvReleaseImage(&imgDisplay2);
	cvReleaseImage(&pyrPrev);
	cvReleaseImage(&pyrCurr);
	cvReleaseImage(&maskMotion);
	cvReleaseImage(&maskHSI);
	cvReleaseImage(&maskRGB);
	cvReleaseImage(&imgRGB);
	cvReleaseImage(&imgHSI);
	cvReleaseImage(&bufHSI);
	cvReleaseImage(&imgGray);
	cvReleaseImage(&imgDiff);
	cvReleaseImage(&img32FBackgroundModel);
	cvReleaseImage(&img32FStandardDeviation);
	cvReleaseVideoWriter(&writer);
	cvReleaseCapture(&capture);
	cvReleaseStructuringElement(&maskMorphology);
	cvReleaseMemStorage(&storage);
	cvDestroyAllWindows();

	delete[] featuresPrev;
	delete[] featuresCurr;

	return 0;
}
