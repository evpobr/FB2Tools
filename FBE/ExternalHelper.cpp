#include "stdafx.h"
#include "resource.h"

#include "utils.h"
#include "apputils.h"

#include "FBE.h"
#include "ExternalHelper.h"

#define	MENU_BASE   5000

struct Genre {
  int	      groupid;
  CString     id;
  CString     text;
};

static CSimpleArray<CString>	g_genre_groups;
static CSimpleArray<Genre>	g_genres;

// genre list helper
static void	    LoadGenres() {
  FILE	  *fp=_tfopen(U::GetProgDirFile(_T("genres.txt")),_T("rb"));

  g_genre_groups.RemoveAll();
  g_genres.RemoveAll();

  char	  buffer[1024];
  while (fgets(buffer,sizeof(buffer),fp)) {
    int	  l=strlen(buffer);
    if (l>0 && buffer[l-1]=='\n')
      buffer[--l]='\0';
    if (l>0 && buffer[l-1]=='\r')
      buffer[--l]='\0';

    if (buffer[0] && buffer[0]!=' ') {
      CString name(buffer);
      name.Replace(_T("&"),_T("&&"));
      g_genre_groups.Add(name);
    } else {
      char  *p=strchr(buffer+1,' ');
      if (!p || p==buffer+1)
	continue;
      *p++='\0';
      Genre   g;
      g.groupid=g_genre_groups.GetSize()-1;
      g.id=buffer+1;
      g.text=p;
      g.text.Replace(_T("&"),_T("&&"));
      g_genres.Add(g);
    }
  }
  fclose(fp);
}

static CMenu	  MakeGenresMenu() {
  CMenu	  ret;
  ret.CreatePopupMenu();

  CMenu	  cur;
  int	  g=-1;
  for (int i=0;i<g_genres.GetSize();++i) {
    if (g_genres[i].groupid!=g) {
      g=g_genres[i].groupid;
      cur.Detach();
      cur.CreatePopupMenu();
      ret.AppendMenu(MF_POPUP|MF_STRING,(UINT)(HMENU)cur,g_genre_groups[g]);
    }
    cur.AppendMenu(MF_STRING,i+MENU_BASE,g_genres[i].text);
  }
  cur.Detach();

  return ret.Detach();
}

HRESULT	ExternalHelper::GenrePopup(IDispatch *obj,LONG x,LONG y,BSTR *name) {
  LoadGenres();
  CMenu	  popup=MakeGenresMenu();
  if (popup) {
    UINT  cmd=popup.TrackPopupMenu(
      TPM_RETURNCMD|TPM_LEFTALIGN|TPM_TOPALIGN,
      x,y,::GetActiveWindow()
    );
    popup.DestroyMenu();
    cmd-=MENU_BASE;
    if (cmd<(UINT)g_genres.GetSize()) {
      *name=g_genres[cmd].id.AllocSysString();
      return S_OK;
    }
  }
  *name=NULL;
  return S_OK;
}