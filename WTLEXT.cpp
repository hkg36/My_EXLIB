#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>
#include <atlcoll.h>
#include <atlctrls.h>
#include <atlmisc.h>
#include <atlcrack.h>

#include "WTLEXT.h"

CPropSheet::CPropSheet(DWORD LTreeWidth,DWORD TitleHeight):TreeWidth(LTreeWidth),StaticHeight(TitleHeight),
	nowpane(nullptr)
{
}

LRESULT CPropSheet::OnCreate (LPCREATESTRUCT cs)
{
	tree.Create(*this,rcDefault,nullptr, 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |WS_BORDER |
		TVS_SHOWSELALWAYS|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT,
		0,
		treeCtrlID);
	topstatic.Create(*this,rcDefault,nullptr, WS_CHILD | WS_VISIBLE);
	topstatic.SetFont(AtlGetDefaultGuiFont());

	return S_OK;
}
void CPropSheet::SetTreeImageList(HIMAGELIST list)
{
	tree.SetImageList(list);
}
HTREEITEM CPropSheet::InsertPane(LPCTSTR name,HWND pagewnd,HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	HTREEITEM treeitem=tree.InsertItem(name,hParent,hInsertAfter);
	if(pagewnd!=nullptr)
		propsheetlist[treeitem]=pagewnd;
	return treeitem;
}
HTREEITEM CPropSheet::InsertPane(LPCTSTR name,HWND pagewnd,int nImage,int nSelectedImage,
	HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	HTREEITEM treeitem=tree.InsertItem(name,nImage,nSelectedImage,hParent,hInsertAfter);
	if(pagewnd!=nullptr)
	{
		::ShowWindow(pagewnd,SW_HIDE);
		propsheetlist[treeitem]=pagewnd;
	}
	return treeitem;
}
void CPropSheet::SelectPane(HWND pagewnd)
{
	HTREEITEM treeitem=nullptr;
	for(auto i=propsheetlist.begin();i!=propsheetlist.end();i++)
	{
		if(pagewnd==i->second)
		{
			treeitem=i->first;
			break;
		}
	}
	if(treeitem)tree.SelectItem(treeitem);
}
void CPropSheet::OnSize(UINT nType, CSize size)
{
	tree.MoveWindow(0,0,TreeWidth,size.cy);
	topstatic.MoveWindow(TreeWidth+1,0,size.cx-TreeWidth-1,StaticHeight);
	pagepos.SetRect(TreeWidth+1,StaticHeight,size.cx,size.cy);

	for(auto i=propsheetlist.begin();i!=propsheetlist.end();i++)
	{
		::MoveWindow(i->second,pagepos.left,pagepos.top,pagepos.Width(),pagepos.Height(),TRUE);
	}
}
LRESULT CPropSheet::OnTreeCtrlSelect(LPNMHDR pnmh)
{
	CStringW str;
	HTREEITEM treeitem=tree.GetSelectedItem();
	tree.GetItemText(treeitem,str);

	auto i=propsheetlist.find(treeitem);
	if(i!=propsheetlist.end())
	{
		topstatic.SetWindowText(str);
		if(::IsWindow(nowpane))
		{
			::ShowWindow(nowpane,SW_HIDE);
		}
		::SetWindowPos(i->second,nullptr,pagepos.left,pagepos.top,pagepos.Width(),pagepos.Height(),
			SWP_NOZORDER|SWP_SHOWWINDOW);
		nowpane=i->second;
	}
	return S_OK;
}

CDebugWindow* CDebugWindow::dbgwnd=nullptr;

void CDebugWindow::Init()
{
	if(dbgwnd) return;
	dbgwnd=new CDebugWindow();
	dbgwnd->Create(nullptr,CRect(0,0,200,200),L"DebugWin",
		WS_BORDER|WS_CAPTION|WS_MINIMIZEBOX|WS_SIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU|WS_CLIPCHILDREN|WS_BORDER);
}
void CDebugWindow::Disponse()
{
	if(dbgwnd==nullptr) return;
	if(dbgwnd->IsWindow())
		dbgwnd->DestroyWindow();
	delete dbgwnd;
	dbgwnd=nullptr;
}
void CDebugWindow::Show(bool show)
{
	if(dbgwnd)
	{
		if(dbgwnd->IsWindow())
		{
			dbgwnd->ShowWindow(show?SW_SHOW:SW_HIDE);
		}
	}
}
void CDebugWindow::WriteLog(CStringW from,LPCWSTR message,...)
{
	if(dbgwnd)
	{
		ATLASSERT( AtlIsValidString( message ) );
		va_list argList;
		va_start( argList, message );
		dbgwnd->AddLogLineV(from,message,argList);
		va_end( argList );
	}
}
void CDebugWindow::OnClose()
{
	ShowWindow(SW_HIDE);
}
LRESULT CDebugWindow::OnCreate (LPCREATESTRUCT cs)
{
	CStringW temp;

	list.Create(*this, rcDefault, nullptr,
		WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
		LVS_REPORT|LVS_SHOWSELALWAYS|LVS_OWNERDATA|LVS_SINGLESEL,
		WS_EX_CLIENTEDGE,(UINT)LogList);
	list.ShowWindow(SW_SHOW);
	list.SetFont(AtlGetDefaultGuiFont());

	list.InsertColumn(0,L"time",0,150);
	list.InsertColumn(1,L"from",0,150);
	list.InsertColumn(2,L"message",0,450);

	list.SetItemCount(MaxLogLine);
	list.EnsureVisible(0,FALSE);
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	list.SetFont(AtlGetDefaultGuiFont());

	CEdit tmpedit;
	tmpedit.Create(*this,rcDefault,0,WS_CHILD,WS_EX_CLIENTEDGE);
	input.SubclassWindow(tmpedit.Detach());
	input.ShowWindow(SW_SHOW);
	input.SetFont(AtlGetDefaultGuiFont());

	enter.Create(*this,0,L"enter",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,EnterBn);
	enter.SetFont(AtlGetDefaultGuiFont());

	detail.Create(*this,rcDefault,0,WS_CHILD|ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL,WS_EX_CLIENTEDGE);
	detail.ShowWindow(SW_SHOW);
	detail.SetFont(AtlGetDefaultGuiFont());

	return S_OK;
}
void CDebugWindow::OnDestroy()
{
}
void CDebugWindow::OnSize(UINT nType, CSize size)
{
	enum{VD1=20,VD2=80,HD1=80};
	CRect rc;
	rc.bottom=size.cy-VD2;
	rc.top=size.cy-VD1-VD2;
	rc.left=0;
	rc.right=size.cx-HD1;
	input.MoveWindow(&rc);

	rc.bottom=size.cy-VD2;
	rc.top=size.cy-VD1-VD2;
	rc.left=size.cx-HD1;
	rc.right=size.cx;
	enter.MoveWindow(&rc);

	rc.left=0;
	rc.right=size.cx;
	rc.bottom=size.cy-VD1-VD2;
	rc.top=0;
	list.MoveWindow(&rc);

	rc.bottom=size.cy;
	rc.top=size.cy-VD2;
	rc.left=0;
	rc.right=size.cx;
	detail.MoveWindow(&rc);
}
LRESULT CDebugWindow::OnListGetDispInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	if(pnmh->hwndFrom==list)
	{
		ReportData((NMLVDISPINFO *)pnmh);
	}
	return S_OK;
}
LRESULT CDebugWindow::OnListItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	if(pnmh->hwndFrom==list)
	{
		int selected=list.GetSelectedIndex();
		if(selected<0) return S_OK;
		if(selected<(int)log.size())
		{
			auto& onemsg=log[selected];
			CAtlStringW temp;
			temp.Format(L"时间:%s\r\n来自:%s\r\n消息:%s",onemsg.time.Format(L"%H:%M:%S"),onemsg.from,onemsg.messge);
			detail.SetWindowText(temp);
		}
	}
	return S_OK;
}
void CDebugWindow::ReportData(NMLVDISPINFO *pdi)
{
	if((pdi->item.mask&LVIF_TEXT)!=LVIF_TEXT) return;
	pdi->item.mask=0;
	if(pdi->item.iItem<(int)log.size())
	{
		LOGSTRUCT &logstruct=log[pdi->item.iItem];
		if(pdi->item.iSubItem==0)
		{
			pdi->item.mask=LVIF_TEXT;
			struct tm totm;
			logstruct.time.GetLocalTm(&totm);
			wcsftime(pdi->item.pszText,pdi->item.cchTextMax,
				L"%H:%M:%S",&totm);
		}
		else if(pdi->item.iSubItem==1)
		{
			pdi->item.mask=LVIF_TEXT;
			wcscpy_s(pdi->item.pszText,pdi->item.cchTextMax,logstruct.from);
		}
		else if(pdi->item.iSubItem==2)
		{
			pdi->item.mask=LVIF_TEXT;
			wcscpy_s(pdi->item.pszText,pdi->item.cchTextMax,logstruct.messge);
		}
	}
}
LRESULT CDebugWindow::OnEnterBn(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	OnEnterCommand();
	return S_OK;
}
LRESULT CDebugWindow::OnEnterCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	OnEnterCommand();
	return S_OK;
}
void CDebugWindow::AddLogLine(CStringW from,LPCWSTR message,...)
{

	ATLASSERT( AtlIsValidString( message ) );
	va_list argList;
	va_start( argList, message );
	AddLogLineV(from,message,argList);
	va_end( argList );
}
void CDebugWindow::AddLogLineV(CStringW from,LPCWSTR message,va_list argList)
{
	CStringW msg;
	msg.FormatV( message, argList );

	LOGSTRUCT logmsg;
	logmsg.time=CTime::GetCurrentTime();
	logmsg.from=from;
	logmsg.messge=msg;

	if(log.size()==MaxLogLine)
	{
		log.pop_front();
	}
	log.push_back(logmsg);
	if(list.IsWindow()) list.Invalidate();
}
void CDebugWindow::OnEnterCommand()
{
	CStringW cmd;
	input.GetWindowText(cmd);
	cmd.Trim();
	if(cmd.IsEmpty()==FALSE)
	{
		for(auto i=inputlog.begin();i!=inputlog.end();i++)
		{
			if(i->Compare(cmd)==0)
			{
				inputlog.erase(i);
				break;
			}
		}
		inputlog.push_front(cmd);
		if(inputlog.size()>10)
			inputlog.pop_back();
		nowlogindex=0;

		input.SetWindowText(L"");
		AddLogLine(L"EnterCmd",L"%s",cmd);
		if(cmdinterface)
			cmdinterface->Command(cmd);
	}
}
LRESULT CDebugWindow::OnEnterCommandUpDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(inputlog.empty()) return S_OK;
	switch(wParam)
	{
	case 0:
		{
			if(nowlogindex==0)
				nowlogindex=inputlog.size()-1;
			else
				nowlogindex-=1;
		}break;
	case 1:
		{
			nowlogindex=(nowlogindex+1)%inputlog.size();
		}break;
	}
	input.SetWindowText(inputlog[nowlogindex]);
	return S_OK;
}