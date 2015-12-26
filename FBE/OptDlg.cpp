#include "stdafx.h"
#include "resource.h"

#include "utils.h"
#include "apputils.h"

#include <atlgdi.h>

#if _WIN32_WINNT>=0x0501
#include <atltheme.h>
#endif

// color picker
#include <ColorButton.h>

#include "OptDlg.h"

class COptDlg: public CDialogImpl<COptDlg>
{
public:
  enum { IDD=IDD_OPTIONS };

  COptDlg() { }

  CColorButton	    m_fg,m_bg;
  CComboBox	    m_fonts;
  CComboBox	    m_fontsize;

  CString	    m_face;
  int		    m_fsz_val;
  
  BEGIN_MSG_MAP(COptDlg)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    REFLECT_NOTIFICATIONS()
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);

  LRESULT OnOK(WORD, WORD wID, HWND, BOOL&);
  LRESULT OnCancel(WORD, WORD wID, HWND, BOOL&);

};
static int __stdcall EnumFontProc(const ENUMLOGFONTEX *lfe,
				 const NEWTEXTMETRICEX *ntm,
				 DWORD type,
				 LPARAM data)
{
  CComboBox	  *cb=(CComboBox*)data;
  cb->AddString(lfe->elfLogFont.lfFaceName);
  return TRUE;
}

static int  font_sizes[]={8,9,10,11,12,13,14,15,16,18,20,22,24,26,28,36,48,72};

LRESULT COptDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
  m_fg.SubclassWindow(GetDlgItem(IDC_FG));
  m_bg.SubclassWindow(GetDlgItem(IDC_BG));
  m_fonts=GetDlgItem(IDC_FONT);
  m_fontsize=GetDlgItem(IDC_FONT_SIZE);

  // init color controls
  m_bg.SetDefaultColor(::GetSysColor(COLOR_WINDOW));
  m_fg.SetDefaultColor(::GetSysColor(COLOR_WINDOWTEXT));
  m_bg.SetColor(U::GetSettingI(_T("ColorBG"),CLR_DEFAULT));
  m_fg.SetColor(U::GetSettingI(_T("ColorFG"),CLR_DEFAULT));

  // get font list
  HDC	hDC=::CreateDC(_T("DISPLAY"),NULL,NULL,NULL);
  LOGFONT lf;
  memset(&lf,0,sizeof(lf));
  lf.lfCharSet=ANSI_CHARSET;
  ::EnumFontFamiliesEx(hDC,&lf,(FONTENUMPROC)&EnumFontProc,(LPARAM)&m_fonts,0);
  ::DeleteDC(hDC);

  // get text
  CString     fnt(U::GetSettingS(_T("Font"),_T("Trebuchet MS")));
  int	      idx=m_fonts.FindStringExact(0,fnt);
  if (idx<0)
    idx=0;
  m_fonts.SetCurSel(idx);

  // init zoom
  int	      m_fsz_val=U::GetSettingI(_T("FontSize"),12);
  CString     szstr;
  szstr.Format(_T("%d"),m_fsz_val);
  m_fontsize.SetWindowText(szstr);
  for (int i=0;i<sizeof(font_sizes)/sizeof(font_sizes[0]);++i) {
    szstr.Format(_T("%d"),font_sizes[i]);
    m_fontsize.AddString(szstr);
  }
  return 0;
}

LRESULT COptDlg::OnOK(WORD, WORD wID, HWND, BOOL&)
{
  // fetch zoom
  CString   szstr(U::GetWindowText(m_fontsize));
  if (_stscanf(szstr,_T("%d"),&m_fsz_val)!=1 ||
    m_fsz_val<6 || m_fsz_val>72)
  {
    MessageBeep(MB_ICONERROR);
    m_fontsize.SetFocus();
    return 0;
  }

  // save colors to registry
  _Settings.SetDWORDValue(_T("ColorBG"),m_bg.GetColor());
  _Settings.SetDWORDValue(_T("ColorFG"),m_fg.GetColor());

  // save font face
  m_face=U::GetWindowText(m_fonts);
  _Settings.SetStringValue(_T("Font"),m_face);
  // save zoom
  _Settings.SetDWORDValue(_T("FontSize"),m_fsz_val);

  EndDialog(wID);
  return 0;
}

LRESULT COptDlg::OnCancel(WORD, WORD wID, HWND, BOOL&){
  EndDialog(wID);
  return 0;
}

bool  ShowOptionsDialog() {
  COptDlg   dlg;
  return dlg.DoModal()==IDOK;
}

//////////////////////////////////////////////////////////////
// Tools:Options
class CToolsOptDlg: public CDialogImpl<CToolsOptDlg>
{
public:
  enum { IDD=IDD_TOOLS_OPTIONS };

  CToolsOptDlg() { }

  CButton	    m_src_wrap;
  CButton	    m_src_hl;
  CButton	    m_src_eol;
  
  BEGIN_MSG_MAP(COptDlg)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    REFLECT_NOTIFICATIONS()
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);

  LRESULT OnOK(WORD, WORD wID, HWND, BOOL&);
  LRESULT OnCancel(WORD, WORD wID, HWND, BOOL&);
};

LRESULT CToolsOptDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
  m_src_wrap=GetDlgItem(IDC_WRAP);
  m_src_hl=GetDlgItem(IDC_SYNTAXHL);
  m_src_eol=GetDlgItem(IDC_SHOWEOL);

  // init controls
  m_src_wrap.SetCheck(U::GetSettingI(_T("XMLSrcWrap"),TRUE));
  m_src_hl.SetCheck(U::GetSettingI(_T("XMLSrcSyntaxHL"),TRUE));
  m_src_eol.SetCheck(U::GetSettingI(_T("XMLSrcShowEOL"),FALSE));

  return 0;
}

LRESULT CToolsOptDlg::OnOK(WORD, WORD wID, HWND, BOOL&)
{
  // save settings to registry
  _Settings.SetDWORDValue(_T("XMLSrcWrap"),m_src_wrap.GetCheck());
  _Settings.SetDWORDValue(_T("XMLSrcSyntaxHL"),m_src_hl.GetCheck());
  _Settings.SetDWORDValue(_T("XMLSrcShowEOL"),m_src_eol.GetCheck());

  EndDialog(wID);
  return 0;
}

LRESULT CToolsOptDlg::OnCancel(WORD, WORD wID, HWND, BOOL&){
  EndDialog(wID);
  return 0;
}

bool  ShowToolsOptionsDialog() {
  CToolsOptDlg   dlg;
  return dlg.DoModal()==IDOK;
}
