// Draw.cpp : ����Ӧ�ó������ڵ㡣
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

// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������

//���廭ͼ����
enum DRAW_TYPE
{
	DRAW_NULL,
	DRAW_PAINT,
	DRAW_LINE,
	DRAW_RECTANGLE,
	DRAW_CIRCULAR,
	DRAW_BEZIER
};
struct stPicInfo//����ÿһ������Ϣ
{
	tagPOINT ptFirst;
	tagPOINT ptEnd;
	DRAW_TYPE type;
	COLORREF pencolor;
	int pointnum;
	list<tagPOINT> staLineInfo;
};

struct savestPicInfo//����ÿһ������Ϣ�浽�ļ���ȡ����list
{
	tagPOINT ptFirst;
	tagPOINT ptEnd;
	DRAW_TYPE type;
	COLORREF pencolor;
	int pointnum;
};

//list<tagPOINT> staLineInfo;//�������ߵ���Ҫ��¼�����Ϣ
//list<list<tagPOINT>> stLineInfo;//��������������Ҫ��¼�����Ϣ


DRAW_TYPE m_DrawType = DRAW_PAINT;//��ʼ��Ϊ����
list<stPicInfo> g_listPicInfo;//����ͼ�α���
stPicInfo g_curDrawPicInfo;//���ڻ���ͼ����Ϣ
bool g_bIsMouseDown = false;//��ǰ����Ƿ���
tagPOINT g_ptFirst;//��ǰ���ڻ������ĵ�
tagPOINT g_ptEnd;
HWND g_hwnd;
COLORREF penColor;

void OnLButtonDown(LPARAM lParam);
void OnLButtonUp(LPARAM lParam);
void DrawThread(void* param);
void DrawAll();
void DrawPic(stPicInfo info, HDC hDc);
void SavePicFile();
void OpenPicFile();
void OnMouseMove(LPARAM lParam);
void paint(stPicInfo& data, tagPOINT ptFirst, tagPOINT ptEnd);

// �˴���ģ���а����ĺ�����ǰ������: 
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

    // TODO: �ڴ˷��ô��롣

    // ��ʼ��ȫ���ַ���
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DRAW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ִ��Ӧ�ó����ʼ��: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DRAW));

    MSG msg;

    // ����Ϣѭ��: 
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
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
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
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   g_hwnd = hWnd;
   MoveWindow(hWnd, 300, 200, 800, 600, FALSE);//����λ�ô�С
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   _beginthread(&DrawThread,0,NULL);

   return TRUE;
}

//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND  - ����Ӧ�ó���˵�
//  WM_PAINT    - ����������
//  WM_DESTROY  - �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // �����˵�ѡ��: 
            switch (wmId)
            {
			case ID_Drawpaint:
				m_DrawType = DRAW_PAINT;
				break;
			case ID_Drawline:
				m_DrawType = DRAW_LINE;
				break;
			case ID_Drawrectangle:
				m_DrawType = DRAW_RECTANGLE;
				break;
			case ID_Drawcircucle:
				m_DrawType = DRAW_CIRCULAR;
				break;
			case ID_save:
				SavePicFile();
				break;
			case ID_open:
				OpenPicFile();
				break;
			case ID_undo:
				if(!g_listPicInfo.empty())
					g_listPicInfo.pop_back();
				break;
			case ID_clear://��ջ����˵�
				g_listPicInfo.clear();
				break;
			case ID_selectcolor://ѡ����ɫ�˵�
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
            // TODO: �ڴ˴�����ʹ�� hdc ���κλ�ͼ����...
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
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// �����ڡ������Ϣ��������
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
	//����ɫ��Ϊ��ɫ
	HBRUSH hBrushBG = CreateSolidBrush(RGB(255, 255, 255));
	RECT rect;
	rect.left = 0;
	rect.right = 800;
	rect.top = 0;
	rect.bottom = 600;
	FillRect(dcMem, &rect, hBrushBG);

	for (list<stPicInfo>::iterator it = g_listPicInfo.begin();it != g_listPicInfo.end();it++)
	{
		DrawPic(*it, dcMem);//���Ƶ�dcMem
	}
	if (g_bIsMouseDown)//��ʱͼ��
	{
		//tagPOINT pt;
		//GetCursorPos(&pt);//����ж��������Ļ�ĵ�
		//ScreenToClient(g_hwnd, &pt);//ת�����˴��ڿͻ�������
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
			SetPixel(hDc, it->x, it->y, info.pencolor);
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
	g_curDrawPicInfo.type = m_DrawType;//��ʱͼ��
	g_curDrawPicInfo.pencolor = penColor;
	g_curDrawPicInfo.ptEnd = g_ptFirst;
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
		//���洢�浽��ʷ��¼��
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
	case DRAW_BEZIER:
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

		char buf[100];//�ļ�ͷ��Ϣ
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
			if (it->pointnum > 0)//�ǵ㼯��
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
		g_listPicInfo.clear();//���ԭ����ͼ��
		//��ôд�����ݾ�Ҫ��ô��ȡ��һһ��Ӧ
		char bufTemp[100] = { 0 };
		fread(bufTemp, Head_Len, 1, pFile);	//ͷ��Ϣ

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
			if (stInfo.pointnum > 0)//�ǵ㼯��
			{
				tagPOINT tmp;
				for (int j = 0;j < stInfo.pointnum;j++)
				{
					fread(&tmp, sizeof(tagPOINT), 1, pFile);
					stInfo.staLineInfo.push_back(tmp);
				}
			}
			g_listPicInfo.push_back(stInfo);
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