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

//对象最小外包矩形
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

//像素位置，影像左上角为原点
class PixelPosition          
{
public:
	PixelPosition(int r = 0, int c = 0);
	PixelPosition& operator = (PixelPosition &p);    //重载赋值
	bool operator == (const PixelPosition &p);             //重载等于

public:
	int m_nRow;                          //行
	int m_nCol;                          //列
};

//相邻对象
class NirObject                        
{
public:
	NirObject(int nID = -1, int ComBrdNum = 0); 
	bool operator == (const NirObject &n);               //重载等于

public:
	int m_nID;                                     //相邻对象ID
	int m_nComBrdNum;                              //与相邻对象的公共边长
};

//影像对象类
class Object
{
public:
	Object(int nID = 0);
	Object(Object &Obj);
	~Object();
	//向影像中插入一个像素
	void InsertPixel(int y, int x);
	void InsertPixel(PixelPosition &p);
	//获取影像对象ID值
	int GetID() { return m_nID; }
	//插入相邻影像对象
	void InsertNirObj(int nID, int nComBrdNum);
	void InsertNirObj(NirObject &nObj);
	//判断是否为相邻影像对象
	bool IsInNirObj(NirObject &nObj);
	bool IsInNirObj(int nID);
	//删除相邻影像对象
	void DeleteNirObj(NirObject &nObj);
	void DeleteNirObj(int nID);
	//获取影像对象像素数目
	int GetPixelNum();
	//按照数据结构顺序获取影像对象的像素
	PixelPosition GetPixelByIndex(int nIndex);
	//获取相邻影像对象列表
	vector<NirObject>& GetNirObjs();
	//获取影像对象最小外包矩形的短边长度
	float GetMinMBRLength();
	//获取影像对象最小外包矩形
	MinRectangle GetMBR() { return m_mMBR; }
	//设置影像对象最小外包矩形
	void SetMBR(MinRectangle &mMBR);
	void SetMBR(int nXMin, int nXMax, int nYMin, int nYMax);
	//设置影像对象的均值
	void SetMean(float *pMean, int nBandCount);
	//设置影像对象的标准差
	void SetStd(float *pStd, int nBandCount);
	//设置影像对象的最大值
	void SetMax(float *pMax, int nBandCount);
	//设置影像对象的最小值
	void SetMin(float *pMin, int nBandCount);
	//获取影像对象均值
	float* GetMean() { return m_pMean; }
	//获取影像对象标准差
	float* GetStd() { return m_pStd; }
	//获取影像对象最大值
	float* GetMax() { return m_pMax; }
	//获取影像对象最小值
	float* GetMin() { return m_pMin; }
	//计算影像对象的属性
	void ComputeAtt(double *pData, int *pSegData, int nRasterXSize, int nRasterYSize, int nRasterCount);
	void ComputeAtt(GDALDataset *pImageDataset, GDALDataset* pFIDDataset, int nRasterCount);
	//获取与相邻影像对象的公共边长
	int GetComBrdByID(int nID);
	//设置与相邻影像对象的公共边长
	void SetComBrdByID(int nID, int nNum);
	//与相邻影像对象的公共边长加1
	void AddOneComBrdNumByID(int nID);
	//判断像素是否在影像对象内
	bool IsPixelIn(PixelPosition &p);
	bool IsPixelIn(int y, int x);
	//设置影像对象的周长
	void SetPerimeter(int nPerimeter) { m_nPerimeter = nPerimeter; }
	//获取影像对象的周长
	int GetPerimeter() { return m_nPerimeter; }
	//获取在影像分割的一次迭代过程中该影像对象是否被分割过
	bool GetIsSeged() { return m_bIsSeged; }
	//设置在影像分割的一次迭代过程中该影像对象是否被分割过
	void SetIsSeged(bool bIsSeged) { m_bIsSeged = bIsSeged; }
	//计算影像对象的均值和标准差
	void ComputeMeanStd(double *pData, int nRasterXSize, int nRasterYSize, int nRasterCount);
	void ComputeMeanStd(GDALDataset *pImageDataset, int nRasterCount);
	//计算影像对象的中心
	void ComputeCenter();
	//获取影像对象的中心
	float GetCenterX() { return m_fXCenter; }
	float GetCenterY() { return m_fYCenter; }

	//获取影像对象类别标签
	void SetClassLabel(int nClassLabel) { m_nClassLabel = nClassLabel; }
	int GetClassLabel() { return m_nClassLabel; }
	int GetThematicLabel() { return m_nThematicLabel; }
	void SetThematicLabel(int nThematicLabel) { m_nThematicLabel = nThematicLabel; }

	//计算影像对象的周长
	void ComputePerimeter(int *pSegData, int nRasterXSize, int nRasterYSize);
	void ComputePerimeter(GDALDataset *pFIDDataset);
	//计算影像对象的最小外包矩形
	void ComputeMBR();

private:
	//影像对象ID
	int m_nID;
	//影像对象包含的像素
	vector<PixelPosition> m_vPixels;
	//影像对象的相邻影像对象
	vector<NirObject> m_vNirObjs;
	//影像对象最小外包矩形
	MinRectangle m_mMBR;
	//影像对象均值
	float *m_pMean;
	//影像对象标准差
	float *m_pStd;
	//影像对象最大值
	float *m_pMax;
	//影像对象最小值
	float *m_pMin;
	//影像对象的周长
	int m_nPerimeter;
	//影像对象是否被分割过
	bool m_bIsSeged;
	//影像对象中心
	float m_fXCenter;
	float m_fYCenter;

	//影像对象类别标签
	int m_nClassLabel;

	//语义约束标签
	int m_nThematicLabel;

public:
	//分割原始影像的波段数
	int m_nBandCount;
};
