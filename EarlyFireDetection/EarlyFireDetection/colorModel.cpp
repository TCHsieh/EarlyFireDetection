
#include "colorModel.h"


/**
*	@Purpose: check fire-like pixels by rgb model base on reference method
*			  This function will change fire-like pixels to red
*	@Parameter:
*		frame: input source image
*		mask: output mask
*/
void checkByRGB(const IplImage* imgSrc, IplImage* mask) {

	static const int step = imgSrc->widthStep / sizeof(uchar);
	static const int RT = 230;

	static uchar* dataSrc = NULL;
	dataSrc = reinterpret_cast<uchar*>(imgSrc->imageData);

	// mask
	static const int stepMask = mask->widthStep / sizeof(uchar);
	static uchar* dataMask = NULL;
	dataMask = reinterpret_cast<uchar*>(mask->imageData);

	static int i = 0, j = 0, k = 0, idx = 0;
	for (i = 0; i < imgSrc->height; ++i) {
		for (j = 0, k = 0; j < step; j += 3, ++k) {
			idx = i * step + j;
			if (dataSrc[idx + 2] > RT && dataSrc[idx + 2] >= dataSrc[idx + 1] && dataSrc[idx + 1] > dataSrc[idx]) {  // RGB color model determine rule
				dataMask[i * stepMask + k] = static_cast<uchar>(255);
			}
		}
	}
}

/**
*	@Purpose: check fire-like pixels by rgb model base on reference method
*			  This function will change fire-like pixels to red
*	@Parameter:
*		frame: input source image
*		mask: output mask
*/
void checkByRGB(const IplImage* imgSrc, const IplImage* maskMotion, IplImage* maskRGB) {

	static const int step = imgSrc->widthStep / sizeof(uchar);
	static uchar* dataSrc = NULL;
	dataSrc = reinterpret_cast<uchar*>(imgSrc->imageData);

	static const int RT = 230;

	// mask
	static const int stepMask = maskRGB->widthStep / sizeof(uchar);
	static uchar* dataMask = NULL;
	dataMask = reinterpret_cast<uchar*>(maskRGB->imageData);
	static uchar* dataMaskMotion = NULL;
	dataMaskMotion = reinterpret_cast<uchar*>(maskMotion->imageData);

	static int i = 0, j = 0, k = 0, idx = 0;
	for (i = 0; i < imgSrc->height; ++i) {
		for (j = 0, k = 0; j < step; j += 3, ++k) {
			idx = i * step + j;
			if (dataMaskMotion[i * stepMask + k] == 255 && dataSrc[idx + 2] > RT && dataSrc[idx + 2] >= dataSrc[idx + 1] && dataSrc[idx + 1] > dataSrc[idx]) {  // RGB color model determine rule
				dataMask[i * stepMask + k] = static_cast<uchar>(255);
			}
		}
	}
}



/**
*   @ Function: Convert RGB to HSI
*   H: 0~360(degree)  HUE_R = 0 , HUE_G = 120 , HUE_B = 240
*   S&I: 0~1
*   @Parameter:       all img require same size
*
*                          [depth]           [channel]
*		     imgRGB:     IPL_DEPTH_8U			 3
*		     imgHSI:     IPL_DEPTH_64F			 3
*		     maskRGB:    IPL_DEPTH_8U			 1
*/
void RGB2HSIMask(const IplImage* imgRGB, IplImage* imgHSI, IplImage* maskRGB) {
	static const double EFS = 0.000000000000001;			         // acceptable bias 
	static const double DIV13 = 0.333333333333333333333333333;		 // 1/3
	static const double DIV180PI = 180 / CV_PI;                      // (180 / PI)  

	// Temp buffer for H S I spectrum
	static IplImage* imgTmp = NULL; // Execute just one time
	imgTmp = cvCreateImage(cvGetSize(imgRGB), IPL_DEPTH_64F, 3); // every times

	static int step = static_cast<int>(imgTmp->widthStep / sizeof(double));

	static double* dataTmp = NULL, *dataHSI = NULL;
	static uchar* dataRGB = NULL;

	dataTmp = reinterpret_cast<double*>(imgTmp->imageData);   // for easy access tmp data		
	dataHSI = reinterpret_cast<double*>(imgHSI->imageData);   // for easy access hsi data
	dataRGB = reinterpret_cast<uchar*>(imgRGB->imageData);    // for easy access rgb data

	// mask
	static const int stepMaskRGB = maskRGB->widthStep / sizeof(uchar);
	static uchar* dataMaskRGB = NULL;
	dataMaskRGB = reinterpret_cast<uchar*>(maskRGB->imageData);


	// idx i, j, idx
	static int i = 0, j = 0, k = 0, idx = 0;
	static double tmp1 = 0.0, tmp2 = 0.0, x = 0.0, theta = 0.0, tmpAdd = 0.0;

	// normalize rgb to [0,1]
	for (i = 0; i < imgRGB->height; ++i) {
		for (j = 0, k = 0; j < step; j += 3, ++k) {        // loop times = width
			if (dataMaskRGB[i * stepMaskRGB + k] == 255) {   // if the pixel is moving object
				idx = i * step + j;
				dataTmp[idx++] = dataRGB[idx] / 255.0;  // dataTmp[ idx ] = dataRGB[ idx ] / 255.0;
				dataTmp[idx++] = dataRGB[idx] / 255.0;  // dataTmp[ idx + 1 ] = dataRGB[ idx + 1 ] / 255.0;
				dataTmp[idx++] = dataRGB[idx] / 255.0;
			}
		}
	}

	for (i = 0; i < imgRGB->height; ++i) {
		for (j = 0, k = 0; j < step; j += 3, ++k) {
			if (dataMaskRGB[i * stepMaskRGB + k] == 255) { // if the pixel is moving object
				idx = i * step + j;
				// IF ( R = G = B ) , IN INTENSITY AXIS THERE IS NO SATURATRION ,AND NO DEFINE HUE VALUE
				if (fabs(dataTmp[idx + 2] - dataTmp[idx + 1]) < EFS && fabs(dataTmp[idx + 1] - dataTmp[idx]) < EFS) {
					dataHSI[idx] = -1.0; // UNDEFINE
					dataHSI[idx + 1] = 0.0;
					dataHSI[idx + 2] = dataTmp[idx];
				}
				else {
					tmpAdd = dataTmp[idx] + dataTmp[idx + 1] + dataTmp[idx + 2];
					tmp1 = dataTmp[idx + 2] - dataTmp[idx + 1];    //r-g
					tmp2 = dataTmp[idx + 2] - dataTmp[idx];         //r-b
					x = 0.5 * (tmp1 + tmp2) / (sqrt(pow(tmp1, 2) + tmp2 * (dataTmp[idx + 1] - dataTmp[idx])));

					// exam
					if (x < -1.0) { x = -1.0; }
					if (x > 1.0) { x = 1.0; }

					theta = DIV180PI * acos(x);

					if (dataTmp[idx] <= dataTmp[idx + 1]) {
						dataHSI[idx] = theta;
					}
					else {
						dataHSI[idx] = 360.0 - theta;
					}
					dataHSI[idx + 1] = 1.0 - (3.0 / tmpAdd) * (minrgb(dataTmp[idx], dataTmp[idx + 1], dataTmp[idx + 2]));
					dataHSI[idx + 2] = DIV13 * tmpAdd;
				}
			}
		}
	}

	cvReleaseImage(&imgTmp);
}


/**
*	@Purpose: check fire-like pixels by rgb model base on reference method
*			  This function will change fire-like pixels to red
*	@Parameter:
*		frame: input source image
*		mask: output mask
*/
void checkByHSI(const IplImage* imgRGB, const IplImage* imgHSI, IplImage* maskRGB, IplImage* maskHSI) {

	static const int stepImg = imgHSI->widthStep / sizeof(double);
	static const int stepImgRGB = imgRGB->widthStep / sizeof(uchar);

	static double* dataHSI = NULL;
	static uchar* dataSrc = NULL;
	dataHSI = reinterpret_cast<double*>(imgHSI->imageData);
	dataSrc = reinterpret_cast<uchar*>(imgRGB->imageData);

	/* HSI threshold */
	static const int trdH = 60;
	static const double trdS = 0.003043487826087;
	static const double trdI = 0.588235294117647;

	// mask
	static const int stepMask = maskRGB->widthStep / sizeof(uchar);

	static uchar* dataMaskRGB = NULL;
	dataMaskRGB = reinterpret_cast<uchar*>(maskRGB->imageData);
	static uchar* dataMaskHSI = NULL;
	dataMaskHSI = reinterpret_cast<uchar*>(maskHSI->imageData);

	static int i = 0, j = 0, k = 0, idx = 0, idxRGB = 0;
	for (i = 0; i < imgHSI->height; ++i) {
		for (j = 0, k = 0; j < stepImg; j += 3, ++k) {    // stepImg = imgWidth * channel
			idx = i * stepImg + j;
			if (dataMaskRGB[i * stepMask + k] == 255 && dataHSI[idx] <= trdH && dataHSI[idx] >= 0 && dataHSI[idx + 2] > trdI && dataHSI[idx + 1] >= (255 - dataSrc[idx + 2]) * trdS) {  // HSI color model determine rule
				dataMaskHSI[i * stepMask + k] = static_cast<uchar>(255);
			}
		}
	}
}


/**
*	@Function: markup the intrest region based on mask
*  @Parameter
*		src: input image
*		backup: output image (for display)
*		mask: input mask
*/
void regionMarkup(const IplImage* imgSrc, IplImage* imgBackup, IplImage* mask) {

	static const int step = imgSrc->widthStep / sizeof(uchar);
	static uchar* dataBackup = NULL;
	dataBackup = reinterpret_cast<uchar*>(imgBackup->imageData);

	// mask
	static const int stepMask = mask->widthStep / sizeof(uchar);
	static uchar* dataMask = NULL;
	dataMask = reinterpret_cast<uchar*>(mask->imageData);

	static int i = 0, j = 0, k = 0, idx = 0;
	for (i = 0; i < imgSrc->height; ++i) {
		for (j = 0, k = 0; j < step; j += 3, ++k) {
			// find the roi
			if (dataMask[i * stepMask + k] == 255) {             // check the mask
				idx = i * step + j;
				dataBackup[idx++] = static_cast<uchar>(0);    // B
				dataBackup[idx++] = static_cast<uchar>(0);    // G
				dataBackup[idx++] = static_cast<uchar>(255);  // R  ; mark with red
			}
		}
	}
}