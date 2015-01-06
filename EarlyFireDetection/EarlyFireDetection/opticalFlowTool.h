
/* Tool function for optical flow */

#ifndef OPTICALFLOWTOOL_H
#define OPTICALFLOWTOOL_H

#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "ds.h"


/* Drawing Arrow for Optical Flow */
void drawArrow(IplImage* imgDisplay, const CvPoint2D32f* featuresPrev, const CvPoint2D32f*  featuresCurr, const int cornerCount, const char* featureFound);


void fireLikeRegion(IplImage* mask, const CvPoint pt1, const CvPoint pt2);

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
int getContourFeatures(IplImage* imgDisplayCntr, IplImage* imgDisplayFireRegion, CvSeq* contour, std::vector< OFRect > & vecOFRect, const RectThrd & trd, CvPoint2D32f* featuresPrev, CvPoint2D32f* featuresCurr);


/* assign feature points to fire-like obj and then push to multimap
input:
vecOFRect:      fire-like obj
status:			the feature stutas(found or not) after tracking
featuresPrev:	previous feature points
featuresCurr:   current feature points after tracking

output:
mulMapOFRect:	new candidate fire-like obj in current frame(with rectangle and motion vector information)

*/
void assignFeaturePoints(std::multimap< int, OFRect > & mulMapOFRect, std::vector< OFRect > & vecOFRect, char* status, CvPoint2D32f* featuresPrev, CvPoint2D32f* featuresCurr);

#endif 