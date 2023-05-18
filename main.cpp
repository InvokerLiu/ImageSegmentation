#include<iostream>
#include<ctime>
using namespace std;

#include "ObjectSegmentation.h"


int main()
{
	CPLSetConfigOption("GDAL_DATA", "./gdal242_64/data");
	clock_t startTime, endTime;
	startTime = clock();
	ObjectSegmentation seg;
	string sInputImageFile = "./TestData/Image.tif";
	string sRGBFile = "";
	string sThematicFile = "./TestData/road.tif";
	string sOutputFile = "./TestData/SegResult.tif";

	seg.SetParam(30, 0.5f, 0.9f);
	string sInfo = seg.Execute(sInputImageFile, sOutputFile, sRGBFile, sThematicFile);
	
	endTime = clock();
	cout << "·Ö¸îÓÃÊ±" << double(endTime - startTime) / CLOCKS_PER_SEC << "Ãë" << endl;
	return 0;
}
