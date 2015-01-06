#ifndef FILESTREAM_H
#define FILESTREAM_H


#include <fstream>  
#include "ds.h"

template <typename T> class fileStream {
	friend std::ofstream & operator << (std::ofstream & fout, const CvRect & rect);
	friend std::ofstream & operator << (std::ofstream & fout, const Feature & ft);
private:
	T m_data;
	typename std::vector< T >::iterator m_iter;
public:
	fileStream();
	~fileStream();

	int ReadTxt(std::vector<T> & vec, const char* filename);
	void WriteTxt(std::vector<T> & vec, const char* filename);
	void WriteXls(std::vector<T> & vec, const char* filename);
	void WriteXls(std::vector<T> & vec, const char* filename, int frame);

	void ReadBinary(std::vector<T> & vec, const char* filename);
	void WriteBinary(std::vector<T> & vec, const char* filename);
};


/* Template specilization of single method from a templated class */
template <>
void fileStream< CvRect >::WriteXls(std::vector< CvRect > & vec, const char* filename, int frame);

template <>
void fileStream< Feature >::WriteXls(std::vector< Feature > & vec, const char* filename);


template <typename T>
fileStream<T>::fileStream() {
}
template <typename T>
fileStream<T>::~fileStream(){
}

template <typename T>
int fileStream<T>::ReadTxt(std::vector<T> & vec, const char* filename){
	std::ifstream fin;

	fin.open(filename, std::ios_base::in);

	if (!fin.is_open()){
		std::cerr << "failed to open file: " << filename << " !!";
		exit(1);
	}

	while (fin.peek() != EOF){
		fin >> m_data;
		vec.push_back(m_data);
	}

	fin.close();
	return vec.size();
}


template <typename T>
void fileStream<T>::WriteTxt(std::vector<T> & vec, const char* filename) {
	std::ofstream fout;

	fout.open(filename, std::ios_base::out);

	if (!fout.is_open()) {
		std::cerr << "failed to open file: " << filename << " !!";
		exit(1);
	}

	for (m_iter = vec.begin(); m_iter != vec.end(); ++m_iter) {
		fout << (*m_iter) << std::endl;
	}

	fout.close();
}

template <typename T>
void fileStream<T>::WriteXls(std::vector<T> & vec, const char* filename) {
	std::ofstream fout;

	fout.open(filename, std::ios_base::out);

	if (!fout.is_open()){
		std::cerr << "failed to open file: " << filename << " !!";
		exit(1);
	}

	for (m_iter = vec.begin(); m_iter != vec.end(); ++m_iter) {
		fout << (*m_iter);
	}
	fout << std::endl;

	fout.close();
}



template <typename T>
void fileStream<T>::ReadBinary(std::vector<T> & vec, const char* filename){
	std::ifstream fin;

	fin.open(filename, std::ios_base::in | std::ios_base::binary);

	if (!fin.is_open()){
		std::cerr << "failed to open file: " << filename << " !!";
		exit(1);
	}

	fin.read((char*)m_iter, sizeof(T)*length);

	fin.close();
}

template <typename T>
void fileStream<T>::WriteBinary(std::vector<T> & vec, const char* filename) {
	std::ofstream fout;

	fout.open(filename, std::ios_base::out | std::ios_base::binary);

	if (!fout.is_open()) {
		std::cerr << "failed to open file: " << filename << " !!";
		exit(1);
	}

	fout.write((char*)m_iter, sizeof(T)*length);

	fout.close();
}


#endif