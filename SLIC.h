#pragma once
#include <iostream>
#include <vector>
#include <stack>
#include <string>
#include <algorithm>
using namespace std;

#include "gdal_priv.h"
#include "cpl_conv.h"
#include "gdal_alg.h"

#include "GlobalValues.h"

//SLIC（Simple Linear Iterative Clustering）分割类
class SLIC
{
public:
	SLIC();
	~SLIC();
	//设置超像素分割参数
	void SetParameters(int nSuperpixelSize = 30, double dCompactness = 1.0, int nNumofIter = 10);
	void SetInternalParameters(int nWidth, int nHeight, int nSuperpixelSize = 30, double dCompactness = 1.0, int nNumofIter = 10);
	//执行超像素初始分割，输入pData为三波段数组,数据范围为0-255,输出结果pInitSegData为单波段数组
	void Execute(int *pData, int *pInitSegData, int *pThematicData = NULL);

	string TestEnforceConnectivity(string sRGBFile, string sInputFile, string sOutputFile);


	string Execute(string sInputFile, string sOutputFile, string sRGBFile = "", string sThematicFile = "");
	//将分割结果中的NoDataValue值去除，将标签重新赋值
	void RefineSegResult(int *pSegData, double *pData, double dNoDataValue, int *nTotalObjNum);
public:
	//将原始大影像转换为三波段RGB图片，便于进行超像素初始分割
	static string CreateRGBImage(string sInputFile, string sOutputFile);
	

private:
	//执行超像素分割主程序
	string PerformSuperpixelSLIC(vector<double>& kseedsl, vector<double>& kseedsa, vector<double>& kseedsb,
		vector<double>&	kseedsx, vector<double>& kseedsy, int *& klabels, int *& pThematicData, int STEP, vector<double>&	edgemag, const double& M = 10.0);
	//选择超像素分割种子点
	void GetLABXYSeeds_ForGivenStepSize(vector<double>&	kseedsl, vector<double>& kseedsa, vector<double>& kseedsb,
		vector<double>&	kseedsx, vector<double>& kseedsy, int STEP, bool perturbseeds, vector<double>& edgemag);
	//修正种子点，使其向梯度下降的方向移动，防止种子点在地物边界
	void PerturbSeeds(vector<double>& kseedsl, vector<double>& kseedsa, vector<double>& kseedsb, 
		vector<double>& kseedsx, vector<double>&	kseedsy, vector<double>& edges, int STEP);
	//边缘检测，辅助种子点修正
	void DetectLabEdges(const double* lvec, const double* avec, const double* bvec,
		vector<double>& edges);
	//RGB空间转换到XYZ空间，辅助转换到LAB空间
	void RGB2XYZ(const int&	sR, const int& sG, const int& sB, double& X, double& Y, double&	Z);
	//RGB空间转换到LAB空间
	void RGB2LAB(const int&	sR, const int& sG,const int& sB, double& lval, double& aval, double& bval);
	//将RGB转换到LAB空间
	void DoRGBtoLABConversion(int *pImageData, double *lvec, double *avec, double *bvec);
	//SLIC分割后处理，防止有些像素没有被分割到
	string EnforceLabelConnectivity(const int* labels, int*& nlabels,//input labels that need to be corrected to remove stray labels
		int* pThematicData, int& numlabels,//the number of labels changes in the end if segments are removed
		const int& K); //the number of superpixels desired by the user

private:
	//LAB空间数据
	double*	m_lvec;
	double*	m_avec;
	double*	m_bvec;

	//影像宽高
	int m_nWidth;
	int m_nHeight;
	//影像宽*高
	int m_nSz;
	//算法迭代次数
	int m_nNumOfIter;
	//超像素大小
	int m_nSuperPixelSize;
	//紧凑度
	double m_dCompactness;

	//中间数据
	vector<double> sigmal;
	vector<double> sigmaa;
	vector<double> sigmab;
	vector<double> sigmax;
	vector<double> sigmay;
	vector<double> distvec;
	vector<double> clustersize;
	vector<double> inv;
};
