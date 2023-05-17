#pragma once

#define HISTOGRAM_NUM 256
//设置一个影像块的大小，这里默认为10000*10000，高或宽超过10000的影像将会被分块处理
#define MaxImageSize 10000
//#define MaxImageSize 500
//设置分割结果中的无效像元值
#define SegNoDataValue -999
#define LabelNoDataValue 255

#define EulerNumber 2.718281828459
