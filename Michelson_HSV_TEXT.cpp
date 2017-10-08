//--------------------------------------【程序说明】-------------------------------------------
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
using namespace std;
using namespace cv;

//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")


//-----------------------------------【宏定义部分】--------------------------------------------
//		描述：定义一些辅助宏
//------------------------------------------------------------------------------------------------
#define WINDOW_NAME1 "【程序原图】"
#define WINDOW_NAME2 "【二值图】"
#define video
#define HSV

//-----------------------------------【全局变量声明部分】--------------------------------------
//		描述：全局变量的声明
//-----------------------------------------------------------------------------------------------
void mouseHandler(int event, int x, int y, int flags, void *param);

Mat g_GrayImage;
Mat g_BinaryImage;
Mat g_SrcImage;
Mat imgHSV;

#ifdef HSV
int iLowH = 0;
int iHighH = 180;
int iLowS = 0;
int iHighS = 255;
int iLowV = 0;
int iHighV = 255;
#endif

#ifndef HSV
int g_nThresh = 150;
int g_nMaxThresh = 255;
#endif

vector<vector<Point> > g_vContours;
vector<Vec4i> g_vHierarchy;
RNG g_rng(12345);

Point selection;
bool Point_is_Setted = false;//标记是否设置标记点  true:已设立标记点  false:标记点未设置 
bool Test_Pixel = false;//检测像素
int num = 0;//记录条纹变化,选定点时就会+1,因此置-1
//2016.11.9使用实验室摄像头不会这样


void mouseHandler(int event, int x, int y, int flags, void *param)
{
	switch (event)
	{
	case CV_EVENT_LBUTTONDOWN:
		if (!Point_is_Setted)
			//at的用法，<>用来表示所指向点的格式，二值图/灰度图为8位单通道
		if (g_BinaryImage.at<uchar>(y, x) != 0)
		{
			printf("Selected Location:(%d,%d)\n", x, y);
			selection.x = x;
			selection.y = y;
			Point_is_Setted = true;
		}
		//设置为必须检测到白色部分才可以设置成功
		else
		{
			printf("Please choose another point.\n");
		}
		break;
		//鼠标右击更改计量点
	case CV_EVENT_RBUTTONDOWN:
		Point_is_Setted = false;
		num = -1;//更换定点的应该同时重置计数值
		printf("Please choose a point for show.\n");
		break;
	default:
		break;
	}
}

int main(int argc, char** argv)
{
	VideoCapture cam(1);//打开摄像头:0一般为内置摄像头，1为外置摄像头

	/*清理图片一块位置用来放置图片*/
	Point screen_points[1][4];
	screen_points[0][0] = Point(0, 0);
	screen_points[0][1] = Point(320, 0);
	screen_points[0][2] = Point(320, 120);
	screen_points[0][3] = Point(0, 120);
	const Point* ppt[1] = { screen_points[0] };
	int npt[] = { 4 };

	namedWindow(WINDOW_NAME1, CV_WINDOW_AUTOSIZE);
	namedWindow(WINDOW_NAME2, CV_WINDOW_AUTOSIZE);
	//绑定鼠标响应函数与窗口
	cvSetMouseCallback(WINDOW_NAME2, mouseHandler, NULL);
#ifdef HSV
	namedWindow("Control", CV_WINDOW_AUTOSIZE);
	cvCreateTrackbar("H下界", "Control", &iLowH, 180);
	cvCreateTrackbar("H上界", "Control", &iHighH, 180);
	cvCreateTrackbar("S下界", "Control", &iLowS, 255);
	cvCreateTrackbar("S上界", "Control", &iHighS, 255);
	cvCreateTrackbar("V下界", "Control", &iLowV, 255);
	cvCreateTrackbar("V上界", "Control", &iHighV, 255);
#endif

#ifndef HSV
	//创建滚动条并进行初始化
	createTrackbar(" 阈值", WINDOW_NAME2, &g_nThresh, g_nMaxThresh);
#endif
	while (1)
	{
		//传入图像
		cam >> g_SrcImage;
		//resize(g_SrcImage, g_SrcImage, Size(g_SrcImage.cols*1.5, g_SrcImage.rows*1.5));

#ifdef HSV
		blur(g_SrcImage, g_SrcImage, Size(3, 3));
		vector<Mat> hsvSplit;
		cvtColor(g_SrcImage, imgHSV, COLOR_BGR2HSV);
		split(imgHSV, hsvSplit);
		equalizeHist(hsvSplit[2], hsvSplit[2]);
		merge(hsvSplit, imgHSV);
		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), g_BinaryImage);
#endif


		

#ifndef HSV
		// 转为灰度图并降噪
		cvtColor(g_SrcImage, g_GrayImage, CV_BGR2GRAY);
		blur(g_GrayImage, g_GrayImage, Size(3, 3));
		// 转为二值图
		threshold(g_GrayImage, g_BinaryImage, g_nThresh, g_nMaxThresh, THRESH_BINARY);
#endif
		//检测目标像素变化
		if (Point_is_Setted)
			//若未检测到255—>0，则每次检测是否出现255—>0,是则改变标志值，等待0—>255到来
		if (!Test_Pixel)
		{
			if (g_BinaryImage.at<uchar>(selection.y, selection.x) == 0)
				Test_Pixel = true;
			waitKey(5);//延时5ms消抖,视情况可去掉
		}
		//如已经检测到一次255—>0，则再检测到0—>255,说明计入一个条纹，重置标志值
		else
		{
			if (g_BinaryImage.at<uchar>(selection.y, selection.x) != 0)
			{
				num++;
				Test_Pixel = false;
				//printf("The num now is:%d.\n", num);
			}
		}
		//在图像里画出来，便于识别
		if (Point_is_Setted)
			circle(g_SrcImage, selection, 2, Scalar(255, 0, 0), 2, 8);
		// 显示原图
		if (num >-1)
		{
			polylines(g_SrcImage, ppt, npt, 1, 1, Scalar(255), 1, 8, 0);
			fillPoly(g_SrcImage, ppt, npt, 1, Scalar(255, 255, 255));
			char string1[5];//设置字符串缓存位置
			string numString("Num of list:");
			string pString("mmHg:");
			string nString("Refractivity:");
			sprintf_s(string1, "%d", num);      // 帧率保留两位小数
			numString += string1;                    // 在后加入数值字符串
			// 将帧率信息写在输出帧上
			putText(g_SrcImage,                  // 图像矩阵
				numString,                  // string型文字内容
				Point(0, 35),           // 文字坐标，以左下角为原点
				FONT_HERSHEY_DUPLEX,   // 字体类型
				1.3,                    // 字体大小
				cv::Scalar(0, 0, 0),    // 字体颜色
				2);

			putText(g_SrcImage,                  // 图像矩阵
				pString,                  // string型文字内容
				Point(0, 70),           // 文字坐标，以左下角为原点
				FONT_HERSHEY_DUPLEX,   // 字体类型
				1.3,                    // 字体大小
				cv::Scalar(0, 0, 0),    // 字体颜色
				2);

			putText(g_SrcImage,                  // 图像矩阵
				nString,                  // string型文字内容
				Point(0, 110),           // 文字坐标，以左下角为原点
				FONT_HERSHEY_DUPLEX,   // 字体类型
				1.3,                    // 字体大小
				cv::Scalar(0, 0, 0),    // 字体颜色
				2);
		}
		imshow(WINDOW_NAME1, g_SrcImage);
		// 显示二值图
		imshow(WINDOW_NAME2, g_BinaryImage);
		char key = (char)waitKey(15);//延时15ms,按Q键或Esc退出
		if (key == 27 || key == 'q' || key == 'Q')break;
	}
	destroyAllWindows();
	return 0;
}