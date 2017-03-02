///////////////////////////////////////////////////////////////////////////////
// �ļ�����  DiaHelper.cpp
// ����ʱ�䣺2007-10-28
// ���ߣ�    ����
// ��Ȩ���У�Titi Studio (?) 2001-2007
//-----------------------------------------------------------------------------
// ˵����    DIA SDK������ʵ��
///////////////////////////////////////////////////////////////////////////////

#include <pdl_base.h>
#include "DiaHelper.h"
#include <diacreate.h>
#include <pdl_com.h>
#include "SymWrap.h"
#include "callback.h"

// ע�����DIA ���汾�� CLSID �ǲ�ͬ��
CLSID clsidDIA[] = {
    // DIA 8
    { 0xbce36434, 0x2c24, 0x499e,
	{ 0xbf, 0x49, 0x8b, 0xd9, 0x9b, 0x0e, 0xeb, 0x68 } },
	// DIA 9
	{ 0x4c41678e, 0x887b, 0x4365,
	{ 0xa0, 0x9e, 0x92, 0x5d, 0x28, 0xdb, 0x33, 0xc2 } },
	// DIA 10
	{ 0xB86AE24D, 0xBF2F, 0x4AC9,
	{ 0xB5, 0xA2, 0x34, 0xB1, 0x4E, 0x4C, 0xE1, 0x1D } }
};

typedef struct _tagModifyParam {
    ModifyEvent pfnCallBack;
    LPVOID      pParam;
} MODIFYPARAM, *PMODIFYPARAM;

CDiaHelper::CDiaHelper(void)
{
}

CDiaHelper::~CDiaHelper(void)
{
}

BOOL CDiaHelper::InitDia(void)
{
    HRESULT hr;
    for (int i = 0; i < sizeof(clsidDIA) / sizeof(GUID); ++i)
    {
        m_ds = NULL;
        hr = ::CoCreateInstance(clsidDIA[i], NULL, CLSCTX_INPROC_SERVER, IID_IDiaDataSource, (PVOID*)&m_ds);
        if (SUCCEEDED(hr))
            return TRUE;
    }
    return FALSE;
}

BOOL CDiaHelper::OpenPDB(LPCWSTR lpPdbFile)
{
    if (NULL != m_pSymbol)
        m_pSymbol.Release();
    if (NULL != m_pSession)
        m_pSession.Release();
    if (NULL != m_ds)
    {
        // װ��PDBһ��֮��Ͳ�����װ���ˣ���������Ҫ����
        m_ds.Release();
        InitDia();
    }

	HRESULT hr;
	wchar_t lpDrive[_MAX_DRIVE];
	wchar_t lpDir[_MAX_DIR];
	wchar_t lpFname[_MAX_FNAME];
	wchar_t lpExt[_MAX_EXT];
	wchar_t* lpSearchPath = L"SRV**\\\\symbols\\symbols";

	//_wsplitpath_s(wszFilename,NULL,0,NULL,0,NULL,0,wszExt,MAX_PATH);
	_wsplitpath_s(lpPdbFile, lpDrive, _MAX_DRIVE, lpDir, _MAX_DIR, lpFname, _MAX_FNAME, lpExt, _MAX_EXT);
	if(!_wcsicmp(lpExt, L".pdb"))
	{
		// Open and prepare a program database (.pdb) file as a debug data source
		hr = m_ds->loadDataFromPdb(lpPdbFile);
	}
	else
	{
		CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
		// thus enabling a user interface to report on the progress of 
		// the location attempt. The client application may optionally 
		// provide a reference to its own implementation of this 
		// virtual base class to the IDiaDataSource::loadDataForExe method.
		callback.AddRef();
		// Open and prepare the debug data associated with the .exe/.dll file
		hr = m_ds->loadDataForExe(lpPdbFile, lpSearchPath, &callback);
	}
    if (FAILED(hr))
        return FALSE;

    hr = m_ds->openSession(&m_pSession);
    if (FAILED(hr))
        return FALSE;

    hr = m_pSession->get_globalScope(&m_pSymbol);
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

BOOL CDiaHelper::cbFindSymbol(IDiaSymbol* pCurSymbol, LPVOID pParam)
{
    DWORD id;
    pCurSymbol->get_symIndexId(&id);
    return id == (DWORD)pParam;
}

BOOL CDiaHelper::EnumTag(
    __in_opt IDiaSymbol* pSymbol,
    __in enum SymTagEnum enTag,
    __in EnumEvent pfnCallBack,
    __in_opt LPVOID pParam,
    __out_opt IDiaSymbol** ppRet)
{
    if (NULL == pfnCallBack)
        return FALSE;
    if (NULL == pSymbol)
        pSymbol = m_pSymbol;

    IDiaSymbol* sym = CSym::Enum(pSymbol, enTag, pfnCallBack, pParam);
    if (PDL_ARGUMENT_PRESENT(ppRet))
        *ppRet = sym;
    return TRUE;
}

IDiaSymbol* CDiaHelper::FindSymbol(
    __in_opt IDiaSymbol* pParentSym,
    __in DWORD id)
{
    if (NULL == pParentSym)
        pParentSym = m_pSymbol;
    return CSym::Enum(pParentSym, SymTagNull, cbFindSymbol, (PVOID)id);
}
