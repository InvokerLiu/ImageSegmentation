#include "ObjectLevel.h"

ObjectLevel::ObjectLevel()
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
}

ObjectLevel::ObjectLevel(ObjectLevel &ObjLevel)
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	m_nBandCount = ObjLevel.m_nBandCount;
	for (int i = 0; i < ObjLevel.GetNumOfObjs(); i++)
	{
		InsertObj(new Object(*ObjLevel.m_vObjs[i]));
	}
}

ObjectLevel::~ObjectLevel()
{
	vector<Object*>::iterator it;
	for (it = m_vObjs.begin(); it != m_vObjs.end(); ++it)
	{
		delete *it;
	}
	m_vObjs.clear();
	m_vObjs.reserve(1);
}

void ObjectLevel::InsertObj(Object *pObj)
{
	m_vObjs.push_back(pObj);
}

Object* ObjectLevel::GetObjByID(int nID)
{
	if (nID >= 0 && nID < GetNumOfObjs())
		return m_vObjs[nID];
	else
		return NULL;
}

string ObjectLevel::CreateObjLevelFromLabel(double *pData, int *pSegData, int *pThematicData, int nRasterXSize, int nRasterYSize, int nRasterCount, bool bComputeNeighbor)
{
	int nSz = nRasterXSize * nRasterYSize;
	m_nBandCount = nRasterCount;
	int nMax = 0, nMin = 0;
	Object *pObj = NULL;
	ComputeMaxMin(pSegData, nSz, nMax, nMin);
	if (nMax == INT_MIN || nMin == INT_MAX)
		return "";
	int ObjNum = nMax - nMin + 1;

	for (int i = 0; i < nSz; i++)
	{
		if (pSegData[i] >= 0)
			pSegData[i] = pSegData[i] - nMin;
	}

	for (int i = 0; i < ObjNum; i++)
	{
		InsertObj(new Object(i));
	}

	int nIndex = 0;
	for (int y = 0; y < nRasterYSize; y++)
	{
		for (int x = 0; x < nRasterXSize; x++)
		{
			nIndex = y * nRasterXSize + x;
			if (pSegData[nIndex] >= 0)
			{
				GetObjByID(pSegData[nIndex])->InsertPixel(y, x);
			}
		}
	}
	for (int i = 0; i < ObjNum; i++)
	{
		pObj = GetObjByID(i);
		pObj->ComputeAtt(pData, pSegData, nRasterXSize, nRasterYSize, nRasterCount);
		pObj->m_nBandCount = nRasterCount;
	}
	int nNumOfObjs = GetNumOfObjs();
	int nPixelNum = 0;
	PixelPosition p;
	
	if (pThematicData)
	{
		int *pCount = new int[256];
		for (int n = 0; n < nNumOfObjs; n++)
		{
			pObj = m_vObjs[n];
			for (int i = 0; i < 256; i++)
				pCount[i] = 0;
			nPixelNum = pObj->GetPixelNum();
			for (int i = 0; i < nPixelNum; i++)
			{
				p = pObj->GetPixelByIndex(i);
				pCount[pThematicData[p.m_nRow * nRasterXSize + p.m_nCol]]++;
			}
			pObj->SetThematicLabel(GetMaxIndex(pCount, 256));
		}
		delete[]pCount;
	}
	if (bComputeNeighbor)
	{
		nIndex = -1;
		const int dx4[4] = { -1, 0, 1, 0 };
		const int dy4[4] = { 0, -1, 0, 1 };
		int x = 0, y = 0;
		for (int n = 0; n < nNumOfObjs; n++)
		{
			pObj = m_vObjs[n];
			nPixelNum = pObj->GetPixelNum();
			for (int i = 0; i < nPixelNum; i++)
			{
				p = pObj->GetPixelByIndex(i);
				for (int k = 0; k < 4; k++)
				{
					x = p.m_nCol + dx4[k];
					y = p.m_nRow + dy4[k];
					if (x >= 0 && x < nRasterXSize && y >= 0 && y < nRasterYSize)
					{
						nIndex = y * nRasterXSize + x;
						if (pSegData[nIndex] != n && pSegData[nIndex] >= 0)
						{
							if (!pObj->IsInNirObj(pSegData[nIndex]))
								pObj->InsertNirObj(pSegData[nIndex], 0);
							pObj->AddOneComBrdNumByID(pSegData[nIndex]);
						}
					}
				}
			}
		}
	}
	return "";
}

string ObjectLevel::CreateObjLevelFromLabel(int *pSegData, int nRasterXSize, int nRasterYSize, bool bComputeNeighbor)
{
	int nSz = nRasterXSize * nRasterYSize;
	int nMax = 0, nMin = 0;
	ComputeMaxMin(pSegData, nSz, nMax, nMin);
	if (nMax == INT_MIN || nMin == INT_MAX)
		return "";
	int ObjNum = nMax - nMin + 1;
	for (int i = 0; i < nSz; i++)
	{
		if(pSegData[i] >= 0)
			pSegData[i] = pSegData[i] - nMin;
	}
	for (int i = 0; i < ObjNum; i++)
	{
		InsertObj(new Object(i));
	}
	int nIndex = 0;
	for (int y = 0; y < nRasterYSize; y++)
	{
		for (int x = 0; x < nRasterXSize; x++)
		{
			nIndex = y * nRasterXSize + x;
			if (pSegData[nIndex] >= 0)
			{
				GetObjByID(pSegData[nIndex])->InsertPixel(y, x);
			}
		}
	}

	if (bComputeNeighbor)
	{
		nIndex = -1;
		const int dx4[4] = { -1, 0, 1, 0 };
		const int dy4[4] = { 0, -1, 0, 1 };
		int nNumOfObjs = GetNumOfObjs();
		int x = 0, y = 0;
		PixelPosition p;
		for (int n = 0; n < nNumOfObjs; n++)
		{
			Object *pObj = m_vObjs[n];
			int nPixelNum = pObj->GetPixelNum();
			for (int i = 0; i < nPixelNum; i++)
			{
				p = pObj->GetPixelByIndex(i);
				for (int k = 0; k < 4; k++)
				{
					x = p.m_nCol + dx4[k];
					y = p.m_nRow + dy4[k];
					if (x >= 0 && x < nRasterXSize && y >= 0 && y < nRasterYSize)
					{
						nIndex = y * nRasterXSize + x;
						if (pSegData[nIndex] != n && pSegData[nIndex] >= 0)
						{
							if (!pObj->IsInNirObj(pSegData[nIndex]))
								pObj->InsertNirObj(pSegData[nIndex], 0);
							pObj->AddOneComBrdNumByID(pSegData[nIndex]);
						}
					}
				}
			}
		}
	}
	return "";
}

string ObjectLevel::CreateObjectLevelFromSolid(string sFIDFile)
{
	GDALDataset *pFIDDataset = (GDALDataset*)GDALOpen(sFIDFile.c_str(), GA_ReadOnly);
	if (pFIDDataset == NULL)
	{
		return "读取影像" + sFIDFile + "失败！";
	}
	int nRasterXSize = pFIDDataset->GetRasterXSize();
	int nRasterYSize = pFIDDataset->GetRasterYSize();
	int *pKLabels = new int[nRasterXSize];

	int nNoDataValue = (int)pFIDDataset->GetRasterBand(1)->GetNoDataValue();
	GDALRasterBand *pBand = pFIDDataset->GetRasterBand(1);
	double min, max;
	if (pBand->GetStatistics(false, false, &min, &max, NULL, NULL) != CE_None)
		pBand->ComputeStatistics(false, &min, &max, NULL, NULL, NULL, NULL);
	int ObjNum = (int)(max - min) + 1;
	int nMin = (int)min;
	for (int i = 0; i < ObjNum; i++)
	{
		InsertObj(new Object(i));
	}
	int nPercent = 0;
	cout << "0%";
	for (int y = 0; y < nRasterYSize; y++)
	{
		pFIDDataset->RasterIO(GF_Read, 0, y, nRasterXSize, 1, pKLabels, nRasterXSize, 1,
			GDT_Int32, 1, NULL, 0, 0, 0);
		for (int x = 0; x < nRasterXSize; x++)
		{
			if (pKLabels[x] != nNoDataValue)
				GetObjByID(pKLabels[x] - nMin)->InsertPixel(y, x);
		}
		nPercent = y * 100 / nRasterYSize;
		cout << "\r" << nPercent << "%";
	}
	cout << "\r100%" << endl;
	GDALClose(pFIDDataset);
	delete[]pKLabels;
	return "";
}

string ObjectLevel::CreateObjectLevelFromSolid(string sImageFile, string sFIDFile)
{
	GDALDataset *pFIDDataset = (GDALDataset*)GDALOpen(sFIDFile.c_str(), GA_ReadOnly);
	if (pFIDDataset == NULL)
	{
		return "读取影像" + sFIDFile + "失败！";
	}
	GDALDataset *pImageDataset = (GDALDataset*)GDALOpen(sImageFile.c_str(), GA_ReadOnly);
	if (pImageDataset == NULL)
	{
		GDALClose(pFIDDataset);
		return "读取影像" + sImageFile + "失败！";
	}
	int nRasterXSize = pFIDDataset->GetRasterXSize();
	int nRasterYSize = pFIDDataset->GetRasterYSize();
	int nRasterCount = pImageDataset->GetRasterCount();
	int *pKLabels = new int[nRasterXSize];

	int nNoDataValue = (int)pFIDDataset->GetRasterBand(1)->GetNoDataValue();
	GDALRasterBand *pBand = pFIDDataset->GetRasterBand(1);
	double min, max;
	if (pBand->GetStatistics(false, false, &min, &max, NULL, NULL) != CE_None)
		pBand->ComputeStatistics(false, &min, &max, NULL, NULL, NULL, NULL);
	int ObjNum = (int)(max - min) + 1;
	int nMin = (int)min;
	for (int i = 0; i < ObjNum; i++)
	{
		InsertObj(new Object(i));
	}
	int nPercent = 0;
	cout << "0%";
	for (int y = 0; y < nRasterYSize; y++)
	{
		pFIDDataset->RasterIO(GF_Read, 0, y, nRasterXSize, 1, pKLabels, nRasterXSize, 1,
			GDT_Int32, 1, NULL, 0, 0, 0);
		for (int x = 0; x < nRasterXSize; x++)
		{
			if (pKLabels[x] != nNoDataValue)
				GetObjByID(pKLabels[x] - nMin)->InsertPixel(y, x);
		}
		nPercent = y * 100 / nRasterYSize;
		cout << "\r" << nPercent << "%";
	}
	cout << "\r100%" << endl;
	
	delete[]pKLabels;
	cout << "计算对象属性..." << endl;
	cout << "0%";
	for (int i = 0; i < ObjNum; i++)
	{
		Object *pObj = GetObjByID(i);
		pObj->ComputeAtt(pImageDataset, pFIDDataset, nRasterCount);
		pObj->m_nBandCount = nRasterCount;
		nPercent = i * 100 / ObjNum;
		cout << "\r" << nPercent << "%";
	}
	cout << "\r100%" << endl;
	cout << "计算对象属性完成！" << endl;
	GDALClose(pFIDDataset);
	GDALClose(pImageDataset);
	return "";
}


void ObjectLevel::ComputeMaxMin(int *pSegData, int nSz, int &nMax, int &nMin)
{
	nMax = INT_MIN;
	nMin = INT_MAX;
	for (int i = 0; i < nSz; i++)
	{
		if (pSegData[i] < 0)
			continue;
		if (pSegData[i] > nMax)
			nMax = pSegData[i];
		if (pSegData[i] < nMin)
			nMin = pSegData[i];
	}
}

void ObjectLevel::TransLevelToArray(int *pSegData, int nRasterXSize, int nRasterYSize)
{
	int nPixelNum = 0;
	PixelPosition p;
	for (int i = 0; i < GetNumOfObjs(); i++)
	{
		nPixelNum = m_vObjs[i]->GetPixelNum();
		for (int j = 0; j < nPixelNum; j++)
		{
			p = m_vObjs[i]->GetPixelByIndex(j);
			pSegData[p.m_nRow * nRasterXSize + p.m_nCol] = i;
		}
	}
}


//string ObjectLevel::SaveLevelToShp(string sShpFile)
//{
//	OGRRegisterAll();
//	CPLSetConfigOption("GDAL_FILENAME_IS_UFT8", "NO");
//	string sTiffFile = sShpFile + ".tif";
//	string sResult = SaveLevelToTiff(sTiffFile);
//	if (sResult != "正常结束！")
//		return sResult;
//	GDALDataset *poSrcDS = (GDALDataset*)GDALOpen(sTiffFile.c_str(), GA_ReadOnly);
//	if (poSrcDS == NULL)
//		return "打开栅格数据集失败！";
//	OGRSFDriver *poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("ESRI Shapefile");
//	if (poDriver == NULL)
//	{
//		GDALClose(poSrcDS);
//		remove(sTiffFile.c_str());
//		return "创建矢量驱动失败！";
//	}
//	OGRDataSource *poDstDS = poDriver->CreateDataSource(sShpFile.c_str());
//	if (poDstDS == NULL)
//	{
//		GDALClose(poSrcDS);
//		remove(sTiffFile.c_str());
//		return "创建矢量文件失败！";
//	}
//	OGRSpatialReference *poSpatialRef = new OGRSpatialReference(poSrcDS->GetProjectionRef());
//	OGRLayer *poLayer = poDstDS->CreateLayer("SceneSegmentation Result", poSpatialRef, wkbPolygon, NULL);
//	OGRFieldDefn ofieldDef("Segment", OFTInteger);
//	poLayer->CreateField(&ofieldDef);
//	GDALRasterBand *pBand = poSrcDS->GetRasterBand(1);
//	GDALPolygonize(pBand, NULL, (OGRLayerH)poLayer, 0, NULL, NULL, NULL);
//	GDALClose(poSrcDS);
//	remove(sTiffFile.c_str());
//	OGRDataSource::DestroyDataSource(poDstDS);
//	return "正常结束！";
//}


void ObjectLevel::ComputeCenter()
{
	for (int i = 0; i < GetNumOfObjs(); i++)
	{
		m_vObjs[i]->ComputeCenter();
	}
}


int ObjectLevel::GetMaxIndex(int *pData, int nNum)
{
	int nMax = pData[0];
	int nMaxIndex = 0;
	for (int i = 1; i < nNum; i++)
	{
		if (pData[i] > nMax)
		{
			nMax = pData[i];
			nMaxIndex = i;
		}
	}
	return nMaxIndex;
}