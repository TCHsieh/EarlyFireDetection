

#include "opticalFlowTool.h"


/* Drawing Arrow for Optical Flow */
void drawArrow(IplImage* imgDisplay, const CvPoint2D32f* featuresPrev, const CvPoint2D32f*  featuresCurr, const int cornerCount, const char* featureFound){

	static int i, lineThickness = 1;
	static CvScalar lineColor = CV_RGB(100, 200, 250);
	static double angle, hypotenuse, tmpCOS, tmpSIN;
	static CvPoint p, q;
	static const double PI_DIV_4 = CV_PI / 4;

	// Draw the flow field
	for (i = 0; i < cornerCount; ++i)
	{
		// if the feature point wasn't be found
		if (featureFound[i] == 0) {
			continue;
		}

		p.x = static_cast<int>(featuresPrev[i].x);
		p.y = static_cast<int>(featuresPrev[i].y);
		q.x = static_cast<int>(featuresCurr[i].x);
		q.y = static_cast<int>(featuresCurr[i].y);

		angle = atan2(static_cast<double>(p.y - q.y), static_cast<double>(p.x - q.x));
		hypotenuse = sqrt(pow(p.y - q.y, 2.0) + pow(p.x - q.x, 2.0));


		q.x = static_cast<int>(p.x - 10 * hypotenuse * cos(angle));
		q.y = static_cast<int>(p.y - 10 * hypotenuse * sin(angle));

		// '|'
		cvLine(imgDisplay, p, q, lineColor, lineThickness, CV_AA, 0);

		tmpCOS = 3 * cos(angle + PI_DIV_4);
		tmpSIN = 3 * sin(angle + PI_DIV_4);

		p.x = static_cast<int>(q.x + tmpCOS);
		p.y = static_cast<int>(q.y + tmpSIN);
		// '/'
		cvLine(imgDisplay, p, q, CV_RGB(255, 0, 0), lineThickness, CV_AA, 0);

		p.x = static_cast<int>(q.x + tmpCOS);
		p.y = static_cast<int>(q.y + tmpSIN);
		// '\'
		cvLine(imgDisplay, p, q, CV_RGB(255, 0, 0), lineThickness, CV_AA, 0);
	}
}



void fireLikeRegion(IplImage* mask, const CvPoint pt1, const CvPoint pt2){

	static const int stepMask = mask->widthStep / sizeof(uchar);
	static uchar* dataMask = NULL;
	dataMask = reinterpret_cast<uchar*>(mask->imageData);
	static int i, j;

	for (i = pt1.y; i < pt2.y; ++i) {
		for (j = pt1.x; j < pt2.x; ++j) {
			dataMask[i * stepMask + j] = 255;
		}
	}
}

/* get the feature points from contour
input:
imgDisplayCntr      : img for display contours
imgDisplayFireRegion: img for boxing the fire-like region with rectangle
contour             : after cvFindContour()
trd                 : threshold
output:
vecOFRect           : fire-like obj will be assign to this container
featuresPrev        : previous contours points
featuresCurr        : current contours points
return:
the number of contour points
*/
int getContourFeatures(IplImage* imgDisplayCntr, IplImage* imgDisplayFireRegion, CvSeq* contour, std::vector< OFRect > & vecOFRect, const RectThrd & trd, CvPoint2D32f* featuresPrev, CvPoint2D32f* featuresCurr){

	static CvRect aRect;
	static CvPoint* p = NULL;
	static unsigned int countCtrP;

	int ContourFeaturePointCount = 0, idxFeature = 0;

	/* thresholding on connected component */
	for (; contour; contour = contour->h_next) {  // contour-based visiting

		/* Recting the Contour with smallest rectangle */
		aRect = cvBoundingRect(contour, 0);

		/* checking the area */
		if (aRect.width > trd.rectWidth && aRect.height > trd.rectHeight && fabs(cvContourArea(contour)) > trd.cntrArea) {

			/* Drawing the Contours */
			cvDrawContours(
				imgDisplayCntr,
				contour,
				CV_RGB(250, 0, 0),		// Red
				CV_RGB(0, 0, 250),		// Blue
				1,						// Vary max_level and compare results
				2,
				8);

			cvShowImage("Fire-like Contours", imgDisplayCntr);


			/* Drawing the region */
			cvRectangle(imgDisplayFireRegion, cvPoint(aRect.x, aRect.y), cvPoint((aRect.x) + (aRect.width), (aRect.y) + (aRect.height)), CV_RGB(255, 10, 0), 2);

			/* for each contour pixel count	*/
			countCtrP = 0;

			/* access points on each contour */
			for (int i = 0; i < contour->total; ++i){
				p = CV_GET_SEQ_ELEM(CvPoint, contour, i);
				//printf(" (%d,%d)\n", p->x, p->y );

				featuresPrev[idxFeature].x = static_cast<float>(p->x);
				featuresPrev[idxFeature++].y = static_cast<float>(p->y);
				++countCtrP;
				++ContourFeaturePointCount;
			}

			/* push to tmp vector for quick access ofrect node */
			vecOFRect.push_back(ofRect(aRect, countCtrP));
		}
	}

	return ContourFeaturePointCount;
}


/* assign feature points to fire-like obj and then push to multimap
input:
vecOFRect:      fire-like obj
status:			the feature stutas(found or not) after tracking
featuresPrev:	previous feature points
featuresCurr:   current feature points after tracking

output:
mulMapOFRect:	new candidate fire-like obj in current frame(with rectangle and motion vector information)

*/
void assignFeaturePoints(std::multimap< int, OFRect > & mulMapOFRect, std::vector< OFRect > & vecOFRect, char* status, CvPoint2D32f* featuresPrev, CvPoint2D32f* featuresCurr){

	static std::vector< OFRect >::iterator itVecOFRect;
	static unsigned int foundCount = 0;

	int i = 0; // feature point index

	// visit each ofrect in vecOFRect 
	for (itVecOFRect = vecOFRect.begin(); itVecOFRect != vecOFRect.end(); ++itVecOFRect){

		foundCount = 0;

		// contour points count
		for (int p = 0; p < (*itVecOFRect).countCtrP; ++p){
			// if the feature point was be found
			if (status[i] == 0) {
				++i;
				continue;
			}
			else{
				/* push feature to vector of ofrect */
				(*itVecOFRect).vecFeature.push_back(feature(featuresPrev[i], featuresCurr[i]));
				++i;
				++foundCount;
			}
		}

		(*itVecOFRect).countDetected = foundCount;

		/* insert ofrect to multimap */
		mulMapOFRect.insert(std::pair< int, OFRect >((*itVecOFRect).rect.x, *itVecOFRect));
	}
	/* clear up container */
	vecOFRect.clear();
}



