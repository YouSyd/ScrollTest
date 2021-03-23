#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include<gdiplus.h>
using namespace Gdiplus; 

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"Comctl32.lib")
#pragma comment(lib,"gdiplus.lib")

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

int Text_Append(HWND hwnd,char* ptext);
void GradientRect(HDC hdc,RECT rc,COLORREF c1,COLORREF c2,int verical_or_horizen);
LRESULT CALLBACK HeadScrollTestProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);

POINT StartDrag;
POINT StartPos;
BOOL StartDragFlag=FALSE;
int DrawScrollTest(HWND hwnd);
int GetScrollRect(HWND hwnd,LPRECT prc);
int GetScrollPos(HWND hwnd);

#define ID_GRIDHEADER_01 0x0001
#define ID_BUTTON_01 0x0002
#define ID_BUTTON_02 0x0003
#define ID_TEXT_01 0x0004
#define ID_TEXT_02 0x0005

int offset=0;
BOOL Dragging=FALSE;
int DragPt;
int DragIndex;

void InitialWindow(HWND hwnd);
LRESULT CALLBACK MainWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT ctrl_command(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK TreeGridHeaderProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
int GetHeadTotalOffset(HWND hwnd);

int Header_draw(HWND hwnd,HDC hdc,LPVOID param);

void regist_window(char* class_name,HINSTANCE instance,WNDPROC proc)
{
    WNDCLASSEX wndclass;

    wndclass.cbSize=sizeof(wndclass);
    wndclass.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;//红烧鱼，如你所愿
    wndclass.lpfnWndProc = proc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = instance;
    wndclass.hIcon = NULL;
    wndclass.hCursor = LoadCursor(NULL,IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0,0,0));
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = class_name;
    wndclass.hIconSm = NULL;

    RegisterClassEx(&wndclass);
}

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    char szClassName[]="Header_Scroll_Test";
    
    regist_window(szClassName,hInstance,MainWndProc);
        
    HWND hwnd = CreateWindowEx(0,
                               szClassName,
                               szClassName,
                               WS_THICKFRAME|WS_CAPTION|WS_OVERLAPPEDWINDOW|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN,
                               200,
                               50,
                               700,
                               600,
                               NULL,
                               NULL,
                               hInstance,
                               NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL,"create windows error !","error",MB_OK);
        return -1;
    }
    
    ShowWindow(hwnd,nCmdShow);
    UpdateWindow(hwnd);
                                           
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
        {
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
            InitialWindow(hwnd);
        }
        break;
    case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_COMMAND:
        {
            ctrl_command(hwnd,message,wParam,lParam);
        }
    break;
    case WM_SIZE:
        {
            int height=(int)HIWORD(lParam);
            int width=(int)LOWORD(lParam);
            HWND head=GetDlgItem(hwnd,ID_GRIDHEADER_01);
            HWND lbl=GetDlgItem(hwnd,ID_TEXT_01);
            HWND lbscroll=GetDlgItem(hwnd,ID_TEXT_02);
            RECT head_rect;
            int head_height=0;
            
            GetClientRect(head,&head_rect);
            head_height=head_rect.bottom-head_rect.top;
            MoveWindow(head,1,0,width-2*1,head_height,TRUE);
            MoveWindow(lbl,1,head_height+10,width-1*2,450,TRUE);
            MoveWindow(lbscroll,1,head_height+20+450,width-1*2,14,TRUE);
        }
    break;
	case WM_CTLCOLORSTATIC: { //Edit 在只读模式ES_READONLY下发送该消息
	    HDC hdc=(HDC)wParam;
	    HWND edit=(HWND)lParam;
	    
	    SetTextColor(hdc,RGB(0,128,255));
	    SetBkMode(hdc,TRANSPARENT);
	    SetBkColor(hdc,RGB(30,30,30));
	    return (HRESULT)CreateSolidBrush(RGB(50,50,50));
	} break;
    }
    return DefWindowProc(hwnd,message,wParam,lParam);
}

void InitialWindow(HWND hwnd)
{
    HFONT font=CreateFont(20,0,0,0,
                    FW_MEDIUM,//FW_SEMIBOLD,
                    FALSE,FALSE,FALSE,
                    DEFAULT_CHARSET,
                    OUT_OUTLINE_PRECIS,
                    CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, 
                    VARIABLE_PITCH,
                    "Courier New");
                    
    HWND header=CreateWindow(WC_HEADER,
                              NULL,
                              WS_CHILD|WS_VISIBLE|HDS_BUTTONS/*|HDS_HORZ*/,
                              50,50,1000,38,
                              hwnd,
                              (HMENU)ID_GRIDHEADER_01,
                              (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),
                              NULL);
    SetWindowLongPtr(header,GWLP_USERDATA,GetWindowLongPtr(header,GWLP_WNDPROC));
    SetWindowLongPtr(header,GWLP_WNDPROC,(LONG_PTR)TreeGridHeaderProc);
    SendMessage(header,WM_SETFONT,(WPARAM)font,0);
    {
        HDITEM hdi={0};
        char treegrid_title[][256]={\
        "Column 1",
        "Column 2",
        "Column 3",
        "Column 4",
        "Column 5",
        "Column 6",
        "Column 7",
        "Column 8",
        "Column 9",
        "Column 10"/*,
        "Column 11",
        "Column 12",
        "Column 13",
        "Column 14",
        "Column 15",
        "Column 16",
        "Column 17",
        "Column 18",
        "Column 19",
        "Column 20"*/};
        int scroll_width=0;        
        for(int index=0;index<sizeof(treegrid_title)/sizeof(char[256]);index++,scroll_width+=100) {
            hdi.mask=HDI_TEXT|HDI_FORMAT|HDI_WIDTH;
            hdi.cxy=150;
            hdi.pszText=treegrid_title[index];
            hdi.cchTextMax=sizeof(hdi.pszText);
            hdi.fmt=HDF_CENTER|HDF_STRING;
            
            SendMessage(header,HDM_INSERTITEM,(WPARAM)index,(LPARAM)&hdi);
        }
#define CX_STEP 10
        {
        //    SetScrollRange(header,SB_HORZ,0,scroll_width/CX_STEP,FALSE);
        //    SetScrollPos(header,SB_HORZ,30, TRUE);
        }
    }
    
    HWND bn_left=CreateWindow(WC_BUTTON,
                              "<<Left",
                              WS_CHILD|WS_VISIBLE,
                              50,250,150,25,
                              hwnd,
                              (HMENU)ID_BUTTON_01,
                              (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),
                              NULL);
    SendMessage(bn_left,WM_SETFONT,(WPARAM)font,0);
    HWND bn_right=CreateWindow(WC_BUTTON,
                              "Right>>",
                              WS_CHILD|WS_VISIBLE,
                              250,250,150,25,
                              hwnd,
                              (HMENU)ID_BUTTON_02,
                              (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),
                              NULL);
    SendMessage(bn_right,WM_SETFONT,(WPARAM)font,0);
    

    HWND lbl=CreateWindow(WC_STATIC,
                          "",
                          WS_CHILD|WS_VISIBLE,
                          0,80,500,150,
                          hwnd,
                          (HMENU)ID_TEXT_01,
                          (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),
                          NULL);
    SendMessage(lbl,WM_SETFONT,(WPARAM)font,0);
    
    char text[256]="";
    sprintf(text,"HHT_ABOVE:0x%08X",HHT_ABOVE);
    Text_Append(lbl,text);
    sprintf(text,"HHT_BELOW:0x%08X",HHT_BELOW);
    Text_Append(lbl,text);
    sprintf(text,"HHT_NOWHERE:0x%08X",HHT_NOWHERE);
    Text_Append(lbl,text);
    sprintf(text,"HHT_ONDIVIDER:0x%08X",HHT_ONDIVIDER);
    Text_Append(lbl,text);
    sprintf(text,"HHT_ONDIVOPEN:0x%08X",HHT_ONDIVOPEN);
    Text_Append(lbl,text);
    sprintf(text,"HHT_ONHEADER:0x%08X",HHT_ONHEADER);
    Text_Append(lbl,text);
    
    HWND lbl2=CreateWindow(WC_STATIC,
                          "",
                          WS_CHILD|WS_VISIBLE|SS_NOTIFY,
                          0,80,100,10,
                          hwnd,
                          (HMENU)ID_TEXT_02,
                          (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),
                          NULL);
    SendMessage(lbl2,WM_SETFONT,(WPARAM)font,0);
    SetWindowLongPtr(lbl2,GWLP_USERDATA,GetWindowLongPtr(lbl2,GWLP_WNDPROC));
    SetWindowLongPtr(lbl2,GWLP_WNDPROC,(LONG_PTR)HeadScrollTestProc);
}

int Text_Append(HWND hwnd,char* ptext)
{
    char text[1024]="";
    
    Static_GetText(hwnd,text,sizeof(text));
    strcat(text,ptext);
    strcat(text,"\r\n");
    Static_SetText(hwnd,text);
    
    return 0;
}

LRESULT CALLBACK HeadScrollTestProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    WNDPROC pre_scroll_proc=(WNDPROC)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    
    switch(message) {
    case WM_PAINT: {
        DrawScrollTest(hwnd);
        return 0;
    } break;
    case WM_LBUTTONDOWN: {
        POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
        RECT rc_thumb;
        if(GetScrollRect(hwnd,&rc_thumb)!=0) break;
            
        if(PtInRect(&rc_thumb,pt)) {
            StartDrag.x=pt.x;
            StartPos.x=(-offset);//GetScrollPos(hwnd);
            StartDragFlag=TRUE;
            SetCapture(hwnd);
        }
    } break;
    case WM_MOUSEMOVE: {
        POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
        if(StartDragFlag&&GetCapture()==hwnd) {
            HWND head=GetDlgItem(GetParent(hwnd),ID_GRIDHEADER_01);
            RECT rc,rc_thumb;
            
            GetClientRect(hwnd,&rc);
            GetScrollRect(head,&rc_thumb);
            //测算当前的Pos.
            SCROLLINFO si;
            si.nMin=0;
            si.nMax=GetHeadTotalOffset(head);
            si.nPage=rc.right-rc.left;
            si.nPos=StartPos.x+(pt.x-StartDrag.x)*1.0/((rc.right-rc.left)-(rc_thumb.right-rc_thumb.left))*(si.nMax-si.nMin+1-si.nPage);
            
            int scroll_pixls=si.nPos-abs(offset);
            offset=-si.nPos;
            if(offset>0) {
                scroll_pixls-=offset;
                offset=0;
            }
            else if(-offset>si.nMax-si.nMin+1-si.nPage) {
                scroll_pixls=-(offset-(si.nMax-si.nMin+1-si.nPage));
                offset=-(si.nMax-si.nMin+1-si.nPage);
            }
            
            InvalidateRect(head,NULL,TRUE);
            
            char text[256]="";
            sprintf(text,"Offset:%d",offset);
            SetWindowText(GetParent(hwnd),text);
        }
    } break;
    case WM_LBUTTONUP: {
        StartDragFlag=FALSE;
        ReleaseCapture();
    } break;
    }
    return CallWindowProc(pre_scroll_proc,hwnd,message,wParam,lParam);    
}

LRESULT CALLBACK TreeGridHeaderProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    WNDPROC pre_header_proc=(WNDPROC)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    
    switch(message) {
   	case HDM_LAYOUT:
   	    {
       		LRESULT result=CallWindowProc(pre_header_proc,hwnd, message, wParam, lParam);
    
       		LPHDLAYOUT lphdLayout=(LPHDLAYOUT)lParam;
       		lphdLayout->pwpos->cy=25;
       		lphdLayout->prc->top=lphdLayout->pwpos->cy;
       		
       		return result;
   	    } 
   	break;
   	case HDM_HITTEST: {
   	    int item_index=1;
   	    LPHDHITTESTINFO phit=(LPHDHITTESTINFO)lParam;
   	    
   	    int item_count=Header_GetItemCount(hwnd);
   	    int total_offset=0;
   	    for(int index=0;index<item_count;index++) {
   	        HDITEM hdi={0};
   	        
   	        hdi.mask=HDI_WIDTH;
   	        Header_GetItem(hwnd,index,&hdi);
   	        
   	        total_offset+=hdi.cxy;
   	        if(abs((total_offset+offset)-phit->pt.x)<5) {
   	            phit->flags=HHT_ONDIVIDER;
   	            phit->iItem=index;
   	            return index;
   	        }
   	        else if((total_offset+offset)-phit->pt.x>0) {
   	            phit->flags=HHT_ONHEADER;
   	            phit->iItem=index;
   	            return index;
   	        }
   	    }
   	    phit->flags=HHT_NOWHERE;
   	    phit->iItem=-1;
   	    return phit->iItem;
   	} break;
   	case WM_SETCURSOR: {
   	    UINT hit_pos=LOWORD(lParam);
        UINT trigger_msg=HIWORD(lParam);
        
        if(hit_pos==HTCLIENT&&trigger_msg==WM_MOUSEMOVE) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd,&pt);
            
   	        HDHITTESTINFO hit={0};
   	        memcpy(&hit.pt,&pt,sizeof(POINT));
   	        SendMessage(hwnd,HDM_HITTEST,0,(LPARAM)&hit);
            
            if(hit.flags==HHT_ONDIVIDER) SetCursor(LoadCursor(NULL,IDC_SIZEWE));
            else if(hit.flags==HHT_ONHEADER) SetCursor(LoadCursor(NULL,IDC_ARROW));
                
            return 0;
        }
   	} break;
   	case WM_MOUSEMOVE: {
   	    POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
   	    HDHITTESTINFO hit={0};
   	    
   	    if(GetCapture()==hwnd&&Dragging) {
   	        HDITEM hdi={0};
   	        
   	        hdi.mask=HDI_WIDTH;
   	        Header_GetItem(hwnd,DragIndex,&hdi);
   	        hdi.cxy+=(pt.x-DragPt);
   	        if(hdi.cxy<=0) hdi.cxy=5;
   	        Header_SetItem(hwnd,DragIndex,&hdi);
   	        DragPt=pt.x;
   	        InvalidateRect(hwnd,NULL,TRUE);
   	    }   	    
   	} break;
   	case WM_LBUTTONDOWN: {
   	    POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
   	    HDHITTESTINFO hit={0};
   	    
   	    memcpy(&hit.pt,&pt,sizeof(POINT));
   	    SendMessage(hwnd,HDM_HITTEST,0,(LPARAM)&hit);
            
        if(hit.flags==HHT_ONDIVIDER) {
            SetCapture(hwnd);
            Dragging=TRUE;
            DragPt=pt.x;
            DragIndex=hit.iItem;
            SetCursor(LoadCursor(NULL,IDC_SIZEWE));
            
            return 0;
        }
   	} break;
   	case WM_LBUTTONUP: {
   	    ReleaseCapture();
   	    Dragging=FALSE;
        SetCursor(LoadCursor(NULL,IDC_ARROW));
   	} break;
   	case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd,&ps);
            Header_draw(hwnd,ps.hdc,&ps);
            EndPaint(hwnd,&ps);
            return 0;
        }
    break;
    case WM_ERASEBKGND:
        {
            return FALSE;
        }
    break;
    case WM_SIZE:
    {
        int height = HIWORD(lParam);
	    int width = LOWORD(lParam);
        int item_count=Header_GetItemCount(hwnd);
        int total_offset=0;
        for(int index=0;index<item_count;index++) {
            HDITEM hdi={0};
            
            hdi.mask=HDI_WIDTH;
            Header_GetItem(hwnd,index,&hdi);
            
            total_offset+=hdi.cxy;
        }
        
        //offset维护了已经滚动的距离（被隐藏的部分），左移为负，右移为正。
        if(offset<0) {
            if(width>=total_offset) {
                ScrollWindow(hwnd,-offset,0,NULL,NULL);
                offset=0;
            }
            else if(abs(offset)+width>=total_offset&&width<total_offset) {
                int offset_tmp=width-total_offset;
                ScrollWindow(hwnd,offset_tmp-offset,0,NULL,NULL);
                offset=offset_tmp;
            }
        }
    } break;   
    }
    
    return CallWindowProc(pre_header_proc,hwnd,message,wParam,lParam);    
}

LRESULT ctrl_command(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    LRESULT result = 0;
    switch(LOWORD(wParam))
    {
        case ID_BUTTON_01://left
        {
            HWND ctrl_hwnd = (HWND)lParam;
            HWND header=GetDlgItem(hwnd,ID_GRIDHEADER_01);
            
            int item_count=Header_GetItemCount(header);
            int total_offset=0;
            for(int index=0;index<item_count;index++) {
                HDITEM hdi={0};
                
                hdi.mask=HDI_WIDTH;
                Header_GetItem(header,index,&hdi);
                
                total_offset+=hdi.cxy;
            }
            
            RECT rc;
            GetClientRect(header,&rc);
            
            if((total_offset-(rc.right-rc.left)+offset)<=0) break;
            
            offset-=20;
            ScrollWindow(header,-20,0,NULL,NULL);
            
            char text[256]="";
            sprintf(text,"Offset:%d",offset);
            SetWindowText(hwnd,text);
        }
        break;
        case ID_BUTTON_02://right
        {
            HWND ctrl_hwnd = (HWND)lParam;   
            HWND header=GetDlgItem(hwnd,ID_GRIDHEADER_01);
            if(offset>=0) break;
            offset+=20;
            ScrollWindow(header,20,0,NULL,NULL);
            
            char text[256]="";
            sprintf(text,"Offset:%d",offset);
            SetWindowText(hwnd,text);
        }
        break;        
    }
    return result;
}

int Header_draw(HWND hwnd,HDC hdc,LPVOID param)
{
    int column_count;
    HDITEM item={0};
    char colum_text_buffer[256]="";
    HFONT pre_font;
    HDC memdc;
    HBITMAP bmp,pre_bmp;
    RECT rect,bmp_rect,column_rect;
    COLORREF rgb_1=RGB(25,25,25),rgb_2=RGB(10,10,10);
    HPEN pen,pre_pen;
    HBRUSH brush;
    PAINTSTRUCT* ps=(PAINTSTRUCT*)param;
    int cx=ps->rcPaint.right-ps->rcPaint.left;//-offset;
    int cy=ps->rcPaint.bottom-ps->rcPaint.top;
    POINT pt_org;
    
    pen=CreatePen(PS_SOLID,1,RGB(100,100,100));
    GetClientRect(hwnd,&rect);
    memdc=CreateCompatibleDC(hdc);
    bmp=CreateCompatibleBitmap(hdc,cx,cy);
    pre_font=(HFONT)SelectObject(memdc,(HFONT)SendMessage(hwnd,WM_GETFONT,0,0));
    pre_bmp=(HBITMAP)SelectObject(memdc,bmp);
    
    OffsetViewportOrgEx(memdc,-(ps->rcPaint.left)+offset,-(ps->rcPaint.top),&pt_org);
    
    SetTextColor(memdc,RGB(0,128,250));
    SetBkMode(memdc,TRANSPARENT);
    CopyRect(&bmp_rect,&rect);
    OffsetRect(&bmp_rect,-offset,0);
    
    RECT rhd_1,rhd_2;
    CopyRect(&rhd_1,&bmp_rect);
    CopyRect(&rhd_2,&bmp_rect);
    rhd_1.bottom=(rhd_1.bottom+rhd_1.top)/2;
    rhd_2.top=rhd_1.bottom;
        
    GradientRect(memdc,rhd_1,rgb_1,rgb_2,1);
    GradientRect(memdc,rhd_2,rgb_2,rgb_1,1);
    
    column_count = Header_GetItemCount(hwnd);
    pre_pen=(HPEN)SelectObject(memdc,pen);
    brush=CreateSolidBrush(RGB(50,50,50));
    
    //并不是所有的item都需要绘制
    //查找需要绘制的item,以及item需要绘制的部分
    //当前的偏移是 -offset
    char text[256]="";
    Static_SetText(GetDlgItem(GetParent(hwnd),ID_TEXT_01),text,sizeof(text));
    
    int total_offset=0;
    for(int index = 0; index < column_count;index++) {
        Header_GetItemRect(hwnd,index,&column_rect);
        
        item.mask=HDI_TEXT|HDI_STATE|HDI_WIDTH;
        item.pszText=colum_text_buffer;
        item.cchTextMax=256;
        Header_GetItem(hwnd,index,&item);
        
        if(column_rect.right<=rect.left+(-offset)) continue;        
        else if(column_rect.left<=rect.left+(-offset)&&column_rect.right>rect.left+(-offset)) {
            //需要部分绘制
            total_offset=column_rect.right-(rect.left+(-offset));
        }
        else if(column_rect.left>=rect.left+(-offset)&&total_offset<cx) {
            //需要整段绘制
            total_offset+=item.cxy;
        }
        else if(total_offset>=cx) break;
        
        sprintf(text,"Item %d ",index+1);
        Text_Append(GetDlgItem(GetParent(hwnd),ID_TEXT_01),text);
        
        DrawText(memdc,colum_text_buffer,-1,&column_rect,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        
        //绘制立体分割线
        column_rect.left-=1;
        POINT pt[5]={{column_rect.right-1,0},
                     {column_rect.right-1,column_rect.bottom},
                     {column_rect.right,0},
                     {column_rect.right,column_rect.bottom}};
        
        COLORREF color_1=RGB(10,10,10),color_2=RGB(40,40,40);
        HPEN sp_line1=CreatePen(PS_SOLID,1,color_1);
        HPEN sp_line2=CreatePen(PS_SOLID,1,color_2);
        HPEN pre_sp;
        
        pre_sp=(HPEN)SelectObject(memdc,sp_line1);
        MoveToEx(memdc,pt[0].x,pt[0].y,&(pt[4]));
        LineTo(memdc,pt[1].x,pt[1].y);
        
        SelectObject(memdc,sp_line2);
        MoveToEx(memdc,pt[2].x,pt[2].y,&(pt[4]));
        LineTo(memdc,pt[3].x,pt[3].y);
        DeleteObject(sp_line1);
        DeleteObject(sp_line2); 
    }
    
    SetViewportOrgEx(memdc,pt_org.x,pt_org.y,NULL);
    
    BitBlt(hdc,ps->rcPaint.left,ps->rcPaint.top,cx,cy,
           memdc,0,0,SRCCOPY);
           
    DeleteObject(SelectObject(memdc,pre_bmp));
    DeleteObject(pen);
    DeleteObject(brush);
    DeleteDC(memdc);
    
    return 0;
}

void GradientRect(HDC hdc,RECT rc_tmp,COLORREF c1,COLORREF c2,int verical_or_horizen)
{
    RECT rc;
    CopyRect(&rc,&rc_tmp);
    
    Point pt[4]={Point(rc.left,rc.top),Point(rc.right,rc.top),Point(rc.right,rc.bottom),Point(rc.left,rc.bottom)};
    Rect rect(rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top);
    Graphics graphic(hdc);
    GraphicsPath path;
    LinearGradientBrush brush(rect,Color(255,GetRValue(c1),GetGValue(c1),GetBValue(c1)),
                                   Color(255,GetRValue(c2),GetGValue(c2),GetBValue(c2)),
                              verical_or_horizen?LinearGradientModeVertical:LinearGradientModeHorizontal);
    
    graphic.SetSmoothingMode(SmoothingModeHighQuality);
    path.AddLines(pt,4);
    graphic.FillPath(&brush,&path);
}

int GetHeadTotalOffset(HWND hwnd)
{
    int item_count=Header_GetItemCount(hwnd);
    int total_offset=0;
    for(int index=0;index<item_count;index++) {
        HDITEM hdi={0};
        
        hdi.mask=HDI_WIDTH;
        Header_GetItem(hwnd,index,&hdi);
        
        total_offset+=hdi.cxy;
    }
    return total_offset;
}

int GetScrollPos(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd,&rc);
    
    HWND head=GetDlgItem(GetParent(hwnd),ID_GRIDHEADER_01);
    
    SCROLLINFO si;
    si.nMin=0;
    si.nMax=GetHeadTotalOffset(head);
    si.nPage=rc.right-rc.left;
    si.nPos=-offset;
    
    if(rc.right-rc.left>si.nMax) return -1;
    
    int thumb_pixls=(rc.right-rc.left)*1.0*si.nPage/(si.nMax-si.nMin+1);
    if(thumb_pixls<50) thumb_pixls=50;
    int thumb_pos=(rc.right-rc.left-thumb_pixls)*1.0*si.nPos/(si.nMax-si.nMin+1-si.nPage);
    
    return thumb_pos;
}

int GetScrollRect(HWND hwnd,LPRECT prc)
{
    RECT rc;
    GetClientRect(hwnd,&rc);
    SCROLLINFO si;
    HWND head=GetDlgItem(GetParent(hwnd),ID_GRIDHEADER_01);
    
    si.nMin=0;
    si.nMax=GetHeadTotalOffset(head);
    si.nPage=rc.right-rc.left;
    si.nPos=-offset;
    
    if(rc.right-rc.left>si.nMax) return -1;
    
    int thumb_pixls=(rc.right-rc.left)*1.0*si.nPage/(si.nMax-si.nMin+1);
    if(thumb_pixls<50) thumb_pixls=50;
    int thumb_pos=(rc.right-rc.left-thumb_pixls)*1.0*si.nPos/(si.nMax-si.nMin+1-si.nPage);
    
    RECT rc_thumb;
    rc_thumb.left=rc.left+thumb_pos;
    rc_thumb.right=rc_thumb.left+thumb_pixls;
    rc_thumb.top=rc.top;
    rc_thumb.bottom=rc.bottom;
    //if(rc_thumb.bottom-rc_thumb.top>13) rc_thumb.bottom=rc_thumb.top+13;
        
    CopyRect(prc,&rc_thumb);
    
    return 0;
}

int DrawScrollTest(HWND hwnd)
{
    HWND head=GetDlgItem(GetParent(hwnd),ID_GRIDHEADER_01);
    int total_offset=GetHeadTotalOffset(head);
        
    RECT rc_thumb;
    GetScrollRect(hwnd,&rc_thumb);
    
    RECT rc,rc_mem;
    GetClientRect(hwnd,&rc);
    
    HDC hdc=GetDC(hwnd);
    HDC memdc=CreateCompatibleDC(hdc);
    HBITMAP bmp=CreateCompatibleBitmap(hdc,rc.right-rc.left,rc.bottom-rc.top);
    HBITMAP pre_bmp=(HBITMAP)SelectObject(memdc,bmp);
    HBRUSH brush_bk=CreateSolidBrush(RGB(50,50,50));
    
    CopyRect(&rc_mem,&rc);
    OffsetRect(&rc_mem,-rc_mem.left,-rc_mem.top);
    FillRect(memdc,&rc_mem,brush_bk);
    
    OffsetRect(&rc_thumb,-rc.left,-rc.top);
    if(rc.right-rc.left<total_offset) {
    Graphics graphic(memdc);
    GraphicsPath path;
    LinearGradientBrush pbrush(Rect(rc_thumb.left,rc_thumb.top,rc_thumb.right-rc_thumb.left,rc_thumb.bottom-rc_thumb.top),
                               Color(255,5,5,5),
                               Color(255,45,45,45),
                               LinearGradientModeVertical);
    
    graphic.SetSmoothingMode(SmoothingModeHighQuality);
    path.AddArc(rc_thumb.left,rc_thumb.top,(rc_thumb.bottom-rc_thumb.top),(rc_thumb.bottom-rc_thumb.top),90,180);
    path.AddArc(rc_thumb.right-(rc_thumb.bottom-rc_thumb.top),rc_thumb.top,(rc_thumb.bottom-rc_thumb.top),(rc_thumb.bottom-rc_thumb.top),-90,180);
    graphic.FillPath(&pbrush,&path);
    }
    
    BitBlt(hdc,rc.left,rc.top,rc.right-rc.top,rc.bottom-rc.top,
           memdc,0,0,SRCCOPY);
    
    DeleteObject(brush_bk);
    DeleteObject(SelectObject(memdc,pre_bmp));
    DeleteDC(memdc);
    ReleaseDC(hwnd,hdc);
    
    return 0;
}
