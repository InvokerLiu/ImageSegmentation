#pragma once

#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include "ObjectLevel.h"

//对象分割类
class ObjectSegmentation
{
public:
	ObjectSegmentation();
	~ObjectSegmentation();

	//执行影像分割
	string Execute(string sInputFile, string sOutputFile, string sRGBFile = "", string sThematicFile = "");
	//设置分割参数
	void SetParam(float fScale, float fWCompactness, float fWColor);
	

private:
	//计算两个对象合并的异质度
	float GetHeter(Object *pObj1, Object *pObj2, NirObject &NirObj);
	//分割影像
	ObjectLevel* SegImage(ObjectLevel *pInitLevel);
	//获取影像对象合并后的均值
	float* GetMerMean(Object *pObj1, Object *pObj2);
	//获取影像对象合并后的标准差
	float* GetMerStd(Object *pObj1, Object *pObj2);
	//获取影像对象合并后的最小外包矩形
	MinRectangle GetMerMBR(Object *pObj1, Object *pObj2);
	//获取一个影像对象与相邻影像最小的合并异质度及该相邻影像对象的索引
	void GetMinHeterAndIndex(Object *pObj, vector<NirObject> &vNirObjs, ObjectLevel *pCurrentLevel,
		ObjectLevel *pResultLevel, float &fMinHeter, int &nMinIndex);
	//依据异质度更新影像对象的属性
	void UpdateObjAttByHeter(Object *pObj, vector<NirObject> &vNirObjs, ObjectLevel *pCurrentLevel,
		ObjectLevel *pResultLevel, float fMinHeter, int nMinIndex, int i);
	//依据索引更新影像对象的属性
	void UpdateObjAttByIndex(Object *pObj, vector<NirObject>& vNirObjs, ObjectLevel *pCurrentLevel,
		ObjectLevel *pResultLevel, int nIndex, int i);


private:
	//分割尺度
	float m_fScale;
	//内部瞬时分割尺度
	float m_fInerScale;
	//分割紧凑度权重
	float m_fWCompactness;
	//分割光谱权重
	float m_fWColor;
	//原始影像波段数
	int m_nBandCount;
	//当前分割影像id
	int nObjID;
};