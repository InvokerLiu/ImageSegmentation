#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <stack>
#include <stdio.h>
using namespace std;

#include "gdal_priv.h"
#include "cpl_conv.h"
#include "gdal_alg.h"
#include "ogrsf_frmts.h"

#include "GlobalValues.h"

//������С�������
class MinRectangle
{
public:
	MinRectangle(int nXMin = 0, int nXMax = 0, int nYMin = 0, int nYMax = 0);
	MinRectangle& operator = (MinRectangle &MBR);
	bool operator == (MinRectangle &MBR);

public:
	int m_nXMin;
	int m_nXMax;
	int m_nYMin;
	int m_nYMax;
};

//����λ�ã�Ӱ�����Ͻ�Ϊԭ��
class PixelPosition          
{
public:
	PixelPosition(int r = 0, int c = 0);
	PixelPosition& operator = (PixelPosition &p);    //���ظ�ֵ
	bool operator == (const PixelPosition &p);             //���ص���

public:
	int m_nRow;                          //��
	int m_nCol;                          //��
};

//���ڶ���
class NirObject                        
{
public:
	NirObject(int nID = -1, int ComBrdNum = 0); 
	bool operator == (const NirObject &n);               //���ص���

public:
	int m_nID;                                     //���ڶ���ID
	int m_nComBrdNum;                              //�����ڶ���Ĺ����߳�
};

//Ӱ�������
class Object
{
public:
	Object(int nID = 0);
	Object(Object &Obj);
	~Object();
	//��Ӱ���в���һ������
	void InsertPixel(int y, int x);
	void InsertPixel(PixelPosition &p);
	//��ȡӰ�����IDֵ
	int GetID() { return m_nID; }
	//��������Ӱ�����
	void InsertNirObj(int nID, int nComBrdNum);
	void InsertNirObj(NirObject &nObj);
	//�ж��Ƿ�Ϊ����Ӱ�����
	bool IsInNirObj(NirObject &nObj);
	bool IsInNirObj(int nID);
	//ɾ������Ӱ�����
	void DeleteNirObj(NirObject &nObj);
	void DeleteNirObj(int nID);
	//��ȡӰ�����������Ŀ
	int GetPixelNum();
	//�������ݽṹ˳���ȡӰ����������
	PixelPosition GetPixelByIndex(int nIndex);
	//��ȡ����Ӱ������б�
	vector<NirObject>& GetNirObjs();
	//��ȡӰ�������С������εĶ̱߳���
	float GetMinMBRLength();
	//��ȡӰ�������С�������
	MinRectangle GetMBR() { return m_mMBR; }
	//����Ӱ�������С�������
	void SetMBR(MinRectangle &mMBR);
	void SetMBR(int nXMin, int nXMax, int nYMin, int nYMax);
	//����Ӱ�����ľ�ֵ
	void SetMean(float *pMean, int nBandCount);
	//����Ӱ�����ı�׼��
	void SetStd(float *pStd, int nBandCount);
	//����Ӱ���������ֵ
	void SetMax(float *pMax, int nBandCount);
	//����Ӱ��������Сֵ
	void SetMin(float *pMin, int nBandCount);
	//��ȡӰ������ֵ
	float* GetMean() { return m_pMean; }
	//��ȡӰ������׼��
	float* GetStd() { return m_pStd; }
	//��ȡӰ��������ֵ
	float* GetMax() { return m_pMax; }
	//��ȡӰ�������Сֵ
	float* GetMin() { return m_pMin; }
	//����Ӱ����������
	void ComputeAtt(double *pData, int *pSegData, int nRasterXSize, int nRasterYSize, int nRasterCount);
	void ComputeAtt(GDALDataset *pImageDataset, GDALDataset* pFIDDataset, int nRasterCount);
	//��ȡ������Ӱ�����Ĺ����߳�
	int GetComBrdByID(int nID);
	//����������Ӱ�����Ĺ����߳�
	void SetComBrdByID(int nID, int nNum);
	//������Ӱ�����Ĺ����߳���1
	void AddOneComBrdNumByID(int nID);
	//�ж������Ƿ���Ӱ�������
	bool IsPixelIn(PixelPosition &p);
	bool IsPixelIn(int y, int x);
	//����Ӱ�������ܳ�
	void SetPerimeter(int nPerimeter) { m_nPerimeter = nPerimeter; }
	//��ȡӰ�������ܳ�
	int GetPerimeter() { return m_nPerimeter; }
	//��ȡ��Ӱ��ָ��һ�ε��������и�Ӱ������Ƿ񱻷ָ��
	bool GetIsSeged() { return m_bIsSeged; }
	//������Ӱ��ָ��һ�ε��������и�Ӱ������Ƿ񱻷ָ��
	void SetIsSeged(bool bIsSeged) { m_bIsSeged = bIsSeged; }
	//����Ӱ�����ľ�ֵ�ͱ�׼��
	void ComputeMeanStd(double *pData, int nRasterXSize, int nRasterYSize, int nRasterCount);
	void ComputeMeanStd(GDALDataset *pImageDataset, int nRasterCount);
	//����Ӱ����������
	void ComputeCenter();
	//��ȡӰ����������
	float GetCenterX() { return m_fXCenter; }
	float GetCenterY() { return m_fYCenter; }

	//��ȡӰ���������ǩ
	void SetClassLabel(int nClassLabel) { m_nClassLabel = nClassLabel; }
	int GetClassLabel() { return m_nClassLabel; }
	int GetThematicLabel() { return m_nThematicLabel; }
	void SetThematicLabel(int nThematicLabel) { m_nThematicLabel = nThematicLabel; }

	//����Ӱ�������ܳ�
	void ComputePerimeter(int *pSegData, int nRasterXSize, int nRasterYSize);
	void ComputePerimeter(GDALDataset *pFIDDataset);
	//����Ӱ��������С�������
	void ComputeMBR();

private:
	//Ӱ�����ID
	int m_nID;
	//Ӱ��������������
	vector<PixelPosition> m_vPixels;
	//Ӱ����������Ӱ�����
	vector<NirObject> m_vNirObjs;
	//Ӱ�������С�������
	MinRectangle m_mMBR;
	//Ӱ������ֵ
	float *m_pMean;
	//Ӱ������׼��
	float *m_pStd;
	//Ӱ��������ֵ
	float *m_pMax;
	//Ӱ�������Сֵ
	float *m_pMin;
	//Ӱ�������ܳ�
	int m_nPerimeter;
	//Ӱ������Ƿ񱻷ָ��
	bool m_bIsSeged;
	//Ӱ���������
	float m_fXCenter;
	float m_fYCenter;

	//Ӱ���������ǩ
	int m_nClassLabel;

	//����Լ����ǩ
	int m_nThematicLabel;

public:
	//�ָ�ԭʼӰ��Ĳ�����
	int m_nBandCount;
};
