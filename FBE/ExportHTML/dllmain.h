#pragma once

class CExportHTMLModule : public ATL::CAtlDllModuleT< CExportHTMLModule >
{
public:
	DECLARE_LIBID(LIBID_ExportHTMLLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_EXPORTHTML, "FD38DFC3-4734-485E-B711-0883A9F448F9")
};

extern class CExportHTMLModule _AtlModule;

