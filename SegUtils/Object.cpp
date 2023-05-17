#include "Object.h"

MinRectangle::MinRectangle(int nXMin, int nXMax, int nYMin, int nYMax)
{
	m_nXMin = nXMin;
	m_nXMax = nXMax;
	m_nYMin = nYMin;
	m_nYMax = nYMax;
}

MinRectangle& MinRectangle::operator = (MinRectangle &MBR)
{
	m_nXMin = MBR.m_nXMin;
	m_nXMax = MBR.m_nXMax;
	m_nYMax = MBR.m_nYMax;
	m_nYMin = MBR.m_nYMin;
	return *this;
}

bool MinRectangle::operator == (MinRectangle &MBR)
{
	if (m_nXMin == MBR.m_nXMin && m_nXMax == MBR.m_nXMax &&
		m_nYMin == MBR.m_nYMin && m_nYMax == MBR.m_nYMax)
		return true;
	else
		return false;
}

PixelPosition::PixelPosition(int r, int c)
{
	m_nRow = r;
	m_nCol = c;
}

PixelPosition& PixelPosition::operator = (PixelPosition &p)
{
	m_nRow = p.m_nRow;
	m_nCol = p.m_nCol;
	return *this;
}

bool PixelPosition::operator == (const PixelPosition &p)
{
	if (m_nRow == p.m_nRow && m_nCol == p.m_nCol)
		return true;
	else
		return false;
}

NirObject::NirObject(int nID, int ComBrdNum)
{
	m_nID = nID;
	m_nComBrdNum = ComBrdNum;
}

bool NirObject::operator == (const NirObject &n)
{
	if (m_nID == n.m_nID)
		return true;
	else
		return false;
}

Object::Object(int nID):m_mMBR()
{
	m_nID = nID;
	m_pMean = NULL;
	m_pStd = NULL;
	m_pMax = NULL;
	m_pMin = NULL;
	m_nPerimeter = 0;
	m_bIsSeged = false;
	m_nBandCount = 0;
	m_fXCenter = m_fYCenter = 0.0f;
	m_vPixels.clear();
	m_vNirObjs.clear();
	m_nClassLabel = 0;
	m_nThematicLabel = 0;
}

Object::Object(Object &Obj)
{
	m_nID = Obj.m_nID;
	m_mMBR = Obj.m_mMBR;
	m_bIsSeged = Obj.m_bIsSeged;
	m_nPerimeter = Obj.m_nPerimeter;
	m_nBandCount = Obj.m_nBandCount;
	m_fXCenter = Obj.m_fXCenter;
	m_fYCenter = Obj.m_fYCenter;
	for (int i = 0; i < Obj.GetPixelNum(); i++)
	{
		InsertPixel(Obj.GetPixelByIndex(i));
	}
	for (int i = 0; i < (int)Obj.m_vNirObjs.size(); i++)
	{
		InsertNirObj(Obj.m_vNirObjs[i]);
	}
	m_pMean = NULL;
	m_pStd = NULL;
	m_pMax = NULL;
	m_pMin = NULL;
	SetMean(Obj.m_pMean, Obj.m_nBandCount);
	SetStd(Obj.m_pStd, Obj.m_nBandCount);
	SetMax(Obj.m_pMax, Obj.m_nBandCount);
	SetMin(Obj.m_pMin, Obj.m_nBandCount);
}

Object::~Object()
{
	if (m_pMean)
	{
		delete[]m_pMean;
	}
	if (m_pStd)
	{
		delete[]m_pStd;
	}
	if (m_pMax)
	{
		delete[]m_pMax;
	}
	if (m_pMin)
	{
		delete[]m_pMin;
	}
	m_vPixels.clear();
	m_vPixels.reserve(1);
	m_vNirObjs.clear();
	m_vNirObjs.reserve(1);
}

void Object::InsertPixel(PixelPosition &p)
{
	m_vPixels.push_back(p);
}

void Object::InsertPixel(int y, int x)
{
	InsertPixel(PixelPosition(y, x));
}

void Object::InsertNirObj(NirObject &nObj)
{
	m_vNirObjs.push_back(nObj);
}

void Object::InsertNirObj(int nID, int nComBrdNum)
{
	m_vNirObjs.push_back(NirObject(nID, nComBrdNum));
}

bool Object::IsInNirObj(NirObject &nObj)
{
	vector<NirObject>::iterator it = find(m_vNirObjs.begin(), m_vNirObjs.end(), nObj);
	if (it == m_vNirObjs.end())
		return false;
	else
		return true;
}

bool Object::IsInNirObj(int nID)
{
	return IsInNirObj(NirObject(nID));
}

void Object::DeleteNirObj(NirObject &nObj)
{
	vector<NirObject>::iterator it = find(m_vNirObjs.begin(), m_vNirObjs.end(), nObj);
	if (it != m_vNirObjs.end())
		m_vNirObjs.erase(it);
}

void Object::DeleteNirObj(int nID)
{
	DeleteNirObj(NirObject(nID));
}

int Object::GetPixelNum()
{
	return (int)m_vPixels.size();
}

vector<NirObject>& Object::GetNirObjs()
{
	return m_vNirObjs;
}

float Object::GetMinMBRLength()
{
	float f1 = (float)(m_mMBR.m_nXMax - m_mMBR.m_nXMin + 1);
	float f2 = (float)(m_mMBR.m_nYMax - m_mMBR.m_nYMin + 1);
	if (f1 > f2)
		return f2;
	else
		return f1;
}

void Object::SetMBR(MinRectangle &mMBR)
{
	m_mMBR = mMBR;
}

void Object::SetMBR(int nXMin, int nXMax, int nYMin, int nYMax)
{
	m_mMBR.m_nXMin = nXMin;
	m_mMBR.m_nXMax = nXMax;
	m_mMBR.m_nYMin = nYMin;
	m_mMBR.m_nYMax = nYMax;
}

void Object::SetMean(float *pMean, int nBandCount)
{
	if (m_pMean)
		delete[]m_pMean;
	m_pMean = new float[nBandCount];
	for (int i = 0; i < nBandCount; i++)
		m_pMean[i] = pMean[i];
}

void Object::SetStd(float *pStd, int nBandCount)
{
	if (m_pStd)
		delete[]m_pStd;
	m_pStd = new float[nBandCount];
	for (int i = 0; i < nBandCount; i++)
		m_pStd[i] = pStd[i];
}

void Object::SetMax(float *pMax, int nBandCount)
{
	if (m_pMax)
		delete[]m_pMax;
	m_pMax = new float[nBandCount];
	for (int i = 0; i < nBandCount; i++)
		m_pMax[i] = pMax[i];
}

void Object::SetMin(float *pMin, int nBandCount)
{
	if (m_pMin)
		delete[]m_pMin;
	m_pMin = new float[nBandCount];
	for (int i = 0; i < nBandCount; i++)
	{
		m_pMin[i] = pMin[i];
	}
}

void Object::ComputeAtt(double *pData, int *pSegData, int nRasterXSize, int nRasterYSize, int nRasterCount)
{
	ComputeMBR();
	ComputeMeanStd(pData, nRasterXSize, nRasterYSize, nRasterCount);
	ComputePerimeter(pSegData, nRasterXSize, nRasterYSize);
}

void Object::ComputeAtt(GDALDataset *pImageDataset, GDALDataset* pFIDDataset, int nRasterCount)
{
	ComputeMBR();
	ComputeMeanStd(pImageDataset, nRasterCount);
	ComputePerimeter(pFIDDataset);
}

int Object::GetComBrdByID(int nID)
{
	vector<NirObject>::iterator it = find(m_vNirObjs.begin(), m_vNirObjs.end(), NirObject(nID));
	if (it != m_vNirObjs.end())
		return it->m_nComBrdNum;
	else
		return 0;
}

void Object::SetComBrdByID(int nID, int nNum)
{
	vector<NirObject>::iterator it = find(m_vNirObjs.begin(), m_vNirObjs.end(), NirObject(nID));
	if (it != m_vNirObjs.end())
		it->m_nComBrdNum = nNum;
}

void Object::AddOneComBrdNumByID(int nID)
{
	vector<NirObject>::iterator it = find(m_vNirObjs.begin(), m_vNirObjs.end(), NirObject(nID));
	if (it != m_vNirObjs.end())
		it->m_nComBrdNum++;
}

bool Object::IsPixelIn(PixelPosition &p)
{
	vector<PixelPosition>::iterator it;
	it = find(m_vPixels.begin(), m_vPixels.end(), p);
	if (it == m_vPixels.end())
		return false;
	else
		return true;
}

bool Object::IsPixelIn(int y, int x)
{
	return IsPixelIn(PixelPosition(y, x));
}

void Object::ComputeMeanStd(double *pData, int nRasterXSize, int nRasterYSize, int nRasterCount)
{
	if (m_pMean)
		delete[]m_pMean;
	if (m_pStd)
		delete[]m_pStd;
	if (m_pMax)
		delete[]m_pMax;
	if (m_pMin)
		delete[]m_pMin;

	int nSz = nRasterXSize * nRasterYSize;

	m_pMean = new float[nRasterCount];
	m_pStd = new float[nRasterCount];
	m_pMax = new float[nRasterCount];
	m_pMin = new float[nRasterCount];

	int nPixelNum = GetPixelNum();
	float mean = 0, std = 0, max = 0, min = FLT_MAX;
	int x = 0, y = 0;
	float fData = 0.0f;
	for (int n = 0; n < nRasterCount; n++)
	{
		mean = std = 0.0f;
		max = 0.0f;
		min = FLT_MAX;
		for (int i = 0; i < nPixelNum; i++)
		{
			x = m_vPixels[i].m_nCol;
			y = m_vPixels[i].m_nRow;
			fData = pData[n * nSz + y * nRasterXSize + x];
			mean += fData;
			std += (float)pow(fData, 2);
			if (fData > max)
				max = fData;
			if (fData < min)
				min = fData;
		}
		m_pMean[n] = mean / nPixelNum;
		float fTemp = std / nPixelNum - (float)pow(m_pMean[n], 2);
		if (fTemp < 0)
			fTemp = 0;
		m_pStd[n] = (float)sqrt(fTemp);
		m_pMax[n] = max;
		m_pMin[n] = min;
	}
}

void Object::ComputeMeanStd(GDALDataset *pImageDataset, int nRasterCount)
{
	if (m_pMean)
		delete[]m_pMean;
	if (m_pStd)
		delete[]m_pStd;
	if (m_pMax)
		delete[]m_pMax;
	if (m_pMin)
		delete[]m_pMin;

	m_pMean = new float[nRasterCount];
	m_pStd = new float[nRasterCount];
	m_pMax = new float[nRasterCount];
	m_pMin = new float[nRasterCount];

	int nXBegin = m_mMBR.m_nXMin, nYBegin = m_mMBR.m_nYMin;
	int nXSize = m_mMBR.m_nXMax - nXBegin + 1;
	int nYSize = m_mMBR.m_nYMax - nYBegin + 1;
	double *pData = new double[nXSize * nYSize * nRasterCount];
	pImageDataset->RasterIO(GF_Read, nXBegin, nYBegin, nXSize, nYSize, pData, nXSize, nYSize, GDT_Float64, nRasterCount, NULL, 0, 0, 0);
	int nSz = nXSize * nYSize;

	int nPixelNum = GetPixelNum();
	float mean = 0, std = 0, max = 0, min = FLT_MAX;
	int x = 0, y = 0;
	float fData = 0.0f;
	for (int n = 0; n < nRasterCount; n++)
	{
		mean = std = 0.0f;
		max = 0.0f;
		min = FLT_MAX;
		for (int i = 0; i < nPixelNum; i++)
		{
			x = m_vPixels[i].m_nCol - nXBegin;
			y = m_vPixels[i].m_nRow - nYBegin;
			fData = pData[n * nSz + y * nXSize + x];
			mean += fData;
			std += (float)pow(fData, 2);
			if (fData > max)
				max = fData;
			if (fData < min)
				min = fData;
		}
		m_pMean[n] = mean / nPixelNum;
		float fTemp = std / nPixelNum - (float)pow(m_pMean[n], 2);
		if (fTemp < 0)
			fTemp = 0;
		m_pStd[n] = (float)sqrt(fTemp);
		m_pMax[n] = max;
		m_pMin[n] = min;
	}
	delete[]pData;
}

void Object::ComputeMBR()
{
	int nPixelNum = GetPixelNum();
	int nMinX = INT_MAX, nMaxX = -1;
	int nMinY = INT_MAX, nMaxY = -1;
	for (int i = 0; i < nPixelNum; i++)
	{
		if (m_vPixels[i].m_nRow < nMinY)
			nMinY = m_vPixels[i].m_nRow;
		if (m_vPixels[i].m_nRow > nMaxY)
			nMaxY = m_vPixels[i].m_nRow;
		if (m_vPixels[i].m_nCol < nMinX)
			nMinX = m_vPixels[i].m_nCol;
		if (m_vPixels[i].m_nCol > nMaxX)
			nMaxX = m_vPixels[i].m_nCol;
	}
	SetMBR(nMinX, nMaxX, nMinY, nMaxY);
}

void Object::ComputePerimeter(int *pSegData, int nRasterXSize, int nRasterYSize)
{
	int dx4[4] = { -1, 0, 1, 0 };
	int dy4[4] = { 0, -1, 0, 1 };
	m_nPerimeter = 0;
	int nPixelNum = GetPixelNum();
	int x = 0, y = 0;
	for (int i = 0; i < nPixelNum; i++)
	{
		for (int k = 0; k < 4; k++)
		{
			x = m_vPixels[i].m_nCol + dx4[k];
			y = m_vPixels[i].m_nRow + dy4[k];
			if (x >= 0 && x < nRasterXSize && y >= 0 && y < nRasterYSize)
			{
				if (pSegData[y * nRasterXSize + x] != m_nID)
					m_nPerimeter++;
			}
			else
				m_nPerimeter++;
		}
	}
}

void Object::ComputePerimeter(GDALDataset *pFIDDataset)
{
	int dx4[4] = { -1, 0, 1, 0 };
	int dy4[4] = { 0, -1, 0, 1 };
	m_nPerimeter = 0;
	int nPixelNum = GetPixelNum();

	int nXBegin = m_mMBR.m_nXMin, nYBegin = m_mMBR.m_nYMin;
	int nXSize = m_mMBR.m_nXMax - nXBegin + 1;
	int nYSize = m_mMBR.m_nYMax - nYBegin + 1;
	int *pSegData = new int[nXSize * nYSize];
	pFIDDataset->RasterIO(GF_Read, nXBegin, nYBegin, nXSize, nYSize, pSegData, nXSize, nYSize, GDT_Int32, 1, NULL, 0, 0, 0);

	int x = 0, y = 0;
	for (int i = 0; i < nPixelNum; i++)
	{
		for (int k = 0; k < 4; k++)
		{
			x = m_vPixels[i].m_nCol - nXBegin + dx4[k];
			y = m_vPixels[i].m_nRow - nYBegin + dy4[k];
			if (x >= 0 && x < nXSize && y >= 0 && y < nYSize)
			{
				if (pSegData[y * nXSize + x] != m_nID)
					m_nPerimeter++;
			}
			else
				m_nPerimeter++;
		}
	}
	delete[]pSegData;
}

PixelPosition Object::GetPixelByIndex(int nIndex)
{
	if (nIndex < 0 || nIndex >= GetPixelNum())
		return PixelPosition();
	else
		return m_vPixels[nIndex];
}

void Object::ComputeCenter()
{
	m_fXCenter = 0.0f;
	m_fYCenter = 0.0f;
	int nPixelNum = GetPixelNum();
	for (int i = 0; i < nPixelNum; i++)
	{
		PixelPosition p = GetPixelByIndex(i);
		m_fXCenter += p.m_nCol;
		m_fYCenter += p.m_nRow;
	}
	m_fXCenter /= nPixelNum;
	m_fYCenter /= nPixelNum;
}
