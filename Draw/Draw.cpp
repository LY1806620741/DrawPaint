// Draw.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "Draw.h"
#include "list"
#include "windowsx.h"
#include "process.h"
#include "commdlg.h"

using namespace std;

#define MAX_LOADSTRING 100
#define Head_Len 20

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

//定义画图类型
enum DRAW_TYPE
{
	DRAW_NULL,
	DRAW_PAINT,
	DRAW_LINE,
	DRAW_RECTANGLE,
	DRAW_CIRCULAR,
	DRAW_BEZIER,
	DRAW_BEZIER_EDIT
};
struct stPicInfo//储存每一步的信息
{
	tagPOINT ptFirst;
	tagPOINT ptEnd;
	DRAW_TYPE type;
	COLORREF pencolor;
	int pointnum=0;
	list<tagPOINT> staLineInfo;
};

struct savestPicInfo//储存每一步的信息存到文件，取消掉list
{
	tagPOINT ptFirst;
	tagPOINT ptEnd;
	DRAW_TYPE type;
	COLORREF pencolor;
	int pointnum;
};

//list<tagPOINT> staLineInfo;//储存曲线等需要记录点的信息
//list<list<tagPOINT>> stLineInfo;//储存曲线所有需要记录点的信息


DRAW_TYPE m_DrawType = DRAW_PAINT;//初始化为画笔
list<stPicInfo> g_listPicInfo;//所有图形保存
stPicInfo g_curDrawPicInfo;//正在画的图形信息
bool g_bIsMouseDown = false;//当前鼠标是否按下
tagPOINT g_ptFirst;//当前正在画的鼠标的点
tagPOINT g_ptEnd;
HWND g_hwnd;
COLORREF penColor;
list<tagPOINT>::iterator Linepointtmp;
int step = 0;//步数

void OnLButtonDown(LPARAM lParam);
void OnLButtonUp(LPARAM lParam);
void DrawThread(void* param);
void DrawAll();
void DrawPic(stPicInfo info, HDC hDc);
void SavePicFile();
void OpenPicFile();
void OnMouseMove(LPARAM lParam);
void paint(stPicInfo& data, tagPOINT ptFirst, tagPOINT ptEnd);
void OnRButtonUp(LPARAM lParam);//右键弹起
void bezierLine(HDC hDc, list<tagPOINT> point,COLORREF penColor);//画bezier曲线
tagPOINT bezierpoit(double t,list<tagPOINT> point);//计算bezier点

//小函数
int fac(int num)//阶乘函数
{
	if (num == 0 || num == 1)
	{
		return 1;
	}

	int sum=1;
	for (int i=2;i <= num;i++)//代码优化省去*1
	{
		sum *= i;
	}
	return sum;
}
int c(int m,int n) //求组合值Cmn=n!/(n-m)!/m!,但是尽量减少不必要的阶乘
{
	if (m == 0)
	{
		return 1;
	}
	int sum=1;
	int i = n;
	for (int j=0;j < m;j++,i--)
	{
		sum *= i;
	}//求得n!/(n-m)!
	sum = sum / fac(m);
	return sum;
}

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DRAW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DRAW));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DRAW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DRAW);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   g_hwnd = hWnd;
   MoveWindow(hWnd, 300, 200, 800, 600, FALSE);//窗口位置大小
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   _beginthread(&DrawThread,0,NULL);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
			case ID_Drawpaint:
				m_DrawType = DRAW_PAINT;
				g_curDrawPicInfo.staLineInfo.clear();
				break;
			case ID_Drawline:
				m_DrawType = DRAW_LINE;
				g_curDrawPicInfo.staLineInfo.clear();
				break;
			case ID_Drawrectangle:
				m_DrawType = DRAW_RECTANGLE;
				g_curDrawPicInfo.staLineInfo.clear();
				break;
			case ID_Drawcircucle:
				m_DrawType = DRAW_CIRCULAR;
				g_curDrawPicInfo.staLineInfo.clear();
				break;
			case ID_save:
				SavePicFile();
				g_curDrawPicInfo.staLineInfo.clear();
				break;
			case ID_open:
				OpenPicFile();
				break;
			case ID_undo:
				if(!g_listPicInfo.empty())
					g_listPicInfo.pop_back();
				break;
			case ID_clear://清空画布菜单
				g_listPicInfo.clear();
				break;
			case ID_selectcolor://选择颜色菜单
			{
				CHOOSECOLOR stChooseColor;
				COLORREF dwCustColors[16];
				stChooseColor.lStructSize = sizeof(CHOOSECOLOR);
				stChooseColor.hwndOwner = hWnd;
				stChooseColor.rgbResult = penColor;
				stChooseColor.lpCustColors = (LPDWORD)dwCustColors;
				stChooseColor.Flags = CC_RGBINIT;
				stChooseColor.lCustData = 0;
				stChooseColor.lpfnHook = NULL;
				stChooseColor.lpTemplateName = NULL;
				if (ChooseColor(&stChooseColor))
				{
					penColor = stChooseColor.rgbResult;
				}
			}
				break;
			case ID_Drawbezier:
				step = 1;//开始选点
				g_curDrawPicInfo.staLineInfo.clear();
				m_DrawType = DRAW_BEZIER_EDIT;
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_LBUTTONDOWN:
		OnLButtonDown(lParam);
		break;
	case WM_LBUTTONUP:
		OnLButtonUp(lParam);
		break;
	case WM_MOUSEMOVE:
		OnMouseMove(lParam);
		break;
	case WM_RBUTTONUP:
		OnRButtonUp(lParam);
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void DrawThread(void* param)
{
	while (1)
	{
		DrawAll();
		Sleep(50);
	}

}

void DrawAll()
{
	HDC hDc = GetDC(g_hwnd);
	if (hDc == NULL)
	{
		return;
	}
	HDC dcMem = CreateCompatibleDC(hDc);
	HBITMAP bitmap = (HBITMAP)CreateCompatibleBitmap(hDc, 800, 600);
	SelectObject(dcMem, bitmap);
	//背景色设为白色
	HBRUSH hBrushBG = CreateSolidBrush(RGB(255, 255, 255));
	RECT rect;
	rect.left = 0;
	rect.right = 800;
	rect.top = 0;
	rect.bottom = 600;
	FillRect(dcMem, &rect, hBrushBG);

	for (list<stPicInfo>::iterator it = g_listPicInfo.begin();it != g_listPicInfo.end();it++)
	{
		DrawPic(*it, dcMem);//绘制到dcMem
	}
	if (g_bIsMouseDown||m_DrawType==DRAW_BEZIER_EDIT)//临时图像
	{
		//tagPOINT pt;
		//GetCursorPos(&pt);//鼠标行对雨电脑屏幕的点
		//ScreenToClient(g_hwnd, &pt);//转换成了窗口客户区坐标
		//memcpy(&g_curDrawPicInfo.ptEnd, &pt, sizeof(tagPOINT));
		DrawPic(g_curDrawPicInfo, dcMem);
	}
	BitBlt(hDc, 0, 0, 800, 600, dcMem, 0, 0, SRCCOPY);
	ReleaseDC(g_hwnd,hDc);
	DeleteObject(dcMem);
	DeleteObject(bitmap);
}

void DrawPic(stPicInfo info, HDC hDc) {
	switch (info.type)
	{
	case DRAW_NULL:
		break;
	case DRAW_PAINT:
	{
		if (info.staLineInfo.empty())
		{
			return;
		}
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(hDc, pen);
		SelectObject(hDc, pen);
		tagPOINT tmp;
		tmp.x = info.staLineInfo.begin()->x;
		tmp.y = info.staLineInfo.begin()->y;
		for (list<tagPOINT>::iterator it = info.staLineInfo.begin();it != info.staLineInfo.end();it++)
		{
			//SetPixel(hDc, it->x, it->y, info.pencolor);
			tagPOINT pt;
			MoveToEx(hDc, tmp.x, tmp.y, &pt);
			LineTo(hDc, it->x, it->y);
			tmp.x = it->x;
			tmp.y = it->y;
		}
		DeleteObject(old);
	}
		break;
	case DRAW_LINE:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(hDc, pen);
		SelectObject(hDc,pen);
		tagPOINT pt;
		MoveToEx(hDc, info.ptFirst.x, info.ptFirst.y, &pt);
		LineTo(hDc, info.ptEnd.x, info.ptEnd.y);
		DeleteObject(old);
	}
		break;
	case DRAW_RECTANGLE:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(hDc, pen);
		SelectObject(hDc, pen);
		HGDIOBJ hBrush = GetStockBrush(NULL_BRUSH);
		SelectObject(hDc, hBrush);
		Rectangle(hDc, info.ptFirst.x, info.ptFirst.y, info.ptEnd.x, info.ptEnd.y);
		DeleteObject(hBrush);
		DeleteObject(old);
	}
		break;
	case DRAW_CIRCULAR:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(hDc, pen);
		SelectObject(hDc, pen);
		HGDIOBJ hBrush = GetStockBrush(NULL_BRUSH);
		SelectObject(hDc, hBrush);
		Ellipse(hDc, info.ptFirst.x, info.ptFirst.y, info.ptEnd.x, info.ptEnd.y);
		DeleteObject(hBrush);
		DeleteObject(old);
	}
		break;
	case DRAW_BEZIER:
	{
		bezierLine(hDc, info.staLineInfo, info.pencolor);
	}
		break;
	case DRAW_BEZIER_EDIT:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(hDc, pen);
		SelectObject(hDc, pen);
		tagPOINT pt;
		for (list<tagPOINT>::iterator it = info.staLineInfo.begin();it != info.staLineInfo.end();it++)
		{
			if (it == info.staLineInfo.begin())
			{
				MoveToEx(hDc, it->x, it->y, &pt);
			}
			LineTo(hDc, it->x, it->y);
			HGDIOBJ hBrush = GetStockBrush(NULL_BRUSH);
			SelectObject(hDc, hBrush);
			Rectangle(hDc, it->x - 3, it->y - 3, it->x + 4, it->y + 4);
			DeleteObject(hBrush);
		}
		if (step==4&&info.staLineInfo.size() >= 2)//画线函数
		{
			bezierLine(hDc,info.staLineInfo, info.pencolor);
		}
		DeleteObject(old);
	}
	break;
	default:
		break;
	}
}

void OnLButtonDown(LPARAM lParam)
{
	if (g_bIsMouseDown)
	{
		return;
	}
	g_bIsMouseDown = true;
	g_ptFirst.x = GET_X_LPARAM(lParam);
	g_ptFirst.y = GET_Y_LPARAM(lParam);
	memcpy(&g_curDrawPicInfo.ptFirst, &g_ptFirst, sizeof(tagPOINT));
	g_curDrawPicInfo.type = m_DrawType;//临时图像
	g_curDrawPicInfo.pencolor = penColor;
	g_curDrawPicInfo.ptEnd = g_ptFirst;
	if (m_DrawType == DRAW_BEZIER_EDIT&&step==4)
	{
		for (list<tagPOINT>::iterator i = g_curDrawPicInfo.staLineInfo.begin();i != g_curDrawPicInfo.staLineInfo.end();i++)
		{
			RECT tmp = { i->x - 3,i->y - 3,i->x + 4,i->y + 4 };
			if (PtInRect(&tmp, g_ptFirst))
			{
				Linepointtmp = i;
			}
		}
	}
}
void OnMouseMove(LPARAM lParam)
{
	if (!g_bIsMouseDown)
	{
		return;
	}
	switch (m_DrawType)
	{
	case DRAW_PAINT:
		g_curDrawPicInfo.ptEnd.x = GET_X_LPARAM(lParam);
		g_curDrawPicInfo.ptEnd.y = GET_Y_LPARAM(lParam);
		paint(g_curDrawPicInfo,g_curDrawPicInfo.ptFirst, g_curDrawPicInfo.ptEnd);
		break;
	case DRAW_BEZIER_EDIT:
		if(step==4)
		{
			Linepointtmp->x = GET_X_LPARAM(lParam);
			Linepointtmp->y = GET_Y_LPARAM(lParam);
		}
		break;
	default:
		g_curDrawPicInfo.ptEnd.x = GET_X_LPARAM(lParam);
		g_curDrawPicInfo.ptEnd.y = GET_Y_LPARAM(lParam);
		break;
	}
}

void OnLButtonUp(LPARAM lParam)
{
	if (!g_bIsMouseDown)
	{
		return;
	}
	g_bIsMouseDown = false;
	g_ptEnd.x = GET_X_LPARAM(lParam);
	g_ptEnd.y = GET_Y_LPARAM(lParam);
	switch (m_DrawType)
	{
	case DRAW_NULL:
		break;
	case DRAW_PAINT:
	{
		stPicInfo stInfo;
		stInfo.type = m_DrawType;
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(tagPOINT));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(tagPOINT));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		//缓存储存到历史记录里
		tagPOINT tmp;
		for (list<tagPOINT>::iterator i = g_curDrawPicInfo.staLineInfo.begin();i != g_curDrawPicInfo.staLineInfo.end();i++)
		{
			tmp.x = i->x;
			tmp.y = i->y;
			stInfo.staLineInfo.push_back(tmp);
		}
		stInfo.pointnum = stInfo.staLineInfo.size();
		g_curDrawPicInfo.staLineInfo.clear();
		g_listPicInfo.push_back(stInfo);
	}
		break;
	case DRAW_LINE:
	{
		stPicInfo stInfo;
		stInfo.type = m_DrawType;
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(tagPOINT));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(tagPOINT));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		g_listPicInfo.push_back(stInfo);
	}
		break;
	case DRAW_RECTANGLE:
	{
		stPicInfo stInfo;
		stInfo.type = m_DrawType;
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(tagPOINT));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(tagPOINT));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		g_listPicInfo.push_back(stInfo);
	}
		break;
	case DRAW_CIRCULAR:
	{
		stPicInfo stInfo;
		stInfo.type = m_DrawType;
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(tagPOINT));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(tagPOINT));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		g_listPicInfo.push_back(stInfo);
	}
		break;
	case DRAW_BEZIER_EDIT:
	{
		if (step == 1)
		{
			g_curDrawPicInfo.staLineInfo.push_back(g_ptEnd);
		}
		if (step == 4)
		{
		}
			break;
	}
		break;
	default:
		break;
	}
}

void OnRButtonUp(LPARAM lParam) {//右键弹起
	switch (m_DrawType)
	{
	case DRAW_BEZIER_EDIT:
		if (g_curDrawPicInfo.staLineInfo.size()>=2)
		{
			switch(step)
			{
			case 1:
				step = 4;
				break;
			case 4:
			{
				stPicInfo stInfo;
				stInfo.type = DRAW_BEZIER;
				memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(tagPOINT));
				memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(tagPOINT));
				memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
				tagPOINT tmp;
				for (list<tagPOINT>::iterator i = g_curDrawPicInfo.staLineInfo.begin();i != g_curDrawPicInfo.staLineInfo.end();i++)
				{
					tmp.x = i->x;
					tmp.y = i->y;
					stInfo.staLineInfo.push_back(tmp);
				}
				stInfo.pointnum = stInfo.staLineInfo.size();
				g_listPicInfo.push_back(stInfo);
				g_curDrawPicInfo.staLineInfo.clear();
				step = 1;
			}
			break;
			}
		}
		break;
	default:
		break;
	}
}

void SavePicFile()
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	TCHAR filename[MAX_PATH] = { 0 };
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"*.DPic";
	ofn.lpstrDefExt = L"DPic";
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hwnd;


	if (GetSaveFileName(&ofn))
	{
		size_t len = wcslen(ofn.lpstrFile) + 1;
		size_t connerted = 0;
		char strFileName[MAX_PATH] = { 0 };
		wcstombs_s(&connerted, strFileName, len, ofn.lpstrFile, _TRUNCATE);
		FILE* pFile;
		fopen_s(&pFile, strFileName, "w");
		if (pFile == NULL)
		{
			return;
		}

		char buf[100];//文件头信息
		sprintf_s(buf, "%d", g_listPicInfo.size());
		fwrite(buf, Head_Len, 1, pFile);

		savestPicInfo tmp;
		for (list<stPicInfo>::iterator it = g_listPicInfo.begin();it != g_listPicInfo.end();it++)
		{
			tmp.pencolor = it->pencolor;
			tmp.pointnum = it->pointnum;
			tmp.ptEnd = it->ptEnd;
			tmp.ptFirst = it->ptFirst;
			tmp.type = it->type;
			fwrite(&tmp, sizeof(savestPicInfo), 1, pFile);
			if (it->pointnum > 0)//是点集类
			{
				for (list<tagPOINT>::iterator i = it->staLineInfo.begin();i != it->staLineInfo.end();i++)
				{
					fwrite(&(*i), sizeof(tagPOINT), 1, pFile);
				}
			}
		}
		fclose(pFile);
	}
}
void OpenPicFile()
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	TCHAR filename[MAX_PATH] = { 0 };
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hwnd;
	if (GetOpenFileName(&ofn))
	{
		size_t len = wcslen(ofn.lpstrFile) + 1;
		size_t connerted = 0;
		char strFileName[MAX_PATH] = { 0 };
		wcstombs_s(&connerted, strFileName, len, ofn.lpstrFile, _TRUNCATE);
		FILE* pFile;
		fopen_s(&pFile, strFileName, "r");
		if (pFile == NULL)
		{
			return;
		}
		g_listPicInfo.clear();//清空原来的图形
		//怎么写的数据就要怎么读取，一一对应
		char bufTemp[100] = { 0 };
		fread(bufTemp, Head_Len, 1, pFile);	//头信息

		int sizePic = atoi(bufTemp);
		for (int i = 0;i < sizePic;i++)
		{
			savestPicInfo readstInfo;
			stPicInfo stInfo;
			fread(&readstInfo, sizeof(readstInfo), 1, pFile);
			stInfo.pencolor = readstInfo.pencolor;
			stInfo.pointnum = readstInfo.pointnum;
			stInfo.ptEnd = readstInfo.ptEnd;
			stInfo.ptFirst = readstInfo.ptFirst;
			stInfo.type = readstInfo.type;
			if (stInfo.pointnum > 0)//是点集类
			{
				tagPOINT tmp;
				for (int j = 0;j < stInfo.pointnum;j++)
				{
					fread(&tmp, sizeof(tagPOINT), 1, pFile);
					stInfo.staLineInfo.push_back(tmp);
				}
			}
			g_listPicInfo.push_back(stInfo);
			stInfo.staLineInfo.clear();
		}
		fclose(pFile);
	}
}

void paint(stPicInfo& data, tagPOINT ptFirst, tagPOINT ptEnd)
{
	int len_x = ptEnd.x - ptFirst.x;
	int len_y = ptEnd.y - ptFirst.y;
	if (!len_x && !len_y)
	{
		return;
	}
	/*
	double length = sqrt(len_x*len_x + len_y*len_y);
	int step_x = len_x / length;
	int step_y = len_y / length;
	//tagPOINT point;
	for (int i=0;i < (int)length;i++)
	{
		point.x = ptFirst.x + step_x;
		point.y = ptFirst.y + step_y;
		data.staLineInfo.push_back(point);
	}*/
	data.staLineInfo.push_back(ptEnd);
	data.ptFirst = data.ptEnd;
}

void bezierLine(HDC hDc, list<tagPOINT> point, COLORREF penColor)
{
	if (point.size() < 2)
	{
		return;
	}
	tagPOINT tmp;
	for (double i = 0.000;i < 1;i += 0.001)//画每一点
	{
		tmp=bezierpoit(i, point);
		SetPixel(hDc, tmp.x, tmp.y,penColor);
	}
}
tagPOINT bezierpoit(double t,list<tagPOINT> point)
{
	//double f0 = 1.0;
	//double f1 = 3 * t - 3 * t*t + t*t*t;
	//double f2 = 3 * t*t - 2 * t*t*t;
	//double f3 = t*t*t;
	tagPOINT tmp;
	if (t == 0)
	{
		tmp.x = point.begin()->x;
		tmp.y = point.begin()->y;
		return tmp;
	}
	list<double> f;//储存基函数
	//计算基函数
	f.push_back(1.0);//代码优化
	int n = point.size()-1;
	for (int i = 1;i<n+1;i++)
	{
		double temp=0;
		for (int j = i;j < n+1;j++)
		{
			temp += pow(-1, i + j)*c(j, n)*c(i - 1, j - 1)*pow(t, j);//基函数公式
			//temp += pow(t, j)*pow((1 - t), n - j);//伯恩斯坦多項式
		}
		f.push_back(temp);
	}
	list<tagPOINT>::iterator i = point.begin();
	list<tagPOINT>::iterator j = point.begin();//上一个
	list<double>::iterator k = f.begin();
	double x;
	double y;
	for (x=i->x,y=i->y,i++,k++;i != point.end();i++,j++,k++)
	{
		x = x + (i->x - j->x)**k;
		y = y + (i->y - j->y)**k;
	}
	//求得坐标

	/*错误的近似的认知
	if (point.size() == 2)//两个点
	{
		double f1 = 3 * t - 3 * t*t + t*t*t;
		x = i->x;
		y = i->y;
		i++;
		x = x + (i->x-j->x)*f1;
		y = y + (i->y-j->y)*f1;
	}
	if (point.size() == 3)//三个点
	{
		double f1 = 3 * t - 3 * t*t + t*t*t;
		double f2 = 3 * t*t - 2 * t*t*t;
		x = i->x;
		y = i->y;
		i++;
		x = x + (i->x-j->x)*f1;
		y = y + (i->y-j->y)*f1;
		i++;
		j++;
		x = x + (i->x-j->x)*f2;
		y = y + (i->y-j->y)*f2;
	}
	if (point.size() > 3)
	{
		double f1 = 3 * t - 3 * t*t + t*t*t;
		double f2 = 3 * t*t - 2 * t*t*t;
		double f3 = t*t*t;
		x = i->x;
		y = i->y;
		i++;
		x = x + (i->x-j->x)*f1;
		y = y + (i->y-j->y)*f1;
		i++;
		j++;
		x = x + (i->x-j->x)*f2;
		y = y + (i->y-j->y)*f2;
		for (i++,j++;i != point.end();i++,j++)
		{
			x = x + (i->x-j->x)*f3;
			y = y + (i->y-j->y)*f3;
		}
	}*/
	tmp.x = (int)x;
	tmp.y = (int)y;
	return tmp;
}