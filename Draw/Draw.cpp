// Draw.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "Draw.h"
#include "list"
#include "windowsx.h"
#include "process.h"
#include "commdlg.h"
#include "vector"

using namespace std;

#define MAX_LOADSTRING 100
#define Head_Len 20
#define TIME 50

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

struct Point
{
	long double x;
	long double y;
};

//定义画图类型
enum DRAW_TYPE
{
	DRAW_NULL,
	DRAW_PAINT,
	DRAW_LINE,
	DRAW_RECTANGLE,
	DRAW_CIRCULAR,
	DRAW_BEZIER,
	DRAW_BEZIER_EDIT,
	DRAW_BEZIER_SHOW

};
struct stPicInfo//储存每一步的信息
{
	Point ptFirst;
	Point ptEnd;
	DRAW_TYPE type;
	COLORREF pencolor;
	int pointnum=0;
	list<Point> staLineInfo;
};

struct savestPicInfo//储存每一步的信息存到文件，取消掉list
{
	Point ptFirst;
	Point ptEnd;
	DRAW_TYPE type;
	COLORREF pencolor;
	int pointnum;
};

//list<Point> staLineInfo;//储存曲线等需要记录点的信息
//list<list<Point>> stLineInfo;//储存曲线所有需要记录点的信息

DRAW_TYPE m_DrawType = DRAW_PAINT;//初始化为画笔
//list<stPicInfo> g_listPicInfo;//所有图形保存
stPicInfo g_curDrawPicInfo;//正在画的图形信息
bool g_bIsMouseDown = false;//当前鼠标是否按下
Point g_ptFirst;//当前正在画的鼠标的点
Point g_ptEnd;
HWND g_hwnd;
COLORREF penColor;
list<Point>::iterator Linepointtmp;
int step = 0;//步数

void bezierLine(HDC hDc, list<Point> point,COLORREF penColor);//画bezier曲线
void DrawThread(void* param);
class CPicInfo {
	CRITICAL_SECTION m_cs;
	list<stPicInfo> m_listPicInfo;//所有图形保存
public:
	CPicInfo() {
		InitializeCriticalSection(&m_cs);
	}
	~CPicInfo() {
		DeleteCriticalSection(&m_cs);
	}
	void lock() {
		EnterCriticalSection(&m_cs);
	}
	void unlock() {
		LeaveCriticalSection(&m_cs);
	}
	void push(stPicInfo pi) {
		lock();
		m_listPicInfo.push_back(pi);
		unlock();
	}
	void clear() {
		lock();
		m_listPicInfo.clear();
		unlock();
	}
	void DrawALL(HDC dcMem) {
		lock();
		for (list<stPicInfo>::iterator it = m_listPicInfo.begin();it != m_listPicInfo.end();it++)
		{
			DrawPic(*it, dcMem);//绘制到dcMem
		}
		unlock();
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
			Point tmp;
			tmp.x = info.staLineInfo.begin()->x;
			tmp.y = info.staLineInfo.begin()->y;
			for (list<Point>::iterator it = info.staLineInfo.begin();it != info.staLineInfo.end();it++)
			{
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
			SelectObject(hDc, pen);
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
			for (list<Point>::iterator it = info.staLineInfo.begin();it != info.staLineInfo.end();it++)
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
			if (step == 4 && info.staLineInfo.size() >= 2)//画线函数
			{
				bezierLine(hDc, info.staLineInfo, info.pencolor);
			}
			DeleteObject(old);
		}
		break;
		default:
			break;
		}
	}
	void undo() {
		lock();
		if (!m_listPicInfo.empty()) {
			m_listPicInfo.pop_back();
		}
		unlock();
	}
	int size() {
		lock();
		return m_listPicInfo.size();
		unlock();
	}
	void save() {
		lock();
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
			
			fopen_s(&pFile, strFileName, "wb");
			
			if (pFile == NULL)
			{
				unlock();
				return;
			}
			char buf[100];//文件头信息
			sprintf_s(buf, "%d", m_listPicInfo.size());
			
			fwrite(buf, Head_Len, 1, pFile);
			savestPicInfo tmp;
			for (list<stPicInfo>::iterator it = m_listPicInfo.begin();it != m_listPicInfo.end();it++)
			{
				tmp.pencolor = it->pencolor;
				tmp.pointnum = it->pointnum;
				tmp.ptEnd = it->ptEnd;
				tmp.ptFirst = it->ptFirst;
				tmp.type = it->type;
				fwrite(&tmp, sizeof(savestPicInfo), 1, pFile);
				if (it->pointnum > 0)//是点集类
				{
					for (list<Point>::iterator i = it->staLineInfo.begin();i != it->staLineInfo.end();i++)
					{
						int t = fwrite(&(*i), sizeof(Point), 1, pFile);
						if (t != 1)
						{
							MessageBox(g_hwnd, L"保存出错", L"", NULL);
						};
					}
				}
			}
			fclose(pFile);
		}
		unlock();

	}
	void open() {
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
			fopen_s(&pFile, strFileName, "rb");
			if (pFile == NULL)
			{
				return;
			}
			clear();//清空原来的图形
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
					Point tmp;
					for (int j = 0;j < stInfo.pointnum;j++)
					{
						int t = fread(&tmp, sizeof(Point), 1, pFile);
						if (t != 1)
						{
							MessageBox(g_hwnd, L"读取不成功", L"", NULL);
							stInfo.staLineInfo.clear();
							stInfo.pointnum = 0;
							return;
						};
						if (ferror(pFile))
						{
							MessageBox(g_hwnd, L"读取出错", L"", NULL);
							stInfo.staLineInfo.clear();
							stInfo.pointnum = 0;
							return;
						};
						if (ferror(pFile))
						{
							MessageBox(g_hwnd, L"文件尾", L"", NULL);
						}
						stInfo.staLineInfo.push_back(tmp);
					}
				}
				push(stInfo);
				stInfo.staLineInfo.clear();
			}
			fclose(pFile);
		}
	}
};
CPicInfo g_PicInfo;
void OnLButtonDown(LPARAM lParam);
void OnLButtonUp(LPARAM lParam);
void DrawAll();
void DrawPic(stPicInfo info, HDC hDc,HDC dcMem);
void SavePicFile();
void OpenPicFile();
void OnMouseMove(LPARAM lParam);
void paint(stPicInfo& data, Point ptFirst, Point ptEnd);
void OnRButtonUp(LPARAM lParam);//右键弹起
Point bezierpoit(double t,list<Point> point);//计算bezier点


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

   //MessageBox(hWnd, L"1.由于使用了多线程，导致了list不安全，在保存撤销清空等需要操作list数组的线程开始时，如果list正在使用，并被修改就会抛出崩溃",L"存在问题",NULL);
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
				break;
			case ID_open:
				OpenPicFile();
				break;
			case ID_undo:
				g_PicInfo.undo();
				break;
			case ID_clear://清空画布菜单
				g_PicInfo.clear();
				break;
			case ID_bezier_up:
			{
				int num = 1;
				Point prpoint;
				for (list<Point>::iterator it = g_curDrawPicInfo.staLineInfo.begin();it != g_curDrawPicInfo.staLineInfo.end();it++)
				{
					if (it == g_curDrawPicInfo.staLineInfo.begin())
					{
						prpoint.x = it->x;
						prpoint.y = it->y;
						continue;
					}
					double t = (num+0.0) / (g_curDrawPicInfo.pointnum);
					num++;
					Point temp;
					temp.x = (1 - t)*it->x + t*prpoint.x;
					temp.y = (1 - t)*it->y + t*prpoint.y;
					prpoint.x = it->x;
					prpoint.y = it->y;
					it->x = temp.x;
					it->y = temp.y;
				}
				g_curDrawPicInfo.staLineInfo.push_back(prpoint);
				g_curDrawPicInfo.pointnum++;
			}
				break;
			case ID_bezier_down:
			{
				if (g_curDrawPicInfo.staLineInfo.size() < 4)
				{
					break;
				}
				g_curDrawPicInfo.ptEnd.x = g_curDrawPicInfo.staLineInfo.rbegin()->x;
				g_curDrawPicInfo.ptEnd.y = g_curDrawPicInfo.staLineInfo.rbegin()->y;
				int num = 1;
				Point prpoint;
				g_curDrawPicInfo.pointnum--;
				for (list<Point>::iterator it = g_curDrawPicInfo.staLineInfo.begin();it != g_curDrawPicInfo.staLineInfo.end();it++)
				{
					if (it == g_curDrawPicInfo.staLineInfo.begin())
					{
						prpoint.x = it->x;
						prpoint.y = it->y;
						continue;
					}
					if (num == g_curDrawPicInfo.pointnum-1)
					{
						it->x = g_curDrawPicInfo.ptEnd.x;
						it->y = g_curDrawPicInfo.ptEnd.y;
						break;
					}
					double t =  (num + 0.0)/(g_curDrawPicInfo.pointnum);
					num++;
					it->x = (it->x - t*prpoint.x) / (1 - t);
					it->y = (it->y - t*prpoint.y) / (1 - t);
					prpoint.x = it->x;
					prpoint.y = it->y;
					
				}
			}
			g_curDrawPicInfo.staLineInfo.pop_back();
				break;
			case ID_bezier_show://演示bezier曲线画法
			{
				g_curDrawPicInfo.type = DRAW_BEZIER_SHOW;
			}
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
            //PAINTSTRUCT ps;
            //HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
           // EndPaint(hWnd, &ps);
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
		Sleep(TIME);
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

	g_PicInfo.DrawALL(dcMem);
	if (g_bIsMouseDown||m_DrawType==DRAW_BEZIER_EDIT)//临时图像
	{
		DrawPic(g_curDrawPicInfo, hDc, dcMem);
	}
	BitBlt(hDc, 0, 0, 800, 600, dcMem, 0, 0, SRCCOPY);
	ReleaseDC(g_hwnd,hDc);
	DeleteObject(dcMem);
	DeleteObject(bitmap);
}

void DrawPic(stPicInfo info, HDC hDc,HDC dcMem) {
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
		HPEN old = (HPEN)SelectObject(dcMem, pen);
		SelectObject(dcMem, pen);
		Point tmp;
		tmp.x = info.staLineInfo.begin()->x;
		tmp.y = info.staLineInfo.begin()->y;
		for (list<Point>::iterator it = info.staLineInfo.begin();it != info.staLineInfo.end();it++)
		{
			tagPOINT pt;
			MoveToEx(dcMem, tmp.x, tmp.y, &pt);
			LineTo(dcMem, it->x, it->y);
			tmp.x = it->x;
			tmp.y = it->y;
		}
		DeleteObject(old);
	}
		break;
	case DRAW_LINE:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(dcMem, pen);
		SelectObject(dcMem,pen);
		tagPOINT pt;
		MoveToEx(dcMem, info.ptFirst.x, info.ptFirst.y, &pt);
		LineTo(dcMem, info.ptEnd.x, info.ptEnd.y);
		DeleteObject(old);
	}
		break;
	case DRAW_RECTANGLE:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(dcMem, pen);
		SelectObject(dcMem, pen);
		HGDIOBJ hBrush = GetStockBrush(NULL_BRUSH);
		SelectObject(dcMem, hBrush);
		Rectangle(dcMem, info.ptFirst.x, info.ptFirst.y, info.ptEnd.x, info.ptEnd.y);
		DeleteObject(hBrush);
		DeleteObject(old);
	}
		break;
	case DRAW_CIRCULAR:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(dcMem, pen);
		SelectObject(dcMem, pen);
		HGDIOBJ hBrush = GetStockBrush(NULL_BRUSH);
		SelectObject(dcMem, hBrush);
		Ellipse(dcMem, info.ptFirst.x, info.ptFirst.y, info.ptEnd.x, info.ptEnd.y);
		DeleteObject(hBrush);
		DeleteObject(old);
	}
		break;
	case DRAW_BEZIER:
	{
		bezierLine(dcMem, info.staLineInfo, info.pencolor);
	}
		break;
	case DRAW_BEZIER_EDIT:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, info.pencolor);
		HPEN old = (HPEN)SelectObject(dcMem, pen);
		SelectObject(dcMem, pen);
		tagPOINT pt;
		for (list<Point>::iterator it = info.staLineInfo.begin();it != info.staLineInfo.end();it++)
		{
			if (it == info.staLineInfo.begin())
			{
				MoveToEx(dcMem, it->x, it->y, &pt);
			}
			LineTo(dcMem, it->x, it->y);
			HGDIOBJ hBrush = GetStockBrush(NULL_BRUSH);
			HPEN penr = CreatePen(PS_SOLID, 1, RGB(255,0,0));
			SelectObject(dcMem, hBrush);
			SelectObject(dcMem, penr);
			Rectangle(dcMem, it->x - 3, it->y - 3, it->x + 4, it->y + 4);
			DeleteObject(hBrush);
			DeleteObject(penr);
			SelectObject(dcMem, pen);
		}
		if (step==4&&info.staLineInfo.size() >= 2)//画线函数
		{
			bezierLine(dcMem,info.staLineInfo, info.pencolor);
		}
		DeleteObject(old);
	}
	break;
	case DRAW_BEZIER_SHOW:
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(255,0,0));
		HPEN old = (HPEN)SelectObject(dcMem, pen);
		SelectObject(dcMem, pen);
		tagPOINT pt;
		for (list<Point>::iterator it = info.staLineInfo.begin();it != info.staLineInfo.end();it++)
		{
			Sleep(200);
			if (it == info.staLineInfo.begin())
			{
				MoveToEx(dcMem, it->x, it->y, &pt);
			}
			LineTo(dcMem, it->x, it->y);
			HGDIOBJ hBrush = GetStockBrush(NULL_BRUSH);
			SelectObject(dcMem, hBrush);
			Ellipse(dcMem, it->x - 3, it->y - 3, it->x + 4, it->y + 4);
			DeleteObject(hBrush);
			
			BitBlt(hDc, 0, 0, 800, 600, dcMem, 0, 0, SRCCOPY);
			
		}
		HDC dcMem2 = CreateCompatibleDC(hDc);
		HDC Action = CreateCompatibleDC(hDc);
		HBITMAP bitmap2 = (HBITMAP)CreateCompatibleBitmap(hDc, 800, 600);
		HBITMAP bitmap3 = (HBITMAP)CreateCompatibleBitmap(hDc, 800, 600);
		SelectObject(dcMem2, bitmap2);
		SelectObject(Action, bitmap3);
		//SetBkColor(dcMem2,RGB(0,0,0));
		BitBlt(dcMem2, 0, 0, 800, 600, dcMem, 0, 0, SRCCOPY);
		
		HBRUSH hBrushBG = CreateSolidBrush(RGB(255, 255, 255));
		RECT rect;
		rect.left = 0;
		rect.right = 800;
		rect.top = 0;
		rect.bottom = 600;
		//SetBkColor(Action,RGB(255,255,255));
		Point tmp;
		for (double i = 0.0;i < 1;i += 0.001)//画每一点
		{
			//BitBlt(Action, 0, 0, 800, 600, dcMem, 0, 0, SRCCOPY);
			
			FillRect(Action, &rect, hBrushBG);
			auto itpr = g_curDrawPicInfo.staLineInfo.begin();
			auto it = g_curDrawPicInfo.staLineInfo.begin();
			for (it++;it != g_curDrawPicInfo.staLineInfo.end();it++,itpr++)
			{
				int x = 1.0*itpr->x + i*(it->x - itpr->x);
				int y = 1.0*itpr->y + i*(it->y - itpr->y);
				Ellipse(Action, x - 1, y - 1, x + 2, y + 2);
			}
			tmp = bezierpoit(i, g_curDrawPicInfo.staLineInfo);
			SetPixel(dcMem2, tmp.x, tmp.y,penColor);//画点
			BitBlt(hDc, 0, 0, 800, 600, dcMem2, 0, 0, SRCAND);
			BitBlt(hDc, 0, 0, 800, 600, Action, 0, 0, SRCAND);
		}
		DeleteObject(dcMem2);
		DeleteObject(bitmap2);
		DeleteObject(Action);
		DeleteObject(bitmap3);
		DeleteObject(hBrushBG);
		DeleteObject(old);
		g_curDrawPicInfo.type = DRAW_BEZIER_EDIT;
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
	memcpy(&g_curDrawPicInfo.ptFirst, &g_ptFirst, sizeof(Point));
	g_curDrawPicInfo.type = m_DrawType;//临时图像
	g_curDrawPicInfo.pencolor = penColor;
	g_curDrawPicInfo.ptEnd = g_ptFirst;
	if (m_DrawType == DRAW_BEZIER_EDIT&&step==4)
	{
		Linepointtmp = g_curDrawPicInfo.staLineInfo.end();
		for (list<Point>::iterator i = g_curDrawPicInfo.staLineInfo.begin();i != g_curDrawPicInfo.staLineInfo.end();i++)
		{
			RECT tmp = { i->x - 3,i->y - 3,i->x + 4,i->y + 4 };
			tagPOINT g_ptFirst1;
			g_ptFirst1.x = g_ptFirst.x;
			g_ptFirst1.y = g_ptFirst.y;
			if (PtInRect(&tmp, g_ptFirst1))
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
		if (step == 4)
		{
			if (Linepointtmp != g_curDrawPicInfo.staLineInfo.end())
			{
				Linepointtmp->x = GET_X_LPARAM(lParam);
				Linepointtmp->y = GET_Y_LPARAM(lParam);
			}
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
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(Point));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(Point));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		//缓存储存到历史记录里
		Point tmp;
		for (list<Point>::iterator i = g_curDrawPicInfo.staLineInfo.begin();i != g_curDrawPicInfo.staLineInfo.end();i++)
		{
			tmp.x = i->x;
			tmp.y = i->y;
			stInfo.staLineInfo.push_back(tmp);
		}
		stInfo.pointnum = stInfo.staLineInfo.size();
		g_curDrawPicInfo.staLineInfo.clear();
		g_PicInfo.push(stInfo);
	}
		break;
	case DRAW_LINE:
	{
		stPicInfo stInfo;
		stInfo.type = m_DrawType;
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(Point));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(Point));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		g_PicInfo.push(stInfo);
	}
		break;
	case DRAW_RECTANGLE:
	{
		stPicInfo stInfo;
		stInfo.type = m_DrawType;
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(Point));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(Point));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		g_PicInfo.push(stInfo);
	}
		break;
	case DRAW_CIRCULAR:
	{
		stPicInfo stInfo;
		stInfo.type = m_DrawType;
		memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(Point));
		memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(Point));
		memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
		g_PicInfo.push(stInfo);
	}
		break;
	case DRAW_BEZIER_EDIT:
	{
		if (step == 1)
		{
			g_curDrawPicInfo.staLineInfo.push_back(g_ptEnd);
			g_curDrawPicInfo.ptEnd.x = g_ptEnd.x;
			g_curDrawPicInfo.ptEnd.y = g_ptEnd.y;
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
				EnableMenuItem(GetMenu(g_hwnd), ID_bezier_up,MF_ENABLED);
				EnableMenuItem(GetMenu(g_hwnd), ID_bezier_down,MF_ENABLED);
				EnableMenuItem(GetMenu(g_hwnd), ID_bezier_show, MF_ENABLED);
				DrawMenuBar(g_hwnd);
				g_curDrawPicInfo.pointnum = g_curDrawPicInfo.staLineInfo.size();
				break;
			case 4:
			{
				stPicInfo stInfo;
				stInfo.type = DRAW_BEZIER;
				memcpy(&stInfo.ptFirst, &g_ptFirst, sizeof(Point));
				memcpy(&stInfo.ptEnd, &g_ptEnd, sizeof(Point));
				memcpy(&stInfo.pencolor, &penColor, sizeof(COLORREF));
				Point tmp;
				for (list<Point>::iterator i = g_curDrawPicInfo.staLineInfo.begin();i != g_curDrawPicInfo.staLineInfo.end();i++)
				{
					tmp.x = i->x;
					tmp.y = i->y;
					stInfo.staLineInfo.push_back(tmp);
				}
				stInfo.pointnum = stInfo.staLineInfo.size();
				g_PicInfo.push(stInfo);
				g_curDrawPicInfo.staLineInfo.clear();
				step = 1;
				EnableMenuItem(GetMenu(g_hwnd), ID_bezier_up, MF_DISABLED);
				EnableMenuItem(GetMenu(g_hwnd), ID_bezier_down, MF_DISABLED);
				EnableMenuItem(GetMenu(g_hwnd), ID_bezier_show, MF_DISABLED);
				DrawMenuBar(g_hwnd);
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
	g_PicInfo.save();
}
void OpenPicFile()
{
	g_PicInfo.open();
}

void paint(stPicInfo& data, Point ptFirst, Point ptEnd)
{
	int len_x = ptEnd.x - ptFirst.x;
	int len_y = ptEnd.y - ptFirst.y;
	if (!len_x && !len_y)
	{
		return;
	}
	data.staLineInfo.push_back(ptEnd);
	data.ptFirst = data.ptEnd;
}

void bezierLine(HDC hDc, list<Point> point, COLORREF penColor)
{
	if (point.size() < 2)
	{
		return;
	}
	Point tmp;
	tagPOINT pt;
	tmp = bezierpoit(0, point);
	MoveToEx(hDc, tmp.x, tmp.y,&pt);
	for (double i = 0.000;i <= 1.01;i += 0.01)//画每一点
	{
		tmp=bezierpoit(i, point);
		//SetPixel(hDc, tmp.x, tmp.y,penColor);//画点
		LineTo(hDc, tmp.x, tmp.y);
	}
}
Point bezierpoit(double t,list<Point> point)
{
	//t = round(t*100) / 100.0;
	vector<Point> controlpoint;
	for (list<Point>::iterator it = point.begin();it != point.end();it++)
	{
		Point tmp;
		tmp.x = it->x;
		tmp.y = it->y;
		controlpoint.push_back(tmp);
	}

	for (int i = 1; i < point.size(); i++)
	{
		for (int j = 0; j < point.size() - i; j++)
		{
				controlpoint[j].x = (1 - t)*controlpoint[j].x + t * controlpoint[j + 1].x;
				controlpoint[j].y = (1 - t)*controlpoint[j].y + t * controlpoint[j + 1].y;
		}
	}
	Point tmp;
	tmp.x = controlpoint[0].x;
	tmp.y = controlpoint[0].y;
	return tmp;
}