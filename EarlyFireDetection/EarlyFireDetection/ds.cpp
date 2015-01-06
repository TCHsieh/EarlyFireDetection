
#include "ds.h"


std::ostream & operator << (std::ostream & os, const Centroid & ctrd){

	os << "countFrame : " << ctrd.countFrame << std::endl;
	os << "x          : " << ctrd.centroid.x << std::endl;
	os << "y          : " << ctrd.centroid.y << std::endl;

	return os;
}


std::ostream & operator << (std::ostream & os, const OFRect & ofr){

	os << "match	: " << ofr.match << std::endl;
	os << "x		: " << ofr.rect.x << std::endl;
	os << "y		: " << ofr.rect.y << std::endl;
	os << "width	: " << ofr.rect.width << std::endl;
	os << "height	: " << ofr.rect.height << std::endl;
	os << "countCtrP: " << ofr.countCtrP << std::endl;

	return os;
}