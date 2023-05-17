#pragma once

#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include "ObjectLevel.h"

//����ָ���
class ObjectSegmentation
{
public:
	ObjectSegmentation();
	~ObjectSegmentation();

	//ִ��Ӱ��ָ�
	string Execute(string sInputFile, string sOutputFile, string sRGBFile = "", string sThematicFile = "");
	//���÷ָ����
	void SetParam(float fScale, float fWCompactness, float fWColor);
	

private:
	//������������ϲ������ʶ�
	float GetHeter(Object *pObj1, Object *pObj2, NirObject &NirObj);
	//�ָ�Ӱ��
	ObjectLevel* SegImage(ObjectLevel *pInitLevel);
	//��ȡӰ�����ϲ���ľ�ֵ
	float* GetMerMean(Object *pObj1, Object *pObj2);
	//��ȡӰ�����ϲ���ı�׼��
	float* GetMerStd(Object *pObj1, Object *pObj2);
	//��ȡӰ�����ϲ������С�������
	MinRectangle GetMerMBR(Object *pObj1, Object *pObj2);
	//��ȡһ��Ӱ�����������Ӱ����С�ĺϲ����ʶȼ�������Ӱ����������
	void GetMinHeterAndIndex(Object *pObj, vector<NirObject> &vNirObjs, ObjectLevel *pCurrentLevel,
		ObjectLevel *pResultLevel, float &fMinHeter, int &nMinIndex);
	//�������ʶȸ���Ӱ����������
	void UpdateObjAttByHeter(Object *pObj, vector<NirObject> &vNirObjs, ObjectLevel *pCurrentLevel,
		ObjectLevel *pResultLevel, float fMinHeter, int nMinIndex, int i);
	//������������Ӱ����������
	void UpdateObjAttByIndex(Object *pObj, vector<NirObject>& vNirObjs, ObjectLevel *pCurrentLevel,
		ObjectLevel *pResultLevel, int nIndex, int i);


private:
	//�ָ�߶�
	float m_fScale;
	//�ڲ�˲ʱ�ָ�߶�
	float m_fInerScale;
	//�ָ���ն�Ȩ��
	float m_fWCompactness;
	//�ָ����Ȩ��
	float m_fWColor;
	//ԭʼӰ�񲨶���
	int m_nBandCount;
	//��ǰ�ָ�Ӱ��id
	int nObjID;
};