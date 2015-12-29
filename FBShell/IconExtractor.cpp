#include "stdafx.h"
#include "IconExtractor.h"

#include "FBShell.h"
#ifdef FBSHELL_USE_EXTERNAL_LIBS
#include "Image.h"
#endif

#ifdef FBSHELL_USE_EXTERNAL_LIBS

// a MemReader helper class
class MemReader : public ImageLoader::BinReader {
private:
  const BYTE  *m_data;
  int	      m_len;
public:
  MemReader(void *mem,int len) { Init(mem,len); }
  int Read(void *buffer,int count) {
    if (count>m_len)
      count=m_len;
    memcpy(buffer,m_data,count);
    m_data+=count;
    m_len-=count;
    return count;
  }
  void	Init(void *mem,int len) { m_data=(const BYTE *)mem; m_len=len; }
};

#endif

// IExtractImage
HRESULT CIconExtractor::GetLocation(wchar_t *file,DWORD filelen,DWORD *prio,
				   const SIZE *sz,
				   DWORD depth,DWORD *flags)
{
  m_desired_size=*sz;
  m_desired_depth=depth;


  *flags|=IEIFLAG_CACHE;

  lstrcpynW(file,m_filename,filelen);

  if (*flags & IEIFLAG_ASYNC) {
    *prio = 1;
    return E_PENDING;
  }

  return S_OK;
}

HRESULT CIconExtractor::Extract(HBITMAP *hBmp) {
  // load image if available
  CString     type;
  int	      datalen;
  void	      *data=NULL;
  if (!LoadObject(m_filename,type,data,datalen))
    return E_FAIL;

#ifdef FBSHELL_USE_GDIPLUS
  CComPtr<IStream> spImageStream;
  HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &spImageStream);
  if (SUCCEEDED(hr))
  {
	  ULONG cbRead;
	  hr = spImageStream->Write(data, datalen, &cbRead);
	  if (SUCCEEDED(hr))
	  {
		  Gdiplus::Bitmap image(spImageStream);
		  if (image.GetLastStatus() == Gdiplus::Ok)
		  {
			  float imageWidth = (float)image.GetWidth();
			  float imageHeight = (float)image.GetHeight();
			  float scale = 0.0f;

			  if (imageWidth <= imageHeight)
				  scale = (float)m_desired_size.cy / (float)image.GetHeight();
			  else
				  scale = (float)m_desired_size.cx / (float)image.GetWidth();

			  float thumbWidth = (float)imageWidth * scale;
			  float thumbHeight = (float)imageHeight * scale;

			  Gdiplus::Bitmap thumb((int)thumbWidth, (int)thumbHeight);
			  if (thumb.GetLastStatus() == Gdiplus::Ok)
			  {
				  Gdiplus::Graphics graphics(&thumb);
				  if (graphics.GetLastStatus() == Gdiplus::Ok)
				  {
					  Gdiplus::Status status = graphics.DrawImage(&image, 0, 0, (int)thumbWidth, (int)thumbHeight);

					  if (status == Gdiplus::Ok)
					  {
						  HBITMAP *hbmReturn = NULL;
						  status = thumb.GetHBITMAP(Gdiplus::Color::Transparent, hbmReturn);
						  if (status == Gdiplus::Ok)
						  {
							  hBmp = hbmReturn;
							  hr = S_OK;
						  }
						  else
							  hr = E_FAIL;
					  }
					  else
						  hr = E_FAIL;
				  }
				  else
					  hr = E_FAIL;
			  }
			  else
				  hr = E_FAIL;
		  }
		  else
			  hr = E_FAIL;
	  }
  }

  try
  {
	  if (FAILED(hr))
		  throw _com_error(hr);
  }
  catch (_com_error &ex)
  {
	  MessageBox(NULL, ex.ErrorMessage(), NULL, MB_ICONERROR);
  }

  return hr;

#elif FBSHELL_USE_ATL_CIMAGE

  CComPtr<IStream> spImageStream;
  HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &spImageStream);
  if (SUCCEEDED(hr))
  {
	  ULONG cbRead;
	  hr = spImageStream->Write(data, datalen, &cbRead);
	  free(data);
	  if (SUCCEEDED(hr))
	  {
		  CImage image;
		  image.Load(spImageStream);

		  float imageWidth = (float)image.GetWidth();
		  float imageHeight = (float)image.GetHeight();
		  float scale = 0.0f;

		  if (imageWidth <= imageHeight)
			  scale = (float)m_desired_size.cy / (float)image.GetHeight();
		  else
			  scale = (float)m_desired_size.cx / (float)image.GetWidth();

		  float thumbWidth = (float)imageWidth * scale;
		  float thumbHeight = (float)imageHeight * scale;

		  HDC hmemDC = CreateCompatibleDC(NULL);

		  SetStretchBltMode(hmemDC, SRCCOPY);

		  SetBrushOrgEx(hmemDC, 0, 0, NULL);

		  HBITMAP hmemBM = CreateCompatibleBitmap(hmemDC, (int)thumbWidth, (int)thumbHeight);

		  SelectObject(hmemDC, hmemBM);

		  image.StretchBlt(hmemDC, 0, 0, (int)thumbWidth, (int)thumbHeight, SRCCOPY);

		  *hBmp = image.Detach();

		  ReleaseDC(NULL, hmemDC);

		  DeleteObject(hmemBM);

		  hr = S_OK;
	  }
  }

  return hr;

#else

  // create an image from our data
  MemReader rdr(data,datalen);
  HDC	hDC=::GetDC(NULL);
  int	w,h;
  bool ok=ImageLoader::Load(hDC,type,&rdr,
    m_desired_size.cx,m_desired_size.cy,
    0,*hBmp,w,h);
  ::ReleaseDC(NULL,hDC);
  free(data);

  return ok ? S_OK : E_FAIL;

#endif
}

// IExtractImage2
HRESULT	CIconExtractor::GetDateStamp(FILETIME *tm) {
  HANDLE  hFile=::CreateFile(m_filename,FILE_READ_ATTRIBUTES,0,NULL,OPEN_EXISTING,0,NULL);
  if (hFile==INVALID_HANDLE_VALUE)
    return HRESULT_FROM_WIN32(::GetLastError());
  ::GetFileTime(hFile,NULL,NULL,tm);
  ::CloseHandle(hFile);
  return S_OK;
}

///////////////////////////////////////////////////////////
// SAX xml content handler (I use SAX instead of DOM for speed)
class CIconExtractor::ContentHandlerImpl :
  public CComObjectRoot,
  public MSXML2::ISAXContentHandler
{
public:
  enum ParseMode {
    NONE,
    COVERPAGE,
    DATA
  };

  // construction
  ContentHandlerImpl() : m_mode(NONE), m_data(NULL), m_ok(false) { }
  ~ContentHandlerImpl() {
    free(m_data);
  }

  DECLARE_NO_REGISTRY()

  BEGIN_COM_MAP(ContentHandlerImpl)
    COM_INTERFACE_ENTRY(MSXML2::ISAXContentHandler)
  END_COM_MAP()

  // ISAXContentHandler
  STDMETHOD(raw_characters)(USHORT *chars,int nch);
  STDMETHOD(raw_endDocument)() { return S_OK; }
  STDMETHOD(raw_startDocument)() { return S_OK; }
  STDMETHOD(raw_endElement)(USHORT *nsuri,int nslen, USHORT *name,int namelen,
	  USHORT *qname,int qnamelen);
  STDMETHOD(raw_startElement)(USHORT *nsuri,int nslen, USHORT *name,int namelen,
	  USHORT *qname,int qnamelen,MSXML2::ISAXAttributes *attr);
  STDMETHOD(raw_ignorableWhitespace)(USHORT *spc,int spclen) { return S_OK; }
  STDMETHOD(raw_endPrefixMapping)(USHORT *prefix,int len) { return S_OK; }
  STDMETHOD(raw_startPrefixMapping)(USHORT *prefix,int plen, USHORT *uri,int urilen) { return S_OK; }
  STDMETHOD(raw_processingInstruction)(USHORT *targ,int targlen, USHORT *data,int datalen) { return S_OK; }
  STDMETHOD(raw_skippedEntity)(USHORT *name,int namelen) { return S_OK; }
  STDMETHOD(raw_putDocumentLocator)(MSXML2::ISAXLocator *loc) { return S_OK; }

  // data access
  void	  *Detach() {
    void *tmp=m_data;
    m_data=NULL;
    return tmp;
  }
  int	  Length() { return m_data_ptr; }
  CString Type() { return m_cover_type; }
  bool	  Ok() { return m_ok; }

private:
  ParseMode	  m_mode;
  bool		  m_ok;

  CString	  m_cover_id;
  CString	  m_cover_type;
  DWORD		  m_data_length;
  DWORD		  m_data_ptr;
  BYTE		  *m_data;

  // base64 decoder state
  DWORD		  m_bits;
  BYTE		  m_shift;

  // extend the data array
  bool		  Extend(BYTE *np) {
    m_data_ptr=np-m_data;
    DWORD   tmp=m_data_length<<1;
    void    *mem=realloc(m_data,tmp);
    if (!mem)
      return false;
    m_data_length=tmp;
    m_data=(BYTE*)mem;
    return true;
  }
};

bool    CIconExtractor::LoadObject(const wchar_t *filename,CString& type,void *&data,int& datalen)
{
  ContentHandlerPtr	      ch;
  if (FAILED(CreateObject(ch)))
    return IStreamPtr();

  MSXML2::ISAXXMLReaderPtr    rdr;
  if (FAILED(rdr.CreateInstance(L"MSXML2.SAXXMLReader.4.0")))
    return IStreamPtr();

  rdr->putContentHandler(ch);

  rdr->raw_parseURL((USHORT *)filename);
  if (!ch->Ok())
    return false;

  type=ch->Type();
  datalen=ch->Length();
  data=ch->Detach();

  return true;
}

HRESULT	CIconExtractor::ContentHandlerImpl::raw_endElement(USHORT *nsuri,int nslen,
	USHORT *name,int namelen,
	USHORT *qname,int qnamelen)
{
  // all elements must be in a fictionbook namespace
  if (!StrEQ(FBNS,(wchar_t*)nsuri,nslen))
    return E_FAIL;

  switch (m_mode) {
  case NONE:
    if (StrEQ(L"description", (wchar_t*)name,namelen) && m_cover_id.IsEmpty())
	return E_FAIL;
    break;
  case COVERPAGE:
    if (StrEQ(L"coverpage", (wchar_t*)name,namelen)) {
      if (m_cover_id.IsEmpty())
	return E_FAIL;
      m_mode=NONE;
    }
    break;
  case DATA:
    // if we got here and have some bits left in our buffer then we have malformed
    // base64 data
    if (m_shift==18)
      m_ok=true;
    return E_FAIL;
  }

  return S_OK;
}

HRESULT	CIconExtractor::ContentHandlerImpl::raw_startElement(USHORT *nsuri,int nslen,
	USHORT *name,int namelen,
	USHORT *qname,int qnamelen,
	MSXML2::ISAXAttributes *attr)
{
  // all elements must be in a fictionbook namespace
  if (!StrEQ(FBNS, (wchar_t*)nsuri,nslen))
    return E_FAIL;

  switch (m_mode) {
  case NONE:
    if (StrEQ(L"coverpage", (wchar_t*)name,namelen))
      m_mode=COVERPAGE;
    else if (StrEQ(L"binary", (wchar_t*)name,namelen)) {
      if (m_cover_id.IsEmpty()) // invalid file
	return E_FAIL;
      if (m_cover_id!=GetAttr(attr,L"id"))
	return S_OK;

      m_cover_type=GetAttr(attr,L"content-type");

      // initialize memory block and a base64 decoder
      m_data_length=32768; // arbitrary initial size
      m_data_ptr=0;
      m_data=(BYTE*)malloc(m_data_length);
      if (m_data==NULL)
	return E_FAIL;
      m_shift=18;
      m_bits=0;
      m_mode=DATA;
    }
    break;
  case COVERPAGE:
    if (StrEQ(L"image", (wchar_t*)name,namelen)) {
      CString	tmp(GetAttr(attr,L"href",XLINKNS));
      if (tmp.GetLength()>1 && tmp[0]==_T('#')) {
	m_cover_id=tmp;
	m_cover_id.Delete(0);
	m_mode=NONE;
      }
    }
    break;
  }

  return S_OK;
}

static BYTE	g_base64_table[256]={
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,62,65,65,65,63,52,53,54,55,56,57,
58,59,60,61,65,65,65,64,65,65,65,0,1,2,3,4,5,6,7,8,9,10,
11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,65,65,65,
65,65,65,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
41,42,43,44,45,46,47,48,49,50,51,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65
};

HRESULT	CIconExtractor::ContentHandlerImpl::raw_characters(USHORT *chars,int nch) {
  if (m_mode!=DATA)
    return S_OK;

  // process base64 data and append to m_data

  // copy globals to local variables
  BYTE	  shift=m_shift;
  DWORD	  acc=m_bits;
  BYTE	  *data=m_data+m_data_ptr;
  DWORD	  space=m_data_length-m_data_ptr;

  for (wchar_t *chars_end= (wchar_t*)chars+nch; (wchar_t*)chars<chars_end; (wchar_t*)++chars) {
    BYTE     bits=g_base64_table[*chars & 0xff]; // not my problem if it wraps
    switch (bits) {
    case 64: // end of data
      switch (shift) { // store remaining bytes
      case 18:
      case 12:
	// malformed base64 data
	return E_FAIL;
      case 6: // one byte
	if (space<2) {
	  if (!Extend(data))
	    return E_FAIL;
	  data=m_data+m_data_ptr;
	  space=m_data_length-m_data_ptr;
	}
	*data++ = (BYTE)(acc>>16);
	break;
      case 0: // two bytes
	if (space<2) {
	  if (!Extend(data))
	    return E_FAIL;
	  data=m_data+m_data_ptr;
	  space=m_data_length-m_data_ptr;
	}
	*data++ = (BYTE)(acc>>16);
	*data++ = (BYTE)(acc>>8);
	break;
      }
      m_data_ptr=data-m_data;
      m_ok=true;
      return E_FAIL;
    case 65: // whitespace, ignore;
      break;
    default: // valid bits, process
      acc|=(DWORD)bits << shift;
      if ((shift-=6)>18) { // wraparound, full triplet ready
	if (space<3) {
	  if (!Extend(data))
	    return E_FAIL;
	  data=m_data+m_data_ptr;
	  space=m_data_length-m_data_ptr;
	}
	*data++ = (BYTE)(acc>>16);
	*data++ = (BYTE)(acc>>8);
	*data++ = (BYTE)(acc);
	shift=18;
	space-=3;
	acc=0;
      }
      break;
    }
  }

  // store back vars
  m_data_ptr=data-m_data;
  m_shift=shift;
  m_bits=acc;

  return S_OK;
}