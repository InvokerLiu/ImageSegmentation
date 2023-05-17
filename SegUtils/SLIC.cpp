// DoLabSLIC.cpp : 定义 DLL 应用程序的导出函数。
//
#include "SLIC.h"
#include "ObjectLevel.h"

SLIC::SLIC()
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	m_lvec = NULL;
	m_avec = NULL;
	m_bvec = NULL;
	m_nHeight = 0;
	m_nNumOfIter = 0;
	m_nSz = 0;
	m_nWidth = 0;
	m_nSuperPixelSize = 0;
	m_dCompactness = 0.0;
}

SLIC::~SLIC()
{
	if (m_lvec != NULL)
	{
		delete[]m_lvec;
		m_lvec = NULL;
	}
	if (m_avec != NULL)
	{
		delete[]m_avec;
		m_avec = NULL;
	}
	if (m_bvec != NULL)
	{
		delete[]m_bvec;
		m_bvec = NULL;
	}
	sigmaa.clear();
	sigmab.clear();
	sigmal.clear();
	sigmax.clear();
	sigmay.clear();
}

void SLIC::SetParameters(int nSuperpixelSize, double dCompactness, int nNumofIter)
{
	if (m_lvec != NULL)
	{
		delete[]m_lvec;
		m_lvec = NULL;
	}
	if (m_avec != NULL)
	{
		delete[]m_avec;
		m_avec = NULL;
	}
	if (m_bvec != NULL)
	{
		delete[]m_bvec;
		m_bvec = NULL;
	}
	sigmaa.clear();
	sigmab.clear();
	sigmal.clear();
	sigmax.clear();
	sigmay.clear();

	m_nSuperPixelSize = nSuperpixelSize;
	m_dCompactness = dCompactness;
	m_nNumOfIter = nNumofIter;
}

void SLIC::SetInternalParameters(int nWidth, int nHeight, int nSuperpixelSize, double dCompactness, int nNumofIter)
{
	if (m_lvec != NULL)
	{
		delete[]m_lvec;
		m_lvec = NULL;
	}
	if (m_avec != NULL)
	{
		delete[]m_avec;
		m_avec = NULL;
	}
	if (m_bvec != NULL)
	{
		delete[]m_bvec;
		m_bvec = NULL;
	}
	sigmaa.clear();
	sigmab.clear();
	sigmal.clear();
	sigmax.clear();
	sigmay.clear();

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nSz = m_nWidth * m_nHeight;
	m_nSuperPixelSize = nSuperpixelSize;
	m_dCompactness = dCompactness;
	m_nNumOfIter = nNumofIter;
}

string SLIC::Execute(string sInputFile, string sOutputFile, string sRGBFile, string sThematicFile)
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
	
	pSegDataset->SetProjection(pDataset->GetProjectionRef());
	pSegDataset->GetRasterBand(1)->SetNoDataValue(SegNoDataValue);

	//////*****************TestCode**************************/////
	//string sTempFile = sOutputFile.substr(0, sOutputFile.size() - 4) + "NoConnectivity.tif";
	//GDALDataset *pTempDataset = pDriver->Create(sTempFile.c_str(), nRasterXSize, nRasterYSize, 1, GDT_Int32, NULL);
	//pTempDataset->SetGeoTransform(pGeoTransform);
	//pTempDataset->SetProjection(pDataset->GetProjectionRef());
	//pTempDataset->GetRasterBand(1)->SetNoDataValue(SegNoDataValue);
	//int *pTempLabel = NULL;
	///////********************************************///////////////
	delete[]pGeoTransform;
	//将原始影像转换为RGB图片用于SLIC初始分割
	if (sRGBFile == "")
	{
		sRGBFile = sInputFile.substr(0, sInputFile.length() - 4) + "RGB.tif";
		cout << "生成RGB图像..." << endl;
		string sResult = "";
		sResult = SLIC::CreateRGBImage(sInputFile, sRGBFile);
		if (sResult != "")
			return sResult;
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
	//原始影像数据
	double *pData = NULL;
	//语义约束数据
	int *pThematicData = NULL;
	//RGB图片数据
	int *pRGBData = NULL;
	//SLIC初始分割结果
	int *pInitSegData = NULL;

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
			pRGBData = new int[nXSize * nYSize * 3];
			pInitSegData = new int[nXSize * nYSize];

			/***************************/
			//pTempLabel = new int[nXSize * nYSize];
			/**************************/
			pData = new double[nXSize * nYSize * nRasterCount];
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
			SetInternalParameters(nXSize, nYSize, m_nSuperPixelSize, m_dCompactness, m_nNumOfIter);
			cout << "SLIC初始分割..." << endl;
			Execute(pRGBData, pInitSegData, pThematicData);

			//计算当前块有多少影像对象
			nSz = nXSize * nYSize;

			//将对象ID排序，同时去除NoDataValue值包含的区域
			//RefineSegResult(pInitSegData, pData, dNoDataValue, &nTotalObjNum);

			//防止影像对象数超过Int的最大值（一般不存在这么大的影像）
			if (nTotalObjNum > 2100000000)
				nTotalObjNum = 0;

			pSegDataset->RasterIO(GF_Write, nBeginX, nBeginY, nXSize, nYSize, pInitSegData, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);

			/*****************************************************************************************************************************/
			//pTempDataset->RasterIO(GF_Write, nBeginX, nBeginY, nXSize, nYSize, pTempLabel, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);
			//delete[]pTempLabel;
			/***************************************************************************************************************************/

			delete[]pData;
			delete[]pRGBData;
			delete[]pInitSegData;
			if (pThematicData != NULL)
			{
				delete[]pThematicData;
				pThematicData = NULL;
			}
		}
	}
	GDALClose(pDataset);
	GDALClose(pRGBDataset);
	GDALClose(pSegDataset);
	//remove(sRGBFile.c_str());
	return "";
}

void SLIC::Execute(int *pData, int *pInitSegData, int *pThematicData)
{
	const int STEP = (int)(sqrt(double(m_nSuperPixelSize)) + 0.5);

	vector<double> kseedsl(0);
	vector<double> kseedsa(0);
	vector<double> kseedsb(0);
	vector<double> kseedsx(0);
	vector<double> kseedsy(0);

	for (int s = 0; s < m_nSz; s++)
		pInitSegData[s] = -1;

	if (m_lvec != NULL)
		delete[] m_lvec;
	if (m_avec != NULL)
		delete[] m_avec;
	if (m_bvec != NULL)
		delete[]m_bvec;

	m_lvec = new double[m_nSz];
	m_avec = new double[m_nSz];
	m_bvec = new double[m_nSz];

	DoRGBtoLABConversion(pData, m_lvec, m_avec, m_bvec);
	
	bool perturbseeds(false);//perturb seeds is not absolutely necessary, one can set this flag to false
	vector<double> edgemag(0);
	if (perturbseeds)
		DetectLabEdges(m_lvec, m_avec, m_bvec, edgemag);

	GetLABXYSeeds_ForGivenStepSize(kseedsl, kseedsa, kseedsb, kseedsx, kseedsy, STEP, perturbseeds, edgemag);
	
	PerformSuperpixelSLIC(kseedsl, kseedsa, kseedsb, kseedsx, kseedsy, pInitSegData, pThematicData, STEP, edgemag, m_dCompactness);

	kseedsa.clear();
	kseedsb.clear();
	kseedsl.clear();
	kseedsx.clear();
	kseedsy.clear();

	int numlabels = kseedsl.size();
	int* nlabels = new int[m_nSz];

	EnforceLabelConnectivity(pInitSegData, nlabels, pThematicData, numlabels, (int)(double(m_nSz) / double(STEP*STEP)));
	for (int i = 0; i < m_nSz; i++)
		pInitSegData[i] = nlabels[i];
	delete[]m_avec;
	m_avec = NULL;
	delete[]m_bvec;
	m_bvec = NULL;
	delete[]m_lvec;
	m_lvec = NULL;
	if (nlabels)
		delete[] nlabels;
}

string SLIC::TestEnforceConnectivity(string sRGBFile, string sInputFile, string sOutputFile)
{
	GDALDataset *pDataset = (GDALDataset*)GDALOpen(sInputFile.c_str(), GA_ReadOnly);
	GDALDataset *pRGBDataset = (GDALDataset*)GDALOpen(sRGBFile.c_str(), GA_ReadOnly);
	
	m_nHeight = pDataset->GetRasterYSize();
	m_nWidth = pDataset->GetRasterXSize();
	m_nSz = m_nWidth * m_nHeight;
	const int STEP = (int)(sqrt(double(m_nSuperPixelSize)) + 0.5);
	GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset *pOutputDataset = pDriver->Create(sOutputFile.c_str(), m_nWidth, m_nHeight, 1, GDT_Int32, NULL);
	double *pGeoTransform = new double[6];
	pDataset->GetGeoTransform(pGeoTransform);
	pOutputDataset->SetProjection(pDataset->GetProjectionRef());
	pOutputDataset->SetGeoTransform(pGeoTransform);
	delete[]pGeoTransform;

	int *pInitSegData = new int[m_nSz];
	int *pRGBData = new int[m_nSz * 3];
	pDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, m_nWidth, m_nHeight, pInitSegData, m_nWidth, m_nHeight, GDT_Int32, 0, 0);
	pRGBDataset->RasterIO(GF_Read, 0, 0, m_nWidth, m_nHeight, pRGBData, m_nWidth, m_nHeight, GDT_Int32, 3, NULL, 0, 0, 0);
	GDALClose(pDataset);
	GDALClose(pRGBDataset);

	if (m_lvec != NULL)
		delete[] m_lvec;
	if (m_avec != NULL)
		delete[] m_avec;
	if (m_bvec != NULL)
		delete[]m_bvec;

	m_lvec = new double[m_nSz];
	m_avec = new double[m_nSz];
	m_bvec = new double[m_nSz];

	DoRGBtoLABConversion(pRGBData, m_lvec, m_avec, m_bvec);
	int nMin = 0, nMax = 3997998;
	int numlabels = nMax - nMin + 1;


	int *nlabels = new int[m_nSz];
	EnforceLabelConnectivity(pInitSegData, nlabels, NULL, numlabels, (int)(double(m_nSz) / double(STEP*STEP)));
	delete[]pInitSegData;
	delete[]pRGBData;

	pOutputDataset->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, m_nWidth, m_nHeight, nlabels, m_nWidth, m_nHeight, GDT_Int32, 0, 0);

	GDALClose(pOutputDataset);

	return "";
}

void SLIC::RefineSegResult(int *pSegData, double *pData, double dNoDataValue, int *nTotalObjNum)
{
	int nSz = m_nHeight * m_nWidth;
	int *pTempData = new int[nSz];
	int *xvec = new int[nSz];
	int *yvec = new int[nSz];
	for (int i = 0; i < nSz; i++)
	{
		pTempData[i] = -1;
		if (pData[i] == dNoDataValue)
			pSegData[i] = SegNoDataValue;
	}
	int dy[4] = { 0, -1, 0, 1 };
	int dx[4] = { -1, 0, 1, 0 };
	int x = 0, y = 0, c = 0, n = 0, count = 1;
	int oindex = 0, nindex = 0;
	for (int i = 0; i < m_nHeight; i++)
	{
		for (int j = 0; j < m_nWidth; j++)
		{
			oindex = i * m_nWidth + j;
			if (oindex == 1893790)
				x = 0;
			if (pTempData[oindex] == -1)
			{
				count = 1;
				if (pSegData[oindex] == SegNoDataValue)
					pTempData[oindex] = SegNoDataValue;
				else
					pTempData[oindex] = *nTotalObjNum;
				xvec[0] = j;
				yvec[0] = i;
				for (c = 0; c < count; c++)
				{
					for (n = 0; n < 4; n++)
					{
						x = xvec[c] + dx[n];
						y = yvec[c] + dy[n];

						if ((x >= 0 && x < m_nWidth) && (y >= 0 && y < m_nHeight))
						{
							nindex = y * m_nWidth + x;
							if (pTempData[nindex] == -1 && pSegData[oindex] == pSegData[nindex])
							{
								xvec[count] = x;
								yvec[count] = y;
								if (pSegData[nindex] == SegNoDataValue)
									pTempData[nindex] = SegNoDataValue;
								else
									pTempData[nindex] = *nTotalObjNum;
								
								count++;
							}
						}

					}
				}
				if (pSegData[oindex] != SegNoDataValue)
					(*nTotalObjNum)++;
			}
		}
	}
	for (int i = 0; i < nSz; i++)
		pSegData[i] = pTempData[i];
	delete[]pTempData;
	delete[]yvec;
	delete[]xvec;
}

string SLIC::PerformSuperpixelSLIC(vector<double>&	kseedsl, vector<double>& kseedsa, vector<double>& kseedsb,
	vector<double>&	kseedsx, vector<double>& kseedsy, int *& klabels, int *& pThematicData, int STEP, vector<double>&	edgemag, const double& M)
{
	int numk = (int)kseedsl.size();
	//----------------
	int offset = STEP;
	//if(STEP < 8) offset = STEP*1.5;//to prevent a crash due to a very small step size
	//----------------

	clustersize.clear();
	inv.clear();
	sigmal.clear();
	sigmaa.clear();
	sigmab.clear();
	sigmax.clear();
	sigmay.clear();
	distvec.clear();
	for (int i = 0; i < numk; i++)
	{
		clustersize.push_back(0);
		inv.push_back(0);
		sigmal.push_back(0);
		sigmaa.push_back(0);
		sigmab.push_back(0);
		sigmax.push_back(0);
		sigmay.push_back(0);
	}
	for (int i = 0; i < m_nSz; i++)
		distvec.push_back(DBL_MAX);

	double invwt = 1.0 / ((STEP / M)*(STEP / M));

	int x1, y1, x2, y2;
	double l, a, b;
	double dist;
	double distxy;
	GDALRasterBand *pBand = NULL;

	float fPercentPerIter = 85.0f / m_nNumOfIter;
	int nTempPercent = 0;
	int nSeedThematicData = 0, nTempThematicData = 0;
	for (int itr = 0; itr < m_nNumOfIter; itr++)
	{
		distvec.assign(m_nSz, DBL_MAX);
		for (int n = 0; n < numk; n++)
		{
			y1 = (int)max(0.0, kseedsy[n] - offset);
			y2 = (int)min((double)m_nHeight, kseedsy[n] + offset);
			x1 = (int)max(0.0, kseedsx[n] - offset);
			x2 = (int)min((double)m_nWidth, kseedsx[n] + offset);
			if (pThematicData != NULL)
				nSeedThematicData = pThematicData[(int)kseedsy[n] * m_nWidth + (int)kseedsx[n]];
			for (int y = y1; y < y2; y++)
			{
				for (int x = x1; x < x2; x++)
				{
					int i = y*m_nWidth + x;
					l = m_lvec[i];
					a = m_avec[i];
					b = m_bvec[i];

					dist = (l - kseedsl[n])*(l - kseedsl[n]) +
						(a - kseedsa[n])*(a - kseedsa[n]) +
						(b - kseedsb[n])*(b - kseedsb[n]);

					distxy = (x - kseedsx[n])*(x - kseedsx[n]) +
						(y - kseedsy[n])*(y - kseedsy[n]);
					

					//------------------------------------------------------------------------
					//dist += distxy*invwt;
					dist = sqrt(dist) + sqrt(distxy*invwt);//this is more exact
										 //------------------------------------------------------------------------
					if (pThematicData != NULL)
						nTempThematicData = pThematicData[i];
					if (dist < distvec[i])
					{
						if (pThematicData != NULL)
						{
							if (nTempThematicData == nSeedThematicData)
							{
								distvec[i] = dist;
								klabels[i] = n;
							}
						}
						else
						{
							distvec[i] = dist;
							klabels[i] = n;
						}
					}
				}
			}
		}

		int dx4[4] = { 1, 0, -1, 0 };
		int dy4[4] = { 0, 1, 0, -1 };
		stack<int> sX, sY;
		vector<int> vX, vY;
		nSeedThematicData = 0;
		nTempThematicData = 0;
		int nTempLabel = -1, nIndex = 0;
		double dMinDis = DBL_MAX, dTempDis = 0;
		for (int y = 0; y < m_nHeight; y++)
		{
			for (int x = 0; x < m_nWidth; x++)
			{
				int i = y * m_nWidth + x;
				if (klabels[i] == -1)
				{
					if (pThematicData != NULL)
					{
						nSeedThematicData = pThematicData[i];
					}
					dMinDis = DBL_MAX;
					nTempLabel = -1;
					for (int j = 0; j < 4; j++)
					{
						x1 = x + dx4[j];
						y1 = y + dy4[j];
						nIndex = y1 * m_nWidth + x1;
						if (pThematicData != NULL)
						{
							nTempThematicData = pThematicData[nIndex];
						}
						if (x1 >= 0 && x1 < m_nWidth && y1 >= 0 && y1 < m_nHeight)
						{
							/*if (klabels[y1 * m_nWidth + x1] != -1 && nTempThematicData == nSeedThematicData)
								klabels[i] = klabels[y1 * m_nWidth + x1];*/
							
							if (klabels[nIndex] != -1 && nTempThematicData == nSeedThematicData)
							{
								dTempDis = (m_lvec[i] - m_lvec[nIndex])*(m_lvec[i] - m_lvec[nIndex]) +
									(m_avec[i] - m_avec[nIndex])*(m_avec[i] - m_avec[nIndex]) +
									(m_bvec[i] - m_bvec[nIndex])*(m_bvec[i] - m_bvec[nIndex]);
								if (dTempDis < dMinDis)
								{
									dMinDis = dTempDis;
									nTempLabel = klabels[nIndex];
								}
							}
						}
					}
					klabels[i] = nTempLabel;
					if (klabels[i] == -1)
					{
						klabels[i] = numk;
						numk++;
						kseedsl.push_back(m_lvec[i]);
						kseedsa.push_back(m_avec[i]);
						kseedsb.push_back(m_bvec[i]);
						kseedsx.push_back(x);
						kseedsy.push_back(y);
						inv.push_back(0);
					}
				}
			}
		}
		sigmal.assign(numk, 0);
		sigmaa.assign(numk, 0);
		sigmab.assign(numk, 0);
		sigmax.assign(numk, 0);
		sigmay.assign(numk, 0);
		clustersize.assign(numk, 0);

		int ind(0);
		for (int r = 0; r < m_nHeight; r++)
		{
			for (int c = 0; c < m_nWidth; c++)
			{
				sigmal[klabels[ind]] += m_lvec[ind];
				sigmaa[klabels[ind]] += m_avec[ind];
				sigmab[klabels[ind]] += m_bvec[ind];
				sigmax[klabels[ind]] += c;
				sigmay[klabels[ind]] += r;
				
				clustersize[klabels[ind]] += 1.0;
				ind++;
			}
			
		}
		
		for (int k = 0; k < numk; k++)
		{
			if (clustersize[k] <= 0) 
				clustersize[k] = 1;
			inv[k] = 1.0 / clustersize[k];//computing inverse now to multiply, than divide later
		}
		
		for (int k = 0; k < numk; k++)
		{
			kseedsl[k] = sigmal[k] * inv[k];
			kseedsa[k] = sigmaa[k] * inv[k];
			kseedsb[k] = sigmab[k] * inv[k];
			kseedsx[k] = sigmax[k] * inv[k];
			kseedsy[k] = sigmay[k] * inv[k];
		}
	}
	return "";
}

void SLIC::GetLABXYSeeds_ForGivenStepSize(vector<double>&	kseedsl, vector<double>& kseedsa, vector<double>& kseedsb,
	vector<double>&	kseedsx, vector<double>& kseedsy, int STEP, bool perturbseeds, vector<double>& edgemag)
{
	const bool hexgrid = false;
	int numseeds(0);
	int n(0);

	int xstrips = (int)(0.5 + double(m_nWidth) / double(STEP));
	int ystrips = (int)(0.5 + double(m_nHeight) / double(STEP));

	int xerr = m_nWidth - STEP*xstrips; 
	if (xerr < 0) 
	{ 
		xstrips--; 
		xerr = m_nWidth - STEP*xstrips; 
	}
	int yerr = m_nHeight - STEP*ystrips; 
	if (yerr < 0) 
	{ 
		ystrips--; 
		yerr = m_nHeight - STEP*ystrips; 
	}

	double xerrperstrip = double(xerr) / double(xstrips);
	double yerrperstrip = double(yerr) / double(ystrips);

	int xoff = STEP / 2;
	int yoff = STEP / 2;
	//-------------------------
	numseeds = xstrips*ystrips;
	//-------------------------
	kseedsl.resize(numseeds);
	kseedsa.resize(numseeds);
	kseedsb.resize(numseeds);
	kseedsx.resize(numseeds);
	kseedsy.resize(numseeds);

	for (int y = 0; y < ystrips; y++)
	{
		int ye = (int)(y * yerrperstrip);
		for (int x = 0; x < xstrips; x++)
		{
			int xe = (int)(x*xerrperstrip);
			int seedx = (x*STEP + xoff + xe);
			if (hexgrid) 
			{ 
				seedx = x*STEP + (xoff << (y & 0x1)) + xe; 
				seedx = min(m_nWidth - 1, seedx); 
			}//for hex grid sampling
			int seedy = (y*STEP + yoff + ye);
			int i = seedy*m_nWidth + seedx;

			kseedsl[n] = m_lvec[i];
			kseedsa[n] = m_avec[i];
			kseedsb[n] = m_bvec[i];
			kseedsx[n] = seedx;
			kseedsy[n] = seedy;
			n++;
		}
	}


	if (perturbseeds)
	{
		PerturbSeeds(kseedsl, kseedsa, kseedsb, kseedsx, kseedsy, edgemag, STEP);
	}
}

void SLIC::PerturbSeeds(vector<double>& kseedsl, vector<double>& kseedsa, vector<double>& kseedsb,
	vector<double>& kseedsx, vector<double>&	kseedsy, vector<double>& edges, int STEP)
{
	if (STEP % 2 == 0)
	{
		STEP--;
	}
	int nSz = STEP * STEP - 1;
	int *dx8 = new int[nSz];
	int *dy8 = new int[nSz];
	int nBegin = (1 - STEP) / 2;
	int nEnd = (STEP - 1) / 2;
	int nCount = 0;
	for (int i = nBegin; i <= nEnd; i++)
	{
		for (int j = nBegin; j <= nEnd; j++)
		{
			if (i != 0 && j != 0)
			{
				dx8[nCount] = j;
				dy8[nCount] = i;
				nCount++;
			}
		}
	}
	//const int dx8[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	//const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };

	int numseeds = (int)kseedsl.size();

	for (int n = 0; n < numseeds; n++)
	{
		int ox = (int)kseedsx[n];//original x
		int oy = (int)kseedsy[n];//original y
		int oind = oy * m_nWidth + ox;

		int storeind = oind;
		for (int i = 0; i < nSz; i++)
		{
			int nx = ox + dx8[i];//new x
			int ny = oy + dy8[i];//new y

			if (nx >= 0 && nx < m_nWidth && ny >= 0 && ny < m_nHeight)
			{
				int nind = ny*m_nWidth + nx;
				if (edges[nind] < edges[storeind])
				{
					storeind = nind;
				}
			}
		}
		if (storeind != oind)
		{
			kseedsx[n] = storeind%m_nWidth;
			kseedsy[n] = storeind / m_nWidth;
			kseedsl[n] = m_lvec[storeind];
			kseedsa[n] = m_avec[storeind];
			kseedsb[n] = m_bvec[storeind];
		}
	}
}

void SLIC::DetectLabEdges(const double* lvec, const double* avec, const double* bvec,
	vector<double>& edges)
{
	edges.resize(m_nSz, 0);
	for (int j = 1; j < m_nHeight - 1; j++)
	{
		for (int k = 1; k < m_nWidth - 1; k++)
		{
			int i = j*m_nWidth + k;

			double dx = (lvec[i - 1] - lvec[i + 1])*(lvec[i - 1] - lvec[i + 1]) +
				(avec[i - 1] - avec[i + 1])*(avec[i - 1] - avec[i + 1]) +
				(bvec[i - 1] - bvec[i + 1])*(bvec[i - 1] - bvec[i + 1]);

			double dy = (lvec[i - m_nWidth] - lvec[i + m_nWidth])*(lvec[i - m_nWidth] - lvec[i + m_nWidth]) +
				(avec[i - m_nWidth] - avec[i + m_nWidth])*(avec[i - m_nWidth] - avec[i + m_nWidth]) +
				(bvec[i - m_nWidth] - bvec[i + m_nWidth])*(bvec[i - m_nWidth] - bvec[i + m_nWidth]);

			//edges[i] = fabs(dx) + fabs(dy);
			edges[i] = dx * dx + dy * dy;
		}
	}
}

void SLIC::RGB2XYZ(const int&	sR, const int& sG, const int& sB, double& X, double& Y, double&	Z)
{
	double R = sR / 255.0;
	double G = sG / 255.0;
	double B = sB / 255.0;

	double r, g, b;

	if (R <= 0.04045)
		r = R / 12.92;
	else
		r = pow((R + 0.055) / 1.055, 2.4);
	if (G <= 0.04045)	
		g = G / 12.92;
	else
		g = pow((G + 0.055) / 1.055, 2.4);
	if (B <= 0.04045)	
		b = B / 12.92;
	else				
		b = pow((B + 0.055) / 1.055, 2.4);

	X = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
	Y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
	Z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;
}

void SLIC::RGB2LAB(const int&	sR, const int& sG, const int& sB, double& lval, double& aval, double& bval)
{
	double X, Y, Z;
	RGB2XYZ(sR, sG, sB, X, Y, Z);
	double epsilon = 0.008856;	//actual CIE standard
	double kappa = 903.3;		//actual CIE standard

	double Xr = 0.950456;	//reference white
	double Yr = 1.0;		//reference white
	double Zr = 1.088754;	//reference white

	double xr = X / Xr;
	double yr = Y / Yr;
	double zr = Z / Zr;

	double fx, fy, fz;
	if (xr > epsilon)	
		fx = pow(xr, 1.0 / 3.0);
	else
		fx = (kappa*xr + 16.0) / 116.0;
	if (yr > epsilon)
		fy = pow(yr, 1.0 / 3.0);
	else
		fy = (kappa*yr + 16.0) / 116.0;
	if (zr > epsilon)
		fz = pow(zr, 1.0 / 3.0);
	else
		fz = (kappa*zr + 16.0) / 116.0;

	lval = 116.0 * fy - 16.0;
	aval = 500.0 * (fx - fy);
	bval = 200.0 * (fy - fz);
}

void SLIC::DoRGBtoLABConversion(int *pImageData, double *lvec, double *avec, double *bvec)
{
	for (int j = 0; j < m_nSz; j++)
	{
		//RGB2LAB(pImageData[j], pImageData[m_nSz + j], pImageData[2 * m_nSz + j], lvec[j], avec[j], bvec[j]);
		lvec[j] = pImageData[j];
		avec[j] = pImageData[m_nSz + j];
		bvec[j] = pImageData[2 * m_nSz + j];
	}
}

string SLIC::EnforceLabelConnectivity(const int* labels, int*& nlabels, int* pThematicData, int& numlabels, const int& K)
{
	const int dx4[4] = { -1,  0,  1,  0 };
	const int dy4[4] = { 0, -1,  0,  1 };

	const int SUPSZ = m_nSz / K;
	for (int i = 0; i < m_nSz; i++) nlabels[i] = -1;
	int label(0);
	int* xvec = new int[m_nSz];
	int* yvec = new int[m_nSz];
	int oindex(0);
	int adjlabel(0);
	int nSeedThematicData = 0, nTempThematicData = 0;
	double dMinDis = DBL_MAX, dTempDis = 0;
	double dMeanL = 0, dMeanA = 0, dMeanB = 0;
	bool bIsMinDis = false;
	int adjx = 0, adjy = 0, nTempIndex = 0;
	int x = 0, y = 0, c = 0, n = 0;
	int j = 0, k = 0, count = 1, nindex = 0;
	int nMinSuperPixelSize = SUPSZ >> 2;
	if (pThematicData != NULL)
		nMinSuperPixelSize = 1;
	for (j = 0; j < m_nHeight; j++)
	{
		for (k = 0; k < m_nWidth; k++)
		{
			if (0 > nlabels[oindex])
			{
				nlabels[oindex] = label;
				xvec[0] = k;
				yvec[0] = j;
				if (pThematicData != NULL)
					nSeedThematicData = pThematicData[oindex];

				count = 1;
				dMeanL = m_lvec[j * m_nWidth + k];
				dMeanA = m_avec[j * m_nWidth + k];
				dMeanB = m_bvec[j * m_nWidth + k];
				bIsMinDis = false;
				//统计当前标签的像素位置及其LAB色彩均值
				for (c = 0; c < count; c++)
				{
					for (n = 0; n < 4; n++)
					{
						x = xvec[c] + dx4[n];
						y = yvec[c] + dy4[n];

						if ((x >= 0 && x < m_nWidth) && (y >= 0 && y < m_nHeight))
						{
							nindex = y*m_nWidth + x;
							if (pThematicData != NULL)
								nTempThematicData = pThematicData[nindex];
							if (0 > nlabels[nindex] && labels[oindex] == labels[nindex] && nSeedThematicData == nTempThematicData)
							{
								xvec[count] = x;
								yvec[count] = y;
								nlabels[nindex] = label;
								dMeanL += m_lvec[y * m_nWidth + x];
								dMeanA += m_lvec[y * m_nWidth + x];
								dMeanB += m_lvec[y * m_nWidth + x];
								count++;
							}
						}

					}
				}
				dMeanL /= count;
				dMeanA /= count;
				dMeanB /= count;
				dMinDis = DBL_MAX;
				
				//-------------------------------------------------------
				// If segment size is less then a limit, assign an
				// adjacent label found before, and decrement label count.
				//-------------------------------------------------------
				//如果标签中的像素数小于阈值，与周围标签合并
				if (count <= nMinSuperPixelSize)
				{
					//找到离当前标签相邻标签中LAB空间距离最近的一个
					for (c = 0; c < count; c++)
					{
						for (n = 0; n < 4; n++)
						{
							x = xvec[c] + dx4[n];
							y = yvec[c] + dy4[n];
							if ((x >= 0 && x < m_nWidth) && (y >= 0 && y < m_nHeight))
							{
								nindex = y*m_nWidth + x;
								if (pThematicData != NULL)
									nTempThematicData = pThematicData[nindex];
								if (nlabels[nindex] != label && nSeedThematicData == nTempThematicData)
								{
									dTempDis = (dMeanL - m_lvec[nindex])*(dMeanL - m_lvec[nindex]) +
										(dMeanA - m_avec[nindex])*(dMeanA - m_avec[nindex]) +
										(dMeanB - m_bvec[nindex])*(dMeanB - m_bvec[nindex]);
									if (dTempDis < dMinDis)
									{
										dMinDis = dTempDis;
										if (nlabels[nindex] >= 0)
										{
											adjlabel = nlabels[nindex];
											bIsMinDis = true;
										}
										else
										{
											adjlabel = label;
											bIsMinDis = false;
											adjx = x;
											adjy = y;
										}
									}
								}
							}
						}
					}

					//若最小距离为Double最大值，说明该聚类只有一个像素，且周围的语义ID与改像素不同
					if (dMinDis == DBL_MAX)
					{
						for (c = 0; c < count; c++)
						{
							for (n = 0; n < 4; n++)
							{
								x = xvec[c] + dx4[n];
								y = yvec[c] + dy4[n];
								if ((x >= 0 && x < m_nWidth) && (y >= 0 && y < m_nHeight))
								{
									nindex = y*m_nWidth + x;
									if (nlabels[nindex] != label)
									{
										dTempDis = (dMeanL - m_lvec[nindex])*(dMeanL - m_lvec[nindex]) +
											(dMeanA - m_avec[nindex])*(dMeanA - m_avec[nindex]) +
											(dMeanB - m_bvec[nindex])*(dMeanB - m_bvec[nindex]);
										if (dTempDis < dMinDis)
										{
											dMinDis = dTempDis;
											if (nlabels[nindex] >= 0)
											{
												adjlabel = nlabels[nindex];
												bIsMinDis = true;
											}
											else
											{
												adjlabel = label;
												bIsMinDis = false;
												adjx = x;
												adjy = y;
											}
										}
									}
								}
							}
						}
					}

					//若相邻标签已被标记，将当前标签合并到已标记标签
					if (bIsMinDis)
					{
						for (c = 0; c < count; c++)
						{
							nindex = yvec[c] * m_nWidth + xvec[c];
							nlabels[nindex] = adjlabel;
						}
						label--;
					}
					//否则将相邻标签合并到当前标签
					else
					{
						nTempIndex = adjy * m_nWidth + adjx;
						yvec[0] = adjy;
						xvec[0] = adjx;
						nlabels[nTempIndex] = label;
						count = 1;
						for (c = 0; c < count; c++)
						{
							for (n = 0; n < 4; n++)
							{
								x = xvec[c] + dx4[n];
								y = yvec[c] + dy4[n];

								if ((x >= 0 && x < m_nWidth) && (y >= 0 && y < m_nHeight))
								{
									nindex = y * m_nWidth + x;
									if (0 > nlabels[nindex] && labels[nTempIndex] == labels[nindex])
									{
										nlabels[nindex] = label;
										yvec[count] = y;
										xvec[count] = x;
										count++;
									}
								}

							}
						}
					}
				}
				label++;
			}
			oindex++;
		}
	}
	numlabels = label;

	if (xvec) delete[] xvec;
	if (yvec) delete[] yvec;
	return "";
}


string SLIC::CreateRGBImage(string sInputFile, string sOutputFile)
{
	remove(sOutputFile.c_str());
	GDALDataset *pDataset = (GDALDataset*)GDALOpen(sInputFile.c_str(), GA_ReadOnly);
	if (pDataset == NULL)
		return "打开影像" + sInputFile + "失败！";

	int nRasterXSize = pDataset->GetRasterXSize();
	int nRasterYSize = pDataset->GetRasterYSize();
	int nRasterCount = pDataset->GetRasterCount();

	GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (pDriver == NULL)
		return "创建栅格驱动失败！";
	GDALDataset *pRGBDataset = pDriver->Create(sOutputFile.c_str(), nRasterXSize, nRasterYSize, 3, GDT_Byte, NULL);
	if (pRGBDataset == NULL)
		return "创建栅格数据失败！";
	double *adfGeoTransform = new double[6];
	pDataset->GetGeoTransform(adfGeoTransform);
	pRGBDataset->SetGeoTransform(adfGeoTransform);
	delete[]adfGeoTransform;
	adfGeoTransform = NULL;
	pRGBDataset->SetProjection(pDataset->GetProjectionRef());
	GDALRasterBand **pBand = new GDALRasterBand*[3];
	if (pDataset->GetRasterCount() < 3)
	{
		pBand[0] = pDataset->GetRasterBand(1);
		pBand[1] = pDataset->GetRasterBand(1);
		pBand[2] = pDataset->GetRasterBand(1);
	}
	else
	{
		pBand[0] = pDataset->GetRasterBand(1);
		pBand[1] = pDataset->GetRasterBand(2);
		pBand[2] = pDataset->GetRasterBand(3);
	}
	double RGBMax = 0, RGBMin = 0;
	GUIntBig *pHistogram = new GUIntBig[HISTOGRAM_NUM];
	for (int i = 0; i < HISTOGRAM_NUM; i++)
		pHistogram[i] = 0;
	int nSz = nRasterXSize * nRasterYSize;
	double *pData = new double[nRasterXSize];
	unsigned char *pOutputData = new unsigned char[nRasterXSize];
	for (int i = 0; i < 3; i++)
	{
		if (pBand[i]->GetStatistics(false, false, &RGBMin, &RGBMax, NULL, NULL) != CE_None)
			pBand[i]->ComputeStatistics(false, &RGBMin, &RGBMax, NULL, NULL, NULL, NULL);
		pBand[i]->GetHistogram(RGBMin - 0.5, RGBMax + 0.5, HISTOGRAM_NUM, pHistogram, false, false, NULL, NULL);

		int nMinIndex = 0;
		int nMaxIndex = 0;
		int nTemp = 0;
		for (int i = 0; i < HISTOGRAM_NUM; i++)
		{
			nTemp += pHistogram[i];
			if (nTemp * 1.0 / nSz > 0.02)
			{
				if (i != 0)
				{
					if (0.02 - (nTemp - pHistogram[i]) * 1.0 / nSz > nTemp * 1.0 / nSz - 0.02)
						nMinIndex = i;
					else
						nMinIndex = i - 1;
				}
				else
					nMinIndex = i;
				break;
			}
		}
		nTemp = 0;
		for (int i = HISTOGRAM_NUM - 1; i >= 0; i--)
		{
			nTemp += pHistogram[i];
			if (nTemp * 1.0 / nSz > 0.02)
			{
				if (i != HISTOGRAM_NUM - 1)
				{
					if (0.02 - (nTemp - pHistogram[i]) * 1.0 / nSz > nTemp * 1.0 / nSz - 0.02)
						nMaxIndex = i;
					else
						nMaxIndex = i + 1;
				}
				else
					nMaxIndex = i;
				break;
			}
		}
		double dMin = RGBMin + (RGBMax - RGBMin) * (nMinIndex + 1) / HISTOGRAM_NUM;
		double dMax = RGBMin + (RGBMax - RGBMin) * nMaxIndex / HISTOGRAM_NUM;
		double dTemp = 255 / (dMax - dMin);
		GDALRasterBand *pRGBBand = pRGBDataset->GetRasterBand(i + 1);
		for (int y = 0; y < nRasterYSize; y++)
		{
			pBand[i]->RasterIO(GF_Read, 0, y, nRasterXSize, 1, pData, nRasterXSize, 1, GDT_Float64, 0, 0);
			for (int x = 0; x < nRasterXSize; x++)
			{
				if (pData[x] <= dMin)
					pOutputData[x] = (unsigned char)0;
				else if (pData[x] >= dMax)
					pOutputData[x] = (unsigned char)255;
				else
					pOutputData[x] = (unsigned char)(dTemp * (pData[x] - dMin));
			}
			pRGBBand->RasterIO(GF_Write, 0, y, nRasterXSize, 1, pOutputData, nRasterXSize, 1, GDT_Byte, 0, 0);
		}
	}
	delete[]pData;
	delete[]pOutputData;
	delete[]pBand;
	delete[]pHistogram;
	GDALClose(pDataset);
	GDALClose(pRGBDataset);
	return "";
}
