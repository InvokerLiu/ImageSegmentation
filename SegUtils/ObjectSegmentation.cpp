// DoLabObjectSegmentation.cpp : 定义 DLL 应用程序的导出函数。
//
#include "ObjectSegmentation.h"
#include "SLIC.h"

ObjectSegmentation::ObjectSegmentation()
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	m_fScale = m_fWColor = m_fWCompactness = 0.0f;
	m_fInerScale = 0.0f;
	nObjID = 0;
}

ObjectSegmentation::~ObjectSegmentation()
{

}

void ObjectSegmentation::SetParam(float fScale, float fWCompactness, float fWColor)
{
	m_fScale = fScale;
	m_fWCompactness = fWCompactness;
	m_fWColor = fWColor;
}

///InputFile：分割影像文件
///sOutputFile：分割输出文件
///sThematicFile：分割语义约束文件
string ObjectSegmentation::Execute(string sInputFile, string sOutputFile, string sRGBFile, string sThematicFile)
{
	GDALDataset *pDataset = (GDALDataset*)GDALOpen(sInputFile.c_str(), GA_ReadOnly);
	if (pDataset == NULL)
		return "打开文件" + sInputFile + "失败！";
	GDALDataset *pThematicDataset = NULL;
	if (sThematicFile != "")
	{
		pThematicDataset = (GDALDataset*)GDALOpen(sThematicFile.c_str(), GA_ReadOnly);
		if (pThematicDataset == NULL)
		{
			GDALClose(pDataset);
			return "打开文件" + sThematicFile + "失败！";
		}
		if (pDataset->GetRasterXSize() != pThematicDataset->GetRasterXSize() || pDataset->GetRasterYSize() != pThematicDataset->GetRasterYSize())
		{
			GDALClose(pDataset);
			GDALClose(pThematicDataset);
			return "文件" + sInputFile + "与文件" + sThematicFile + "的行列号不一致！";
		}
	}

	int nRasterXSize = pDataset->GetRasterXSize();
	int nRasterYSize = pDataset->GetRasterYSize();
	int nRasterCount = pDataset->GetRasterCount();
	m_nBandCount = nRasterCount;

	GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset *pSegDataset = pDriver->Create(sOutputFile.c_str(), nRasterXSize, nRasterYSize, 1, GDT_Int32, NULL);
	if (pSegDataset == NULL)
	{
		GDALClose(pDataset);
		return "创建文件" + sOutputFile + "失败！";
	}

	double *pGeoTransform = new double[6];
	pDataset->GetGeoTransform(pGeoTransform);
	pSegDataset->SetGeoTransform(pGeoTransform);
	string sProjectionRef = pDataset->GetProjectionRef();
	CPLErr error = pSegDataset->SetProjection(sProjectionRef.c_str());
	pSegDataset->GetRasterBand(1)->SetNoDataValue(SegNoDataValue);

	//将原始影像转换为RGB图片用于SLIC初始分割
	if (sRGBFile == "")
	{
		sRGBFile = sInputFile.substr(0, sInputFile.length() - 4) + "RGB.tif";
		cout << "生成RGB图像..." << endl;
		ifstream in(sRGBFile.c_str());
		if (in.good() == false)
		{
			string sResult = "";
			sResult = SLIC::CreateRGBImage(sInputFile, sRGBFile);
			if (sResult != "")
				return sResult;
		}
	}
	GDALDataset *pRGBDataset = (GDALDataset*)GDALOpen(sRGBFile.c_str(), GA_ReadOnly);
	if (pRGBDataset == NULL)
		return "打开文件" + sRGBFile + "失败！";

	int nRow = nRasterYSize / MaxImageSize;
	if (nRow * MaxImageSize != nRasterYSize)
		nRow += 1;
	int nCol = nRasterXSize / MaxImageSize;
	if (nCol * MaxImageSize != nRasterXSize)
		nCol += 1;

	/**********************Debug Code*******************************/
	//string sTestSLIC = sOutputFile.substr(0, sOutputFile.size() - 4) + "SLIC.tif";
	//GDALDataset *pTestSLICDataset = pDriver->Create(sTestSLIC.c_str(), nRasterXSize, nRasterYSize, 1, GDT_Int32, NULL);
	//pTestSLICDataset->SetGeoTransform(pGeoTransform);
	//pTestSLICDataset->SetProjection(pDataset->GetProjectionRef());
	//pTestSLICDataset->GetRasterBand(1)->SetNoDataValue(SegNoDataValue);
	/***************************************************************/
	delete[]pGeoTransform;
	//原始影像数据
	double *pData = NULL;
	//语义约束数据
	int *pThematicData = NULL;
	//RGB图片数据
	int *pRGBData = NULL;
	//SLIC初始分割结果
	int *pInitSegData = NULL;
	//分割结果
	int *pSegData = NULL;

	SLIC slic;
	ObjectLevel *pInitLevel = NULL, *pResultLevel = NULL;

	int nBeginX = 0, nBeginY = 0;
	int nXSize = 0, nYSize = 0;
	int nIndex = 0;
	if (nRow * nCol > 1)
	{
		cout << "由于影像过大，将其分为" << nRow * nCol << "块进行处理！" << endl;
	}
	double dNoDataValue = pDataset->GetRasterBand(1)->GetNoDataValue();
	int nCurrentObjNum = 0, nTotalObjNum = 0;
	int nMax = 0, nMin = 0;
	int nSz = 0;
	for (int r = 0; r < nRow; r++)
	{
		for (int c = 0; c < nCol; c++)
		{
			
			nIndex = r * nCol + c + 1;
			cout << "正在处理第" << nIndex << "块..." << endl;
			nBeginX = c * MaxImageSize;
			nBeginY = r * MaxImageSize;
			nXSize = MaxImageSize;
			nYSize = MaxImageSize;
			if (nBeginX + nXSize > nRasterXSize)
				nXSize = nRasterXSize - nBeginX;
			if (nBeginY + nYSize > nRasterYSize)
				nYSize = nRasterYSize - nBeginY;
			//创建数组
			pData = new double[nXSize * nYSize * nRasterCount];
			pRGBData = new int[nXSize * nYSize * 3];
			pInitSegData = new int[nXSize * nYSize];
			pSegData = new int[nXSize * nYSize];
			
			//读取数据
			cout << "读取数据..." << endl;
			pDataset->RasterIO(GF_Read, nBeginX, nBeginY, nXSize, nYSize, pData, nXSize, nYSize, GDT_Float64, nRasterCount, NULL, 0, 0, 0);
			pRGBDataset->RasterIO(GF_Read, nBeginX, nBeginY, nXSize, nYSize, pRGBData, nXSize, nYSize, GDT_Int32, 3, NULL, 0, 0, 0);
			if (pThematicDataset != NULL)
			{
				pThematicData = new int[nXSize * nYSize];
				pThematicDataset->RasterIO(GF_Read, nBeginX, nBeginY, nXSize, nYSize, pThematicData, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);
			}
			//SLIC初始分割
			slic.SetInternalParameters(nXSize, nYSize);
			cout << "SLIC初始分割..." << endl;
			slic.Execute(pRGBData, pInitSegData, pThematicData);

			/**************************************************/
			//pTestSLICDataset->RasterIO(GF_Write, nBeginX, nBeginY, nXSize, nYSize, pInitSegData, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);
			/**************************************************/

			/**************中途输入SLIC数据****************/
			/*GDALDataset *pTestSLICDataset = (GDALDataset*)GDALOpen("D:\\SegTest\\SLIC.tif", GA_ReadOnly);
			pTestSLICDataset->RasterIO(GF_Read, nBeginX, nBeginY, nXSize, nYSize, pInitSegData, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);*/
			/**********************************************/
			pInitLevel = new ObjectLevel();
			cout << "生成影像对象..." << endl;
			pInitLevel->CreateObjLevelFromLabel(pData, pInitSegData, pThematicData, nXSize, nYSize, nRasterCount);

			cout << "多分辨率分割..." << endl;
			pResultLevel = SegImage(pInitLevel);
			cout << "存储分割结果..." << endl << endl;
			pResultLevel->TransLevelToArray(pSegData, nXSize, nYSize);

			//计算当前块有多少影像对象
			nSz = nXSize * nYSize;
			//将对象ID排序，同时去除NoDataValue值包含的区域
			slic.RefineSegResult(pSegData, pData, dNoDataValue, &nTotalObjNum);

			//防止影像对象数超过Int的最大值（一般不存在这么大的影像）
			if (nTotalObjNum > 2100000000)
				nTotalObjNum = 0;

			pSegDataset->RasterIO(GF_Write, nBeginX, nBeginY, nXSize, nYSize, pSegData, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);
			delete[]pData;
			delete[]pRGBData;
			delete[]pInitSegData;
			delete[]pSegData;
			delete pResultLevel;
			if (pThematicData)
			{
				delete[]pThematicData;
				pThematicData = NULL;
			}
		}
	}
	GDALClose(pDataset);
	GDALClose(pRGBDataset);
	GDALClose(pSegDataset);
	return "";
}

MinRectangle ObjectSegmentation::GetMerMBR(Object *pObj1, Object *pObj2)
{
	MinRectangle Rect = pObj1->GetMBR();
	MinRectangle mMBR = pObj2->GetMBR();
	if (mMBR.m_nXMax > Rect.m_nXMax)
		Rect.m_nXMax = mMBR.m_nXMax;
	if (mMBR.m_nXMin < Rect.m_nXMin)
		Rect.m_nXMin = mMBR.m_nXMin;
	if (mMBR.m_nYMax > Rect.m_nYMax)
		Rect.m_nYMax = mMBR.m_nYMax;
	if (mMBR.m_nYMin < Rect.m_nYMin)
		Rect.m_nYMin = mMBR.m_nYMin;
	return Rect;
}

float* ObjectSegmentation::GetMerMean(Object *pObj1, Object *pObj2)
{
	int nBandCount = pObj1->m_nBandCount;
	float *pMerMean = new float[nBandCount];
	for (int i = 0; i < nBandCount; i++)
	{
		pMerMean[i] = (pObj1->GetMean()[i] * pObj1->GetPixelNum() + pObj2->GetMean()[i] * pObj2->GetPixelNum())
			/ (pObj1->GetPixelNum() + pObj2->GetPixelNum());
	}
	return pMerMean;
}

float* ObjectSegmentation::GetMerStd(Object *pObj1, Object *pObj2)
{
	float *pMean1 = pObj1->GetMean();
	float *pMean2 = pObj2->GetMean();
	float *pStd1 = pObj1->GetStd();
	float *pStd2 = pObj2->GetStd();
	int nNum1 = pObj1->GetPixelNum();
	int nNum2 = pObj2->GetPixelNum();
	float fn = 1.0f / (nNum1 + nNum2);
	int nBandCount = pObj1->m_nBandCount;
	float *pMerStd = new float[nBandCount];
	for (int i = 0; i < nBandCount; i++)
	{
		pMerStd[i] = (float)(sqrt((nNum1 * pow(pStd1[i], 2) +
			nNum2 * pow(pStd2[i], 2)) * fn + nNum1 * nNum2 *
			pow(pMean1[i] - pMean2[i], 2) * pow(fn, 2)));
	}
	return pMerStd;
}

float ObjectSegmentation::GetHeter(Object *pObj1, Object *pObj2, NirObject &NirObj)
{
	float fHeter = 0.0f;
	int nPerimeter1 = pObj1->GetPerimeter(), nPerimeter2 = pObj2->GetPerimeter();
	int nMerPerimeter = nPerimeter1 + nPerimeter2 - 2 * NirObj.m_nComBrdNum;
	MinRectangle MerMinRect = GetMerMBR(pObj1, pObj2);
	float fMBRLength1 = pObj1->GetMinMBRLength();
	float fMBRLength2 = pObj2->GetMinMBRLength();
	float fMerMBRLength = (float)(MerMinRect.m_nXMax - MerMinRect.m_nXMin + 1);
	float fTemp = (float)(MerMinRect.m_nYMax - MerMinRect.m_nYMin + 1);
	if (fMerMBRLength > fTemp)
		fMerMBRLength = fTemp;
	int nPixelNum1 = pObj1->GetPixelNum(), nPixelNum2 = pObj2->GetPixelNum();
	int nMerPixelNum = nPixelNum1 + nPixelNum2;
	float fn = 1.0f / nMerPixelNum;
	float *pStd1 = pObj1->GetStd();
	float *pStd2 = pObj2->GetStd();
	float *pMerStd = GetMerStd(pObj1, pObj2);
	float fSmoothness = nMerPixelNum * nMerPerimeter / fMerMBRLength - nPixelNum1 * nPerimeter1 / fMBRLength1
		- nPixelNum2 * nPerimeter2 / fMBRLength2;
	float fCompactness = (float)(sqrt(nMerPixelNum) * nMerPerimeter - sqrt(nPixelNum1) * nPerimeter1
		- sqrt(nPixelNum2) * nPerimeter2);
	float fColor = 0.0f;
	int nBandCount = pObj1->m_nBandCount;
	for (int i = 0; i < nBandCount; i++)
	{
		fColor += (nMerPixelNum * pMerStd[i] - nPixelNum1 * pStd1[i] - nPixelNum2 * pStd2[i]);
	}
	delete[]pMerStd;
	fColor = fColor / nBandCount;
	float fShape = m_fWCompactness * fCompactness + (1 - m_fWCompactness) * fSmoothness;
	fHeter = m_fWColor * fColor + (1 - m_fWColor) * fShape;
	return fHeter;
}

ObjectLevel* ObjectSegmentation::SegImage(ObjectLevel *pInitLevel)
{
	if (pInitLevel->GetNumOfObjs() == 1)
		return pInitLevel;
	ObjectLevel *pCurrentLevel = pInitLevel;
	ObjectLevel *pResultLevel = new ObjectLevel();
	int nObjNumOfCurLevel = pCurrentLevel->GetNumOfObjs();
	int nNewObjNum = 0;
	nObjID = 0;
	float fMinHeter = 0.0f;
	int nMinIndex = 0;

	/***************Test*****************/
	//string sOutputDir = "D:\\SegTest\\TempData\\";
	//string sOutputFile = "";
	//GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	//int nXSize = 513, nYSize = 513;
	//int *pSegData = new int[nXSize * nYSize];
	//GDALDataset *pDataset = NULL;
	//int nCount = 1;
	//string sReferenceFile = "D:\\SegTest\\1.tif";
	//GDALDataset *pReferenceDataset = (GDALDataset*)GDALOpen(sReferenceFile.c_str(), GA_ReadOnly);
	//double *pTrans = new double[6];
	//pReferenceDataset->GetGeoTransform(pTrans);
	/************************************/

	int nIterCount = (int)m_fScale / 10;
	if (nIterCount * 10 != m_fScale)
		nIterCount += 1;

	for (int s = 1; s <= nIterCount; s++)
	{
		m_fInerScale = pow(s * 10, 2);
		if (s == nIterCount)
			m_fInerScale = pow(m_fScale, 2);

		if (s != 1)
		{
			delete pCurrentLevel;
			pCurrentLevel = pResultLevel;
			pResultLevel = new ObjectLevel();
		}
		nObjID = 0;
		nObjNumOfCurLevel = pCurrentLevel->GetNumOfObjs();
		nNewObjNum = 0;
		Object *pObj = NULL, *pNirObj = NULL;
		while (nNewObjNum < nObjNumOfCurLevel)
		{
			for (int i = 0; i < nObjNumOfCurLevel; i++)
			{
				pObj = pCurrentLevel->GetObjByID(i);
				if (pObj->GetIsSeged())
					continue;
				vector<NirObject> vNirObjs = pObj->GetNirObjs();
				GetMinHeterAndIndex(pObj, vNirObjs, pCurrentLevel, pResultLevel, fMinHeter, nMinIndex);
				/*if (vNirObjs[nMinIndex].m_nID < nObjID)
					pNirObj = pResultLevel->GetObjByID(vNirObjs[nMinIndex].m_nID);
				else
					pNirObj = pCurrentLevel->GetObjByID(vNirObjs[nMinIndex].m_nID);
				if (pObj->GetThematicLabel() != pNirObj->GetThematicLabel())
				{
					fMinHeter = FLT_MAX;
				}*/
				UpdateObjAttByHeter(pObj, vNirObjs, pCurrentLevel, pResultLevel, fMinHeter, nMinIndex, i);
			}
			nNewObjNum = pResultLevel->GetNumOfObjs();
			if (nNewObjNum == nObjNumOfCurLevel || nNewObjNum == 1)
				break;
			nObjNumOfCurLevel = nNewObjNum;
			nNewObjNum = 0;
			nObjID = 0;
			delete pCurrentLevel;
			pCurrentLevel = pResultLevel;
			pResultLevel = new ObjectLevel();

			/*****************************************/
			//pCurrentLevel->TransLevelToArray(pSegData, nXSize, nYSize);
			//sOutputFile = sOutputDir + to_string(nCount) + ".tif";
			//pDataset = pDriver->Create(sOutputFile.c_str(), nXSize, nYSize, 1, GDT_Int32, NULL);
			//pDataset->SetProjection(pReferenceDataset->GetProjectionRef());
			//pDataset->SetGeoTransform(pTrans);
			//pDataset->RasterIO(GF_Write, 0, 0, nXSize, nYSize, pSegData, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);
			//GDALClose(pDataset);
			//nCount++;
			/*****************************************/
		}
	}
	delete pCurrentLevel;
	/**********************/
	//delete[]pSegData;
	//delete[]pTrans;
	//GDALClose(pReferenceDataset);
	/**********************/
	return pResultLevel;
}

void ObjectSegmentation::GetMinHeterAndIndex(Object *pObj, vector<NirObject> &vNirObjs, ObjectLevel *pCurrentLevel,
	ObjectLevel *pResultLevel, float &fMinHeter, int &nMinIndex)
{
	float fHeter = 0.0f;
	fMinHeter = FLT_MAX;
	nMinIndex = 0;
	for (int j = 0; j < (int)vNirObjs.size(); j++)
	{
		Object *pNirObj = NULL;
		if (vNirObjs[j].m_nID < nObjID)
			pNirObj = pResultLevel->GetObjByID(vNirObjs[j].m_nID);
		else
			pNirObj = pCurrentLevel->GetObjByID(vNirObjs[j].m_nID);
		fHeter = GetHeter(pObj, pNirObj, vNirObjs[j]);
		if (fHeter < fMinHeter && pObj->GetThematicLabel() == pNirObj->GetThematicLabel())
		{
			fMinHeter = fHeter;
			nMinIndex = j;
		}
	}
}

void ObjectSegmentation::UpdateObjAttByHeter(Object *pObj, vector<NirObject> &vNirObjs, ObjectLevel *pCurrentLevel,
	ObjectLevel *pResultLevel, float fMinHeter, int nMinIndex, int i)
{
	int nComBrdNum = 0;
	int nPixelNumOfObj = 0;
	int nBandCount = m_nBandCount;
	if (fMinHeter > m_fInerScale)
	{
		Object *pNewObj = new Object(nObjID);
		for (int j = 0; j < (int)vNirObjs.size(); j++)
		{
			pNewObj->InsertNirObj(vNirObjs[j]);
			Object *pTempObj = NULL;
			if (vNirObjs[j].m_nID < nObjID)
				pTempObj = pResultLevel->GetObjByID(vNirObjs[j].m_nID);
			else
				pTempObj = pCurrentLevel->GetObjByID(vNirObjs[j].m_nID);
			nComBrdNum = pTempObj->GetComBrdByID(i);
			pTempObj->DeleteNirObj(i);
			pTempObj->InsertNirObj(nObjID, nComBrdNum);
		}
		pNewObj->SetMBR(pObj->GetMBR());
		pNewObj->SetPerimeter(pObj->GetPerimeter());
		pNewObj->SetMean(pObj->GetMean(), nBandCount);
		pNewObj->SetStd(pObj->GetStd(), nBandCount);
		pNewObj->SetIsSeged(pObj->GetIsSeged());
		pNewObj->SetThematicLabel(pObj->GetThematicLabel());
		nPixelNumOfObj = pObj->GetPixelNum();
		pNewObj->m_nBandCount = pObj->m_nBandCount;
		for (int i = 0; i < nPixelNumOfObj; i++)
		{
			pNewObj->InsertPixel(pObj->GetPixelByIndex(i));
		}
		pResultLevel->InsertObj(pNewObj);
		nObjID++;
		return;
	}
	if (vNirObjs[nMinIndex].m_nID < nObjID)
	{
		Object *pNewObj = pResultLevel->GetObjByID(vNirObjs[nMinIndex].m_nID);
		pNewObj->DeleteNirObj(i);
		MinRectangle MerMBR = GetMerMBR(pObj, pNewObj);
		pNewObj->SetMBR(MerMBR);
		float *pMerMean = GetMerMean(pObj, pNewObj);
		float *pMerStd = GetMerStd(pObj, pNewObj);
		pNewObj->SetMean(pMerMean, nBandCount);
		pNewObj->SetStd(pMerStd, nBandCount);
		pNewObj->m_nBandCount = pObj->m_nBandCount;
		delete[]pMerMean;
		delete[]pMerStd;
		int nPerimeter = pObj->GetPerimeter() + pNewObj->GetPerimeter()
			- 2 * vNirObjs[nMinIndex].m_nComBrdNum;
		pNewObj->SetPerimeter(nPerimeter);
		for (int j = 0; j < (int)vNirObjs.size(); j++)
		{
			if (j != nMinIndex)
			{
				if (!pNewObj->IsInNirObj(vNirObjs[j]))
					pNewObj->InsertNirObj(vNirObjs[j]);
				else
					pNewObj->SetComBrdByID(vNirObjs[j].m_nID,
						pObj->GetComBrdByID(vNirObjs[j].m_nID) +
						pNewObj->GetComBrdByID(vNirObjs[j].m_nID));
				Object *pTempObj = NULL;
				if (vNirObjs[j].m_nID < nObjID)
					pTempObj = pResultLevel->GetObjByID(vNirObjs[j].m_nID);
				else
					pTempObj = pCurrentLevel->GetObjByID(vNirObjs[j].m_nID);
				nComBrdNum = pTempObj->GetComBrdByID(i);
				pTempObj->DeleteNirObj(i);
				if (!pTempObj->IsInNirObj(vNirObjs[nMinIndex]))
					pTempObj->InsertNirObj(vNirObjs[nMinIndex].m_nID, nComBrdNum);
				else
					pTempObj->SetComBrdByID(vNirObjs[nMinIndex].m_nID,
						nComBrdNum + pTempObj->GetComBrdByID(vNirObjs[nMinIndex].m_nID));
			}
		}
		nPixelNumOfObj = pObj->GetPixelNum();
		for (int j = 0; j < nPixelNumOfObj; j++)
		{
			pNewObj->InsertPixel(pObj->GetPixelByIndex(j));
		}
	}
	else
	{
		Object *pNewObj = new Object(nObjID);
		Object *pNirObj = pCurrentLevel->GetObjByID(vNirObjs[nMinIndex].m_nID);
		pNewObj->SetMBR(GetMerMBR(pObj, pNirObj));
		pNewObj->SetThematicLabel(pObj->GetThematicLabel());
		int nPerimeter = pObj->GetPerimeter() + pNirObj->GetPerimeter()
			- 2 * vNirObjs[nMinIndex].m_nComBrdNum;
		pNewObj->SetPerimeter(nPerimeter);
		float *pMean = GetMerMean(pObj, pNirObj);
		float *pStd = GetMerStd(pObj, pNirObj);
		pNewObj->SetMean(pMean, nBandCount);
		pNewObj->SetStd(pStd, nBandCount);
		pNewObj->m_nBandCount = pObj->m_nBandCount;
		delete[]pMean;
		delete[]pStd;
		for (int j = 0; j < (int)vNirObjs.size(); j++)
		{
			if (j != nMinIndex)
			{
				if (!pNewObj->IsInNirObj(vNirObjs[j]))
					pNewObj->InsertNirObj(vNirObjs[j]);
				Object* pTempObj = NULL;
				if (vNirObjs[j].m_nID < nObjID)
					pTempObj = pResultLevel->GetObjByID(vNirObjs[j].m_nID);
				else
					pTempObj = pCurrentLevel->GetObjByID(vNirObjs[j].m_nID);
				nComBrdNum = pTempObj->GetComBrdByID(i);
				pTempObj->DeleteNirObj(i);
				if (!pTempObj->IsInNirObj(nObjID))
					pTempObj->InsertNirObj(nObjID, nComBrdNum);
			}
		}
		vector<NirObject> vNirNirObjs = pNirObj->GetNirObjs();
		for (int j = 0; j < (int)vNirNirObjs.size(); j++)
		{
			if (vNirNirObjs[j].m_nID != i)
			{
				if (!pNewObj->IsInNirObj(vNirNirObjs[j]))
					pNewObj->InsertNirObj(vNirNirObjs[j]);
				else
					pNewObj->SetComBrdByID(vNirNirObjs[j].m_nID,
						pNewObj->GetComBrdByID(vNirNirObjs[j].m_nID) + vNirNirObjs[j].m_nComBrdNum);
				Object *pTempObj = NULL;
				if (vNirNirObjs[j].m_nID < nObjID)
					pTempObj = pResultLevel->GetObjByID(vNirNirObjs[j].m_nID);
				else
					pTempObj = pCurrentLevel->GetObjByID(vNirNirObjs[j].m_nID);
				nComBrdNum = pTempObj->GetComBrdByID(vNirObjs[nMinIndex].m_nID);
				pTempObj->DeleteNirObj(vNirObjs[nMinIndex]);
				if (!pTempObj->IsInNirObj(nObjID))
					pTempObj->InsertNirObj(NirObject(nObjID, nComBrdNum));
				else
					pTempObj->SetComBrdByID(nObjID, nComBrdNum + pTempObj->GetComBrdByID(nObjID));
			}
		}
		nPixelNumOfObj = pObj->GetPixelNum();
		for (int j = 0; j < nPixelNumOfObj; j++)
			pNewObj->InsertPixel(pObj->GetPixelByIndex(j));
		nPixelNumOfObj = pNirObj->GetPixelNum();
		for (int j = 0; j < nPixelNumOfObj; j++)
			pNewObj->InsertPixel(pNirObj->GetPixelByIndex(j));
		pNirObj->SetIsSeged(true);
		pResultLevel->InsertObj(pNewObj);
		nObjID++;
	}
}

void ObjectSegmentation::UpdateObjAttByIndex(Object *pObj, vector<NirObject>& vNirObjs, ObjectLevel *pCurrentLevel,
	ObjectLevel *pResultLevel, int nIndex, int i)
{
	int nComBrdNum = 0;
	int nPixelNumOfObj = 0;
	int nBandCount = m_nBandCount;
	if (nIndex == -1)
	{
		Object *pNewObj = new Object(nObjID);
		for (int j = 0; j < (int)vNirObjs.size(); j++)
		{
			pNewObj->InsertNirObj(vNirObjs[j]);
			Object *pTempObj = NULL;
			if (vNirObjs[j].m_nID < nObjID)
				pTempObj = pResultLevel->GetObjByID(vNirObjs[j].m_nID);
			else
				pTempObj = pCurrentLevel->GetObjByID(vNirObjs[j].m_nID);
			nComBrdNum = pTempObj->GetComBrdByID(i);
			pTempObj->DeleteNirObj(i);
			pTempObj->InsertNirObj(nObjID, nComBrdNum);
		}
		pNewObj->SetMBR(pObj->GetMBR());
		pNewObj->SetPerimeter(pObj->GetPerimeter());
		pNewObj->SetMean(pObj->GetMean(), nBandCount);
		pNewObj->SetStd(pObj->GetStd(), nBandCount);
		pNewObj->SetIsSeged(pObj->GetIsSeged());
		nPixelNumOfObj = pObj->GetPixelNum();
		pNewObj->m_nBandCount = pObj->m_nBandCount;
		for (int j = 0; j < nPixelNumOfObj; j++)
		{
			pNewObj->InsertPixel(pObj->GetPixelByIndex(j));
		}
		pResultLevel->InsertObj(pNewObj);
		nObjID++;
		return;
	}
	if (vNirObjs[nIndex].m_nID < nObjID)
	{
		Object *pNewObj = pResultLevel->GetObjByID(vNirObjs[nIndex].m_nID);
		pNewObj->DeleteNirObj(i);
		MinRectangle MerMBR = GetMerMBR(pObj, pNewObj);
		pNewObj->SetMBR(MerMBR);
		float *pMerMean = GetMerMean(pObj, pNewObj);
		float *pMerStd = GetMerStd(pObj, pNewObj);
		pNewObj->SetMean(pMerMean, nBandCount);
		pNewObj->SetStd(pMerStd, nBandCount);
		pNewObj->m_nBandCount = pObj->m_nBandCount;
		delete[]pMerMean;
		delete[]pMerStd;
		int nPerimeter = pObj->GetPerimeter() + pNewObj->GetPerimeter()
			- 2 * vNirObjs[nIndex].m_nComBrdNum;
		pNewObj->SetPerimeter(nPerimeter);
		for (int j = 0; j < (int)vNirObjs.size(); j++)
		{
			if (j != nIndex)
			{
				if (!pNewObj->IsInNirObj(vNirObjs[j]))
					pNewObj->InsertNirObj(vNirObjs[j]);
				else
					pNewObj->SetComBrdByID(vNirObjs[j].m_nID,
						pObj->GetComBrdByID(vNirObjs[j].m_nID) +
						pNewObj->GetComBrdByID(vNirObjs[j].m_nID));
				Object *pTempObj = NULL;
				if (vNirObjs[j].m_nID < nObjID)
					pTempObj = pResultLevel->GetObjByID(vNirObjs[j].m_nID);
				else
					pTempObj = pCurrentLevel->GetObjByID(vNirObjs[j].m_nID);
				nComBrdNum = pTempObj->GetComBrdByID(i);
				pTempObj->DeleteNirObj(i);
				if (!pTempObj->IsInNirObj(vNirObjs[nIndex]))
					pTempObj->InsertNirObj(vNirObjs[nIndex].m_nID, nComBrdNum);
				else
					pTempObj->SetComBrdByID(vNirObjs[nIndex].m_nID,
						nComBrdNum + pTempObj->GetComBrdByID(vNirObjs[nIndex].m_nID));
			}
		}
		nPixelNumOfObj = pObj->GetPixelNum();
		for (int j = 0; j < nPixelNumOfObj; j++)
		{
			pNewObj->InsertPixel(pObj->GetPixelByIndex(j));
		}
	}
	else
	{
		Object *pNewObj = new Object(nObjID);
		Object *pNirObj = pCurrentLevel->GetObjByID(vNirObjs[nIndex].m_nID);
		pNewObj->SetMBR(GetMerMBR(pObj, pNirObj));
		int nPerimeter = pObj->GetPerimeter() + pNirObj->GetPerimeter()
			- 2 * vNirObjs[nIndex].m_nComBrdNum;
		pNewObj->SetPerimeter(nPerimeter);
		float *pMean = GetMerMean(pObj, pNirObj);
		float *pStd = GetMerStd(pObj, pNirObj);
		pNewObj->SetMean(pMean, nBandCount);
		pNewObj->SetStd(pStd, nBandCount);
		pNewObj->m_nBandCount = pObj->m_nBandCount;
		delete[]pMean;
		delete[]pStd;
		for (int j = 0; j < (int)vNirObjs.size(); j++)
		{
			if (j != nIndex)
			{
				if (!pNewObj->IsInNirObj(vNirObjs[j]))
					pNewObj->InsertNirObj(vNirObjs[j]);
				Object* pTempObj = NULL;
				if (vNirObjs[j].m_nID < nObjID)
					pTempObj = pResultLevel->GetObjByID(vNirObjs[j].m_nID);
				else
					pTempObj = pCurrentLevel->GetObjByID(vNirObjs[j].m_nID);
				nComBrdNum = pTempObj->GetComBrdByID(i);
				pTempObj->DeleteNirObj(i);
				if (!pTempObj->IsInNirObj(nObjID))
					pTempObj->InsertNirObj(nObjID, nComBrdNum);
			}
		}
		vector<NirObject> vNirNirObjs = pNirObj->GetNirObjs();
		for (int j = 0; j < (int)vNirNirObjs.size(); j++)
		{
			if (vNirNirObjs[j].m_nID != i)
			{
				if (!pNewObj->IsInNirObj(vNirNirObjs[j]))
					pNewObj->InsertNirObj(vNirNirObjs[j]);
				else
					pNewObj->SetComBrdByID(vNirNirObjs[j].m_nID,
						pNewObj->GetComBrdByID(vNirNirObjs[j].m_nID) + vNirNirObjs[j].m_nComBrdNum);
				Object *pTempObj = NULL;
				if (vNirNirObjs[j].m_nID < nObjID)
					pTempObj = pResultLevel->GetObjByID(vNirNirObjs[j].m_nID);
				else
					pTempObj = pCurrentLevel->GetObjByID(vNirNirObjs[j].m_nID);
				nComBrdNum = pTempObj->GetComBrdByID(vNirObjs[nIndex].m_nID);
				pTempObj->DeleteNirObj(vNirObjs[nIndex]);
				if (!pTempObj->IsInNirObj(nObjID))
					pTempObj->InsertNirObj(NirObject(nObjID, nComBrdNum));
				else
					pTempObj->SetComBrdByID(nObjID, nComBrdNum + pTempObj->GetComBrdByID(nObjID));
			}
		}
		nPixelNumOfObj = pObj->GetPixelNum();
		for (int j = 0; j < nPixelNumOfObj; j++)
			pNewObj->InsertPixel(pObj->GetPixelByIndex(j));
		nPixelNumOfObj = pNirObj->GetPixelNum();
		for (int j = 0; j < nPixelNumOfObj; j++)
			pNewObj->InsertPixel(pNirObj->GetPixelByIndex(j));
		pNirObj->SetIsSeged(true);
		pResultLevel->InsertObj(pNewObj);
		nObjID++;
	}
}