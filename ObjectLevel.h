#pragma once
#include "Object.h"

//ĳһ���ָ�߶ȵĶ����
class ObjectLevel
{
public:
	ObjectLevel();
	ObjectLevel(ObjectLevel &ObjLevel);
	~ObjectLevel();
	//��ȡ������е�һ��Ӱ�����
	Object *GetObjByID(int nID);
	//����һ��Ӱ�����
	void InsertObj(Object *pObj);
	//�ӷָ����д���һ�������
	string CreateObjLevelFromLabel(double *pData, int *pSegData, int *pThematicData, int nRasterXSize, int nRasterYSize, int nRasterCount, bool bComputeNeighbor = true);
	string CreateObjLevelFromLabel(int *pSegData, int nRasterXSize, int nRasterYSize, bool bComputeNeighbor = false);

	//�Ӷ����FID�д��������
	string CreateObjectLevelFromSolid(string sFIDFile);
	string CreateObjectLevelFromSolid(string sImageFile, string sFIDFile);
	//��ȡ�������Ӱ����������
	int GetNumOfObjs() { return (int)m_vObjs.size(); }
	//������㱣��Ϊդ���ļ�
	void TransLevelToArray(int *pSegData, int nRasterXSize, int nRasterYSize);
	//������㱣��Ϊʸ���ļ�
	//string SaveLevelToShp(string sShpFile);
	//��������������Ӱ����������
	void ComputeCenter();
	//����Label�������Сֵ
	static void ComputeMaxMin(int *pSegData, int nSz, int &nMax, int &nMin);
	int GetMaxIndex(int *pData, int nNum);

private:
	//������а�����Ӱ������б�
	vector<Object*> m_vObjs;
	//ԭʼӰ��Ĳ�����
	int m_nBandCount;
};
