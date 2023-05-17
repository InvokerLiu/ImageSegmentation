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

//SLIC��Simple Linear Iterative Clustering���ָ���
class SLIC
{
public:
	SLIC();
	~SLIC();
	//���ó����طָ����
	void SetParameters(int nSuperpixelSize = 30, double dCompactness = 1.0, int nNumofIter = 10);
	void SetInternalParameters(int nWidth, int nHeight, int nSuperpixelSize = 30, double dCompactness = 1.0, int nNumofIter = 10);
	//ִ�г����س�ʼ�ָ����pDataΪ����������,���ݷ�ΧΪ0-255,������pInitSegDataΪ����������
	void Execute(int *pData, int *pInitSegData, int *pThematicData = NULL);

	string TestEnforceConnectivity(string sRGBFile, string sInputFile, string sOutputFile);


	string Execute(string sInputFile, string sOutputFile, string sRGBFile = "", string sThematicFile = "");
	//���ָ����е�NoDataValueֵȥ��������ǩ���¸�ֵ
	void RefineSegResult(int *pSegData, double *pData, double dNoDataValue, int *nTotalObjNum);
public:
	//��ԭʼ��Ӱ��ת��Ϊ������RGBͼƬ�����ڽ��г����س�ʼ�ָ�
	static string CreateRGBImage(string sInputFile, string sOutputFile);
	

private:
	//ִ�г����طָ�������
	string PerformSuperpixelSLIC(vector<double>& kseedsl, vector<double>& kseedsa, vector<double>& kseedsb,
		vector<double>&	kseedsx, vector<double>& kseedsy, int *& klabels, int *& pThematicData, int STEP, vector<double>&	edgemag, const double& M = 10.0);
	//ѡ�����طָ����ӵ�
	void GetLABXYSeeds_ForGivenStepSize(vector<double>&	kseedsl, vector<double>& kseedsa, vector<double>& kseedsb,
		vector<double>&	kseedsx, vector<double>& kseedsy, int STEP, bool perturbseeds, vector<double>& edgemag);
	//�������ӵ㣬ʹ�����ݶ��½��ķ����ƶ�����ֹ���ӵ��ڵ���߽�
	void PerturbSeeds(vector<double>& kseedsl, vector<double>& kseedsa, vector<double>& kseedsb, 
		vector<double>& kseedsx, vector<double>&	kseedsy, vector<double>& edges, int STEP);
	//��Ե��⣬�������ӵ�����
	void DetectLabEdges(const double* lvec, const double* avec, const double* bvec,
		vector<double>& edges);
	//RGB�ռ�ת����XYZ�ռ䣬����ת����LAB�ռ�
	void RGB2XYZ(const int&	sR, const int& sG, const int& sB, double& X, double& Y, double&	Z);
	//RGB�ռ�ת����LAB�ռ�
	void RGB2LAB(const int&	sR, const int& sG,const int& sB, double& lval, double& aval, double& bval);
	//��RGBת����LAB�ռ�
	void DoRGBtoLABConversion(int *pImageData, double *lvec, double *avec, double *bvec);
	//SLIC�ָ������ֹ��Щ����û�б��ָ
	string EnforceLabelConnectivity(const int* labels, int*& nlabels,//input labels that need to be corrected to remove stray labels
		int* pThematicData, int& numlabels,//the number of labels changes in the end if segments are removed
		const int& K); //the number of superpixels desired by the user

private:
	//LAB�ռ�����
	double*	m_lvec;
	double*	m_avec;
	double*	m_bvec;

	//Ӱ����
	int m_nWidth;
	int m_nHeight;
	//Ӱ���*��
	int m_nSz;
	//�㷨��������
	int m_nNumOfIter;
	//�����ش�С
	int m_nSuperPixelSize;
	//���ն�
	double m_dCompactness;

	//�м�����
	vector<double> sigmal;
	vector<double> sigmaa;
	vector<double> sigmab;
	vector<double> sigmax;
	vector<double> sigmay;
	vector<double> distvec;
	vector<double> clustersize;
	vector<double> inv;
};
