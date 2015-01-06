
#include "fileStream.h"

#include <iomanip>


std::ofstream & operator << (std::ofstream & fout, const CvRect & rect){

	fout << rect.y << "\t" << rect.y + rect.height << "\t" << rect.x << "\t" << rect.x + rect.width << std::endl;
	return fout;
}

std::ofstream & operator << (std::ofstream & fout, const Feature & ft){
	int orien;
	if (ft.perv.x >= ft.curr.x){
		if (ft.perv.y >= ft.curr.y){
			orien = 0;   // up left 
		}
		else{
			orien = 2;   // down left
		}
	}
	else{
		if (ft.perv.y >= ft.curr.y){
			orien = 1;   // up right
		}
		else{
			orien = 3;   // down right
		}
	}

	fout << ft.perv.x << ", " << ft.perv.y << "\t" << ft.curr.x << ", " << ft.curr.y << "\t" << sqrt(pow(abs(ft.curr.x - ft.perv.x), 2) + pow(abs(ft.curr.y - ft.perv.y), 2)) << "\t" << orien << std::endl;
	return fout;
}


/* Template specilization of single method from a templated class */

void fileStream< CvRect >::WriteXls(std::vector< CvRect > & vec, const char* filename, int frame) {
	std::ofstream fout;

	fout.open(filename, std::ios_base::out);

	if (!fout.is_open()){
		std::cerr << "failed to open file: " << filename << " !!";
		exit(1);
	}

	/* Setup Title */
	fout << "\t" << std::setw(8) << "Up" << "\t" << std::setw(8) << "Down" << "\t" << std::setw(8) << "Left" << std::setw(8) << "\t" << std::setw(8) << "Right" << std::endl;

	for (m_iter = vec.begin(); m_iter != vec.end(); ++m_iter) {
		fout << "Frame_" << frame++ << "\t";
		fout << (*m_iter);
	}

	fout << std::endl;

	fout.close();
}


void fileStream< Feature >::WriteXls(std::vector< Feature > & vec, const char* filename){

	std::ofstream fout;

	fout.open(filename, std::ios_base::out);

	if (!fout.is_open()){
		std::cerr << "failed to open file: " << filename << " !!";
		exit(1);
	}

	/* Setup Title */
	fout << std::setw(20) << "Prev" << "\t" << std::setw(20) << "Curr" << "\t" << std::setw(8) << "Amp" << std::setw(8) << "\t" << std::setw(8) << "Orien" << std::endl;

	for (m_iter = vec.begin(); m_iter != vec.end(); ++m_iter) {
		fout << (*m_iter);
	}

	fout << std::endl;

	fout.close();

}

