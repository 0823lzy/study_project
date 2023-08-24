// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include"RemoteClientDlg.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


BOOL CWatchDialog::OnInitDialog()//初始化函数
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();//得到父类对象
		if (pParent->isFull()) {
			CRect rect;
			m_picture.GetWindowRect(rect);//获取目标的大小，输入到rect里
			//BitBlt是一个Windows API函数，
			// 用于在设备上执行位块传输（Bit-block Transfer，简称BitBlt）
			// 它允许在不同的设备上进行图像数据的传输、复制、拉伸、反转等操作
			//得到图像，第一个参数目标设备的句柄,后两个为左上角xy坐标,最后一个参数为操作的类型
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
			
			//缩放到目标图像里
			pParent->GetImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0,
				rect.Width(),
				rect.Height(),
				SRCCOPY);
			m_picture.InvalidateRect(NULL);//通知进行重绘
			pParent->GetImage().Destroy();//销毁
			pParent->SetImageStatus();//初始化缓存数据为false

		}
	}
	CDialog::OnTimer(nIDEvent);
}



