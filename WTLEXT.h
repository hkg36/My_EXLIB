#pragma once
#include <atlbase.h>
#include <atlstr.h>
#include <atlcrack.h>
#include <atltime.h>
#include <atlframe.h>
#include <deque>
#include <map>

class CPropSheet:public CWindowImpl<CPropSheet>
{
public:
	const static DWORD treeCtrlID=1002;
	BEGIN_MSG_MAP_EX(CPropSheet)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_SIZE(OnSize)
		NOTIFY_HANDLER_EX(treeCtrlID,TVN_SELCHANGED,OnTreeCtrlSelect)
		FORWARD_NOTIFICATIONS();
	END_MSG_MAP()

	CTreeViewCtrl tree;
	CStatic topstatic;
	std::map<HTREEITEM,HWND> propsheetlist;
	CRect pagepos;
	HWND nowpane;
	DWORD TreeWidth,StaticHeight;
	CPropSheet(DWORD LTreeWidth=200,DWORD TitleHeight=15);
	LRESULT OnCreate (LPCREATESTRUCT cs);
	void SetTreeImageList(HIMAGELIST list);
	HTREEITEM InsertPane(LPCTSTR name,HWND pagewnd,HTREEITEM hParent=TVI_ROOT, HTREEITEM hInsertAfter=TVI_LAST);
	HTREEITEM InsertPane(LPCTSTR name,HWND pagewnd,int nImage,int nSelectedImage,
		HTREEITEM hParent=TVI_ROOT, HTREEITEM hInsertAfter=TVI_LAST);
	void SelectPane(HWND pagewnd);
	void OnSize(UINT nType, CSize size);
	LRESULT OnTreeCtrlSelect(LPNMHDR pnmh);
};

class CDebugWindow:public CWindowImpl<CDebugWindow>
{
public:
	struct DebugCommandInterface
	{
		virtual void Command(CStringW cmdline)=0;
	};
	DECLARE_FRAME_WND_CLASS_EX(nullptr, 0,CS_DBLCLKS,0)
	static void Init();
	static void Disponse();
	static void Show(bool show=true);
	static void SetCommandCallback(DebugCommandInterface *cmdcallback)
	{
		dbgwnd->cmdinterface=cmdcallback;
	}
	CDebugWindow():cmdinterface(nullptr){}
	static void WriteLog(CStringW from,LPCWSTR message,...);
protected:
	static CDebugWindow* dbgwnd;
	DebugCommandInterface *cmdinterface;
	enum{MaxLogLine=500,LogList=2,EnterBn};
	enum{MSG_ENTERCOMMAND=WM_USER+1,MSG_ENTERCOMMANDUPDOWN};
	BEGIN_MSG_MAP_EX(CDebugWindow)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_SIZE(OnSize)
		MSG_WM_CLOSE(OnClose)
		NOTIFY_HANDLER(LogList,LVN_GETDISPINFO,OnListGetDispInfo)
		NOTIFY_HANDLER(LogList,LVN_ITEMCHANGED,OnListItemChanged)
		COMMAND_ID_HANDLER(EnterBn,OnEnterBn)
		MESSAGE_HANDLER(MSG_ENTERCOMMAND,OnEnterCommand)
		MESSAGE_HANDLER(MSG_ENTERCOMMANDUPDOWN,OnEnterCommandUpDown)
	END_MSG_MAP()

	class CEditEx:public CWindowImpl<CEditEx,CEdit>
	{
	public:
		DECLARE_WND_SUPERCLASS(nullptr, CEdit::GetWndClassName())

		BEGIN_MSG_MAP_EX(CEditEx)
			MSG_WM_CHAR(OnChar)
			MSG_WM_KEYDOWN(OnKeyDown)
		END_MSG_MAP()

		void OnChar(UINT nChar,UINT nRepCnt,UINT nFlags)
		{
			if(nChar=='\r')
			{
				GetParent().SendMessage(MSG_ENTERCOMMAND,0,0);
			}
			else SetMsgHandled(FALSE);
		}
		void OnKeyDown(UINT nChar,UINT nRepCnt,UINT nFlags )
		{
			switch(nChar)
			{
			case VK_UP:
				{
					GetParent().PostMessage(MSG_ENTERCOMMANDUPDOWN,1,0);
				}break;
			case VK_DOWN:
				{
					GetParent().PostMessage(MSG_ENTERCOMMANDUPDOWN,0,0);
				}break;
			}
		}
	};
	CListViewCtrl list;
	CEditEx input;
	CEdit detail;
	CButton enter;
	struct LOGSTRUCT
	{
		CTime time;
		CStringW from;
		CStringW messge;
	};
	std::deque<LOGSTRUCT> log;
	std::deque<CStringW> inputlog;
	int nowlogindex;
	void OnClose();
	LRESULT OnCreate (LPCREATESTRUCT cs);
	void OnDestroy();
	void OnSize(UINT nType, CSize size);
	LRESULT OnListGetDispInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnListItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void ReportData(NMLVDISPINFO *pdi);
	LRESULT OnEnterBn(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnterCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEnterCommandUpDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void AddLogLine(CStringW from,LPCWSTR message,...);
	void AddLogLineV(CStringW from,LPCWSTR message,va_list argList);
	void OnEnterCommand();
};