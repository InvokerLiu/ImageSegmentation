#include<iostream>
#include<ctime>
using namespace std;

#include "ObjectSegmentation.h"


int main()
{
	CPLSetConfigOption("GDAL_DATA", "E:\\ProgramFile\\CppLib\\gdal242_64\\data");
	clock_t startTime, endTime;
	startTime = clock();
	ObjectSegmentation seg;
	string sInputImageFile = "D:\\SegTest\\Image.tif";
	string sRGBFile = "";
	string sThematicFile = "D:\\SegTest\\road.tif";
	string sOutputFile = "D:\\SegTest\\SegResult.tif";

	seg.SetParam(30, 0.5f, 0.9f);
	string sInfo = seg.Execute(sInputImageFile, sOutputFile, sRGBFile, sThematicFile);
	
	endTime = clock();
	cout << "·Ö¸îÓÃÊ±" << double(endTime - startTime) / CLOCKS_PER_SEC << "Ãë" << endl;
	return 0;
}