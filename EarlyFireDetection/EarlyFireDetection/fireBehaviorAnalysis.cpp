

#include "fireBehaviorAnalysis.h"



/* Counting the foldback point at each directions */
void flodbackPoint(const std::vector< CvRect > & vecRect, DirectionsCount & count){
	std::vector< CvRect >::const_iterator itVec;

#ifndef VECTORACCESS
#define DE_IT_VEC_NEXT (*(itVec + 1))
#define DE_IT_VEC_PREVIOUS (*(itVec - 1))
#endif

	for (itVec = ++vecRect.begin(); (itVec + 1) != vecRect.end(); ++itVec){
		if ((DE_IT_VEC_NEXT.y - (*itVec).y) * ((*itVec).y - DE_IT_VEC_PREVIOUS.y) < 0){
			++count.countUp;
		}
		if ((DE_IT_VEC_NEXT.x - (*itVec).x) * ((*itVec).x - DE_IT_VEC_PREVIOUS.x) < 0){
			++count.countLeft;
		}
		if (((DE_IT_VEC_NEXT.y + DE_IT_VEC_NEXT.height) - ((*itVec).y + (*itVec).height))  * (((*itVec).y + (*itVec).height) - (DE_IT_VEC_PREVIOUS.y + DE_IT_VEC_PREVIOUS.height)) < 0){
			++count.countDown;
		}
		if (((DE_IT_VEC_NEXT.x + DE_IT_VEC_NEXT.width) - ((*itVec).x + (*itVec).width)) * (((*itVec).x + (*itVec).width) - (DE_IT_VEC_PREVIOUS.x + DE_IT_VEC_PREVIOUS.width)) < 0){
			++count.countRight;
		}
	}
}


/* Analysis the rect information */
bool judgeDirectionsMotion(const std::vector< CvRect > & vecRect, CvRect & rectFire) {

	DirectionsCount count;
	zeroCount(count);
	flodbackPoint(vecRect, count);

	/* Direction Up required to be growth and sparkle */
	if ((vecRect.front().y - vecRect.back().y) > 2 && count.countUp >= 3){
		/* set up the last rect to rect the frame */
		rectFire = vecRect.back();
		return true;
	}
	else{
		return false;
	}
}
