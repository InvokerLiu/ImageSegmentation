#pragma once
#include "Object.h"

//某一个分割尺度的对象层
class ObjectLevel
{
public:
	ObjectLevel();
	ObjectLevel(ObjectLevel &ObjLevel);
	~ObjectLevel();
	//获取对象层中的一个影像对象
	Object *GetObjByID(int nID);
	//插入一个影像对象
	void InsertObj(Object *pObj);
	//从分割结果中创建一个对象层
	string CreateObjLevelFromLabel(double *pData, int *pSegData, int *pThematicData, int nRasterXSize, int nRasterYSize, int nRasterCount, bool bComputeNeighbor = true);
	string CreateObjLevelFromLabel(int *pSegData, int nRasterXSize, int nRasterYSize, bool bComputeNeighbor = false);

	//从多边形FID中创建对象层
	string CreateObjectLevelFromSolid(string sFIDFile);
	string CreateObjectLevelFromSolid(string sImageFile, string sFIDFile);
	//获取对象层中影像对象的数量
	int GetNumOfObjs() { return (int)m_vObjs.size(); }
	//将对象层保存为栅格文件
	void TransLevelToArray(int *pSegData, int nRasterXSize, int nRasterYSize);
	//将对象层保存为矢量文件
	//string SaveLevelToShp(string sShpFile);
	//计算对象层中所有影像对象的中心
	void ComputeCenter();
	//计算Label的最大最小值
	static void ComputeMaxMin(int *pSegData, int nSz, int &nMax, int &nMin);
	int GetMaxIndex(int *pData, int nNum);

private:
	//对象层中包含的影像对象列表
	vector<Object*> m_vObjs;
	//原始影像的波段数
	int m_nBandCount;
};
