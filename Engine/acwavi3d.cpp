/* AVI/MPG player for AGS
  VMR9-based player to work with D3D
*/
//#define ALLEGRO_STATICLINK  // already defined in project settings
#include <stdio.h>
#include <allegro.h>
#include <winalleg.h>
#include <allegro/platform/aintwin.h>
#include <d3d9.h>
#include <strmif.h>
#define DWORD_PTR DWORD*
#define LONG_PTR LONG*
#define LPD3DVECTOR D3DVECTOR*
#define _D3DTYPES_H_
#define _STRSAFE_H_INCLUDED_
typedef float D3DVALUE, *LPD3DVALUE;
#include "VMR9Graph.h"
#include <ali3d.h>
#include "ac_context.h"
//#include <atlbase.h>
#include "ac_input.h"


#ifndef VS2005
// *** Workarounds to get this to work in VC 6
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUID(IID_IVMRAspectRatioControl9,0xd96c29,0xbbde,0x4efc,0x99,0x1,0xbb,0x50,0x36,0x39,0x21,0x46);
DEFINE_GUID(IID_IVMRDeinterlaceControl9,0xa215fb8d,0x13c2,0x4f7f,0x99,0x3c,0,0x3d,0x62,0x71,0xa4,0x59);
DEFINE_GUID(IID_IVMRFilterConfig9,0x5a804648,0x4f66,0x4867,0x9c,0x43,0x4f,0x5c,0x82,0x2c,0xf1,0xb8);
DEFINE_GUID(IID_IVMRImageCompositor9,0x4a5c89eb,0xdf51,0x4654,0xac,0x2a,0xe4,0x8e,0x2,0xbb,0xab,0xf6);
DEFINE_GUID(IID_IVMRImagePresenter9,0x69188c61,0x12a3,0x40f0,0x8f,0xfc,0x34,0x2e,0x7b,0x43,0x3f,0xd7);
DEFINE_GUID(IID_IVMRImagePresenterConfig9,0x45c15cab,0x6e22,0x420a,0x80,0x43,0xae,0x1f,0xa,0xc0,0x2c,0x7d);
DEFINE_GUID(IID_IVMRImagePresenterExclModeConfig,0xe6f7ce40,0x4673,0x44f1,0x8f,0x77,0x54,0x99,0xd6,0x8c,0xb4,0xea);
DEFINE_GUID(IID_IVMRMixerBitmap9,0xced175e5,0x1935,0x4820,0x81,0xbd,0xff,0x6a,0xd0,0xc,0x91,0x8);
DEFINE_GUID(IID_IVMRMixerControl9,0x1a777eaa,0x47c8,0x4930,0xb2,0xc9,0x8f,0xee,0x1c,0x1b,0xf,0x3b);
DEFINE_GUID(IID_IVMRMonitorConfig9,0x46c2e457,0x8ba0,0x4eef,0xb8,0xb,0x6,0x80,0xf0,0x97,0x87,0x49);
DEFINE_GUID(IID_IVMRSurface9,0xdfc581a1,0x6e1f,0x4c3a,0x8d,0xa,0x5e,0x97,0x92,0xea,0x2a,0xfc);
DEFINE_GUID(IID_IVMRSurfaceAllocator9,0x8d5148ea,0x3f5d,0x46cf,0x9d,0xf1,0xd1,0xb8,0x96,0xee,0xdb,0x1f);
DEFINE_GUID(IID_IVMRSurfaceAllocatorNotify9,0xdca3f5df,0xbb3a,0x4d03,0xbd,0x81,0x84,0x61,0x4b,0xfb,0xfa,0xc);
DEFINE_GUID(IID_IVMRVideoStreamControl9,0xd0cfe38b,0x93e7,0x4772,0x89,0x57,0x4,0,0xc4,0x9a,0x44,0x85);
DEFINE_GUID(IID_IVMRWindowlessControl9,0x8f537d09,0xf85e,0x4414,0xb2,0x3b,0x50,0x2e,0x54,0xc7,0x99,0x27);

GUID CLSID_VideoMixingRenderer9  = {0x51b4abf3, 0x748f, 0x4e3b, 0xa2, 
0x76, 0xc8, 0x28, 0x33, 0x0e, 0x92, 0x6a};

#endif // VS2005

#ifndef _DEBUG
	#define USES_CONVERSION int _convert; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
#else
	#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
#endif

inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	return lpw;
}
inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	return AtlA2WHelper(lpw, lpa, nChars, CP_ACP);
}
#define ATLA2WHELPER AtlA2WHelper

	#define A2W(lpa) (\
		((_lpa = lpa) == NULL) ? NULL : (\
			_convert = (lstrlenA(_lpa)+1),\
			ATLA2WHELPER((LPWSTR) alloca(_convert*2), _lpa, _convert)))


// Interface from main game

#include "ac.h"
#include "ac_context.h"
#include "acwavi.h" // for lastError

CVMR9Graph *graph = NULL;

void dxmedia_shutdown_3d()
{
  if (graph != NULL)
  {
    delete graph;
    graph = NULL;
  }
}

int dxmedia_play_video_3d(const char* filename, IDirect3DDevice9 *device, bool useAVISound, int canskip, int stretch) 
{
  HWND gameWindow = win_get_window();

  if (graph == NULL)
  {
    graph = new CVMR9Graph(gameWindow, device);
  }

  if (!useAVISound)
    update_polled_stuff_and_crossfade();

  if (!graph->SetMediaFile(filename, useAVISound))
  {
    dxmedia_shutdown_3d();
    return -1;
  }
  graph->SetLayerZOrder(0, 0);

  if (!useAVISound)
    update_polled_stuff_and_crossfade();

  if (!graph->PlayGraph())
  {
    dxmedia_shutdown_3d();
    return -1;
  }

  OAFilterState filterState = State_Running;
  while ((filterState != State_Stopped) && (!want_exit))
  {
    while (timerloop == 0) Sleep(1);
    timerloop = 0;

    if (!useAVISound)
      update_polled_stuff_and_crossfade();

    next_iteration();
    filterState = graph->GetState();

    if (rec_kbhit()) {
      int key = rec_getch();
      
      if ((canskip == 1) && (key == 27))
        break;
      if (canskip >= 2)
        break;
    }
    if ((rec_mgetbutton() >= 0) && (canskip == 3))
      break;

    //device->Present(NULL, NULL, 0, NULL);
	}

  graph->StopGraph();

  dxmedia_shutdown_3d();
  return 0;
}


// VMR9Graph.cpp: implementation of the CVMR9Graph class.
//
//////////////////////////////////////////////////////////////////////


//#include <atlbase.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// Function name	: CVMR9Graph::CVMR9Graph
// Description	    : constructor
// Return type		: 
CVMR9Graph::CVMR9Graph()
{
	InitDefaultValues();
}


// Function name	: CVMR9Graph
// Description	    : constructor
// Return type		: 
// Argument         : HWND MediaWindow
// Argument         : int NumberOfStream
CVMR9Graph::CVMR9Graph(HWND MediaWindow, IDirect3DDevice9 *device, int NumberOfStream)
{
	InitDefaultValues();
	
	if (MediaWindow != NULL) 
  {
		m_hMediaWindow = MediaWindow;
	}

	if (NumberOfStream > 0 && NumberOfStream < 11) {
		m_nNumberOfStream = NumberOfStream;
	}

  //m_pD3DDevice = device;
  m_oldWndProc = GetWindowLong(m_hMediaWindow, GWL_WNDPROC);
}

// Function name	: CVMR9Graph::~CVMR9Graph
// Description	    : destructor
// Return type		: 
CVMR9Graph::~CVMR9Graph()
{
	ReleaseAllInterfaces();
  long newProc = GetWindowLong(m_hMediaWindow, GWL_WNDPROC);
}


// Function name	: CVMR9Graph::InitDefaultValues
// Description	    : initialize all default values
// Return type		: void 
void CVMR9Graph::InitDefaultValues()
{
	ZeroMemory(m_pszErrorDescription, 1024);
	m_dwRotId				= -1;
	m_nNumberOfStream		= 4;		// default VMR9 config
	m_hMediaWindow			= NULL;
	// SRC interfaces
	for (int i=0; i<10; i++) {
		m_srcFilterArray[i] = NULL;
	}
	// SOUND interface
	m_pDirectSoundFilter	= NULL;
	// GRAPH interfaces
	m_pGraphUnknown			= NULL;
	m_pGraphBuilder			= NULL;
	m_pFilterGraph			= NULL;
	m_pFilterGraph2			= NULL;
	m_pMediaControl			= NULL;
  m_pMediaSeeking = NULL;
	//m_pMediaEvent			= NULL;
	m_pMediaEventEx			= NULL;
	// VMR9 interfaces
	m_pVMRBaseFilter		= NULL;
	m_pVMRFilterConfig		= NULL;
	m_pVMRMixerBitmap		= NULL;
	m_pVMRMixerControl		= NULL;
	m_pVMRMonitorConfig		= NULL;
	m_pVMRWindowlessControl	= NULL;
	// DIRECT3D interfaces
	m_pD3DSurface			= NULL;
}

// Function name	: CVMR9Graph::ReleaseAllInterfaces
// Description	    : release all of the graph interfaces
// Return type		: void 
void CVMR9Graph::ReleaseAllInterfaces()
{
	// CALLBACK handle
	/*if (m_pMediaEventEx != NULL) {
		m_pMediaEventEx->SetNotifyWindow(NULL, WM_MEDIA_NOTIF, NULL);
	}*/
	// SRC interfaces
	for (int i=0; i<10; i++) {
		IBaseFilter* pSrcFilter = m_srcFilterArray[i];
		if (pSrcFilter != NULL) {
			pSrcFilter->Release();
			m_srcFilterArray[i] = NULL;
		}
	}
	// SOUND interfaces
	if (m_pDirectSoundFilter != NULL) {
		m_pDirectSoundFilter->Release();
		m_pDirectSoundFilter = NULL;
	}
	// VMR9 interfaces
	if (m_pVMRFilterConfig != NULL) {
		m_pVMRFilterConfig->Release();
		m_pVMRFilterConfig = NULL;
	}
	if (m_pVMRMixerBitmap != NULL) {
		m_pVMRMixerBitmap->Release();
		m_pVMRMixerBitmap = NULL;
	}
	if (m_pVMRMixerControl != NULL) {
		m_pVMRMixerControl->Release();
		m_pVMRMixerControl = NULL;
	}
	if (m_pVMRMonitorConfig != NULL) {
		m_pVMRMonitorConfig->Release();
		m_pVMRMonitorConfig = NULL;
	}
	if (m_pVMRWindowlessControl != NULL) {
		m_pVMRWindowlessControl->Release();
		m_pVMRWindowlessControl = NULL;
	}
	if (m_pVMRBaseFilter != NULL) {
		m_pVMRBaseFilter->Release();
		m_pVMRBaseFilter = NULL;
	}
	// GRAPH interfaces
	if (m_pGraphBuilder != NULL) {
		m_pGraphBuilder->Release();
		m_pGraphBuilder = NULL;
	}
	if (m_pFilterGraph != NULL) {
		m_pFilterGraph->Release();
		m_pFilterGraph = NULL;
	}
	if (m_pFilterGraph2 != NULL) {
		m_pFilterGraph2->Release();
		m_pFilterGraph2 = NULL;
	}
	if (m_pMediaControl != NULL) {
		m_pMediaControl->Release();
		m_pMediaControl = NULL;
	}
  if (m_pMediaSeeking!= NULL) {
		m_pMediaSeeking->Release();
		m_pMediaSeeking = NULL;
	}
	/*if (m_pMediaEvent != NULL) {
		m_pMediaEvent->Release();
		m_pMediaEvent = NULL;
	}*/
	/*if (m_pMediaEventEx != NULL) {
		m_pMediaEventEx->Release();
		m_pMediaEventEx = NULL;
	}*/
	if (m_pGraphUnknown != NULL) {
		m_pGraphUnknown->Release();
		m_pGraphUnknown = NULL;
	}
	// DIRECT3D interfaces
	if (m_pD3DSurface != NULL) {
		m_pD3DSurface->Release();
		m_pD3DSurface = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////


// Function name	: CVMR9Graph::GetLastError
// Description	    : get the last error description
// Return type		: LPCTSTR 
LPCTSTR CVMR9Graph::GetLastError()
{
	return (const char*)m_pszErrorDescription;
}


// Function name	: CVMR9Graph::AddToRot
// Description	    : let the graph instance be accessible from graphedit
// Return type		: HRESULT 
// Argument         : IUnknown *pUnkGraph
// Argument         : DWORD *pdwRegister
HRESULT CVMR9Graph::AddToRot(IUnknown *pUnkGraph) 
{
	if (pUnkGraph == NULL) {
		return E_INVALIDARG;
	}

    IMoniker * pMoniker;
    IRunningObjectTable *pROT;
    if (FAILED(GetRunningObjectTable(0, &pROT))) {
        return E_FAIL;
    }
    WCHAR wsz[256];
    wsprintfW(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());
    HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) {
        hr = pROT->Register(0, pUnkGraph, pMoniker, &m_dwRotId);
        pMoniker->Release();
    }
    pROT->Release();

    return hr;
}


// Function name	: CVMR9Graph::RemoveFromRot
// Description	    : remove the graph instance accessibility from graphedit
// Return type		: void 
void CVMR9Graph::RemoveFromRot()
{
	if (m_dwRotId != -1) {
		IRunningObjectTable *pROT;
		if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
			pROT->Revoke(m_dwRotId);
			m_dwRotId = -1;
			pROT->Release();
		}
	}
}


// Function name	: CVMR9Graph::GetPin
// Description	    : return the desired pin
// Return type		: IPin* 
// Argument         : IBaseFilter *pFilter
// Argument         : PIN_DIRECTION PinDir
IPin* CVMR9Graph::GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
    BOOL       bFound = FALSE;
    IEnumPins  *pEnum;
    IPin       *pPin;

    pFilter->EnumPins(&pEnum);
    while(pEnum->Next(1, &pPin, 0) == S_OK) {
        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);
        if (PinDir == PinDirThis)
        {
            IPin *pTmp = 0;
            if (SUCCEEDED(pPin->ConnectedTo(&pTmp)))  // Already connected, not the pin we want.
            {
                pTmp->Release();
            }
            else  // Unconnected, this is the pin we want.
            {
              bFound = true;
              break;
            }
        }
        pPin->Release();
    }
    pEnum->Release();

    return (bFound ? pPin : 0);
}


// Function name	: CVMR9Graph::ReportError
// Description	    : report an error in the dump device
// Return type		: void 
// Argument         : const char* pszError
// Argument         : HRESULT hrCode
void CVMR9Graph::ReportError(const char* pszError, HRESULT hrCode)
{
	TCHAR szErr[MAX_ERROR_TEXT_LEN];
	DWORD res = AMGetErrorText(hrCode, szErr, MAX_ERROR_TEXT_LEN);
	if (res != 0) {
 		sprintf(m_pszErrorDescription, "[ERROR in %s, line %d] %s : COM Error 0x%x, %s", __FILE__, __LINE__, pszError, hrCode, szErr);
	} else {
		sprintf(m_pszErrorDescription, "[ERROR in %s, line %d] %s : COM Error 0x%x", __FILE__, __LINE__, pszError, hrCode);
	}
  strcpy(lastError, m_pszErrorDescription);
	//TRACE("%s \r\n", m_pszErrorDescription);
}


// Function name	: CVMR9Graph::GetNextFilter
// Description	    : 
// Return type		: HRESULT 
// Argument         : IBaseFilter *pFilter
// Argument         : PIN_DIRECTION Dir
// Argument         : IBaseFilter **ppNext
HRESULT CVMR9Graph::GetNextFilter(IBaseFilter *pFilter, PIN_DIRECTION Dir, IBaseFilter **ppNext)
{
    if (!pFilter || !ppNext) return E_POINTER;

    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr)) return hr;
    while (S_OK == pEnum->Next(1, &pPin, 0)) {
        // See if this pin matches the specified direction.
        PIN_DIRECTION ThisPinDir;
        hr = pPin->QueryDirection(&ThisPinDir);
        if (FAILED(hr)) {
            // Something strange happened.
            hr = E_UNEXPECTED;
            pPin->Release();
            break;
        }
        if (ThisPinDir == Dir) {
            // Check if the pin is connected to another pin.
            IPin *pPinNext = 0;
            hr = pPin->ConnectedTo(&pPinNext);
            if (SUCCEEDED(hr))
            {
                // Get the filter that owns that pin.
                PIN_INFO PinInfo;
                hr = pPinNext->QueryPinInfo(&PinInfo);
                pPinNext->Release();
                pPin->Release();
                pEnum->Release();
                if (FAILED(hr) || (PinInfo.pFilter == NULL))
                {
                    // Something strange happened.
                    return E_UNEXPECTED;
                }
                // This is the filter we're looking for.
                *ppNext = PinInfo.pFilter; // Client must release.
                return S_OK;
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching filter.
    return E_FAIL;
}


// Function name	: CVMR9Graph::RemoveFilterChain
// Description	    : remove a chain of filter, stopping at pStopFilter
// Return type		: BOOL 
// Argument         : IBaseFilter* pFilter
// Argument         : IBaseFilter* pStopFilter
BOOL CVMR9Graph::RemoveFilterChain(IBaseFilter* pFilter, IBaseFilter* pStopFilter)
{
	HRESULT hr;

	IBaseFilter* pFilterFound = NULL;
	
	hr = GetNextFilter(pFilter, PINDIR_OUTPUT, &pFilterFound);
	if (SUCCEEDED(hr) && pFilterFound != pStopFilter) {
		RemoveFilterChain(pFilterFound, pStopFilter);
		pFilterFound->Release();
	}

	m_pFilterGraph->RemoveFilter(pFilter);

	return TRUE;
}


// Function name	: CVMR9Graph::AddFilterByClsid
// Description	    : add a filter in the chain
// Return type		: HRESULT 
// Argument         : IGraphBuilder *pGraph
// Argument         : LPCWSTR wszName
// Argument         : const GUID& clsid
// Argument         : IBaseFilter **ppF
HRESULT CVMR9Graph::AddFilterByClsid(IGraphBuilder *pGraph, LPCWSTR wszName, const GUID& clsid, IBaseFilter **ppF)
{
    *ppF = NULL;
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)ppF);
    if (SUCCEEDED(hr))
    {
        hr = pGraph->AddFilter((*ppF), wszName);
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////
// Layer helper methods
//////////////////////////////////////////////////////////////////////


// Function name	: CVMR9Graph::IsValidLayer
// Description	    : check for valid layer
// Return type		: BOOL 
// Argument         : int nLayer
BOOL CVMR9Graph::IsValidLayer(int nLayer)
{
	if (nLayer > 9 || nLayer < 0) return FALSE;
	
	IBaseFilter* pBaseFilter = m_srcFilterArray[nLayer];
	if (pBaseFilter == NULL) 
		return FALSE;
	else
		return TRUE;
}



//////////////////////////////////////////////////////////////////////
// Graph Construction / Render
//////////////////////////////////////////////////////////////////////


// We need this because the filter graph must be built
// on the D3D thread
static int wndproc_build_filter_graph()
{
  return graph->BuildAndRenderGraph(graph->UseAVISound);
}

BOOL CVMR9Graph::BuildAndRenderGraph(bool withSound)
{
	USES_CONVERSION;

  int nLayer = 0;
  HRESULT hr;

	// ENSURE that a valid graph builder is available
	if (m_pGraphBuilder == NULL) {
		BOOL bRet = BuildFilterGraph(withSound);
		if (!bRet) return bRet;
	}

  // ENSURE that the filter graph is in a stop state
	OAFilterState filterState;
	m_pMediaControl->GetState(500, &filterState);
	if (filterState != State_Stopped) {
		m_pMediaControl->Stop();
	}

  	// CHECK a source filter availaibility for the layer
	if (m_srcFilterArray[nLayer] == NULL) {
		char pszFilterName[10];
		sprintf(pszFilterName, "SRC%02d", nLayer);
		IBaseFilter* pBaseFilter = NULL;
		hr = m_pGraphBuilder->AddSourceFilter(A2W(m_pszFileName), A2W(pszFilterName), &pBaseFilter);
		if (FAILED(hr)) {
			ReportError("Could not find a source filter for this file", hr);
			return FALSE;
		}
		m_srcFilterArray[nLayer] = pBaseFilter;
	} else {
		// suppress the old src filter
		IBaseFilter* pBaseFilter = m_srcFilterArray[nLayer];
		RemoveFilterChain(pBaseFilter, m_pVMRBaseFilter);
		pBaseFilter->Release();
		m_srcFilterArray[nLayer] = NULL;
		// create a new src filter
		char pszFilterName[10];
		sprintf(pszFilterName, "SRC%02d", nLayer);
		hr = m_pGraphBuilder->AddSourceFilter(A2W(m_pszFileName), A2W(pszFilterName), &pBaseFilter);
		m_srcFilterArray[nLayer] = pBaseFilter;
		if (FAILED(hr)) {
			m_srcFilterArray[nLayer] = NULL;
			ReportError("Could not load the file", hr);
			return FALSE;
		}
	}

	// RENDER the graph
	BOOL bRet = RenderGraph();
	if (!bRet) return bRet;

  return TRUE;
}

// Function name	: CVMR9Graph::SetMediaFile
// Description	    : set a media source
// Return type		: BOOL 
// Argument         : const char* pszFileName
// Argument         : int nLayer = 0
BOOL CVMR9Graph::SetMediaFile(const char* pszFileName, bool withSound, int nLayer)
{

	if (pszFileName == NULL) {
		ReportError("Could not load a file with an empty file name", E_INVALIDARG);
		return FALSE;
	}

  UseAVISound = withSound;
  m_pszFileName = pszFileName;

  if (!wnd_call_proc(wndproc_build_filter_graph))
    return FALSE;


	return TRUE;
}

// Function name	: CVMR9Graph::BuildFilterGraph
// Description	    : construct the filter graph
// Return type		: BOOL 
BOOL CVMR9Graph::BuildFilterGraph(bool withSound)
{
	HRESULT hr;

	ReleaseAllInterfaces();
	RemoveFromRot();
	
	// BUILD the filter graph
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IUnknown, (void**) &m_pGraphUnknown);
	if (FAILED(hr)) {
		ReportError("Could not build the graph", hr);
		return FALSE;
	}
	// QUERY the filter graph interfaces
	hr = m_pGraphUnknown->QueryInterface(IID_IGraphBuilder, (void**) &m_pGraphBuilder);
	hr = m_pGraphUnknown->QueryInterface(IID_IFilterGraph, (void**) &m_pFilterGraph);
	hr = m_pGraphUnknown->QueryInterface(IID_IFilterGraph2, (void**) &m_pFilterGraph2);
	hr = m_pGraphUnknown->QueryInterface(IID_IMediaControl, (void**) & m_pMediaControl);
  hr = m_pGraphUnknown->QueryInterface(IID_IMediaSeeking, (void**) & m_pMediaSeeking);
	//hr = m_pGraphUnknown->QueryInterface(IID_IMediaEvent, (void**) &m_pMediaEvent);
	//hr = m_pGraphUnknown->QueryInterface(IID_IMediaEventEx, (void**) &m_pMediaEventEx);

/*	// SET the graph state window callback
	if (m_pMediaEventEx != NULL) {
		m_pMediaEventEx->SetNotifyWindow((OAHWND)m_hMediaWindow, WM_MEDIA_NOTIF, NULL);
    //m_pMediaEventEx->SetNotifyWindow(NULL, NULL, NULL);
	}*/

  if (withSound)
	  BuildSoundRenderer();

// Don't known what's wrong... but RenderEx crash when playing whith graphedit build 021204 ...
  //do we need this??
	//AddToRot(m_pGraphUnknown);

	return BuildVMR();
}


// Function name	: CVMR9Graph::BuildVMR
// Description	    : construct and add the VMR9 renderer to the graph
// Return type		: BOOL 
BOOL CVMR9Graph::BuildVMR()
{
	HRESULT hr;

	if (m_hMediaWindow == NULL) {
		ReportError("Could not operate without a Window", E_FAIL);
		return FALSE;
	}

	if (m_pGraphBuilder == NULL) {
		ReportError("Could not build the VMR, the graph isn't valid", E_FAIL);
		return FALSE;
	}

	// BUILD the VMR9
	IBaseFilter *pVmr = NULL;
	hr = CoCreateInstance(CLSID_VideoMixingRenderer9, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**) &m_pVMRBaseFilter);
	if (FAILED(hr)) {
		ReportError("Could not create an instance ofthe VMR9", hr);
		return FALSE;
	}

	// ADD the VMR9 to the graph
	hr = m_pGraphBuilder->AddFilter(m_pVMRBaseFilter, L"VMR9");
	if (FAILED(hr)) {
		ReportError("Could not add the VMR9 to the Graph", hr);
		return FALSE;
	}

	// DIRECT3D
	//BOOL bD3D = BuildDirect3d();

	// QUERY the VMR9 interfaces
	hr = m_pVMRBaseFilter->QueryInterface(IID_IVMRFilterConfig9, (void**) &m_pVMRFilterConfig);
	if (SUCCEEDED(hr)) {
		// CONFIGURE the VMR9
		m_pVMRFilterConfig->SetRenderingMode(VMR9Mode_Windowless);
		m_pVMRFilterConfig->SetNumberOfStreams(m_nNumberOfStream);
	}

	hr = m_pVMRBaseFilter->QueryInterface(IID_IVMRWindowlessControl9, (void**) &m_pVMRWindowlessControl);
	if (SUCCEEDED(hr)) {
		// CONFIGURE the VMR9
		m_pVMRWindowlessControl->SetVideoClippingWindow(m_hMediaWindow);
		m_pVMRWindowlessControl->SetAspectRatioMode(VMR9ARMode_LetterBox);
	}

	hr = m_pVMRBaseFilter->QueryInterface(IID_IVMRMixerBitmap9, (void**) &m_pVMRMixerBitmap);
	hr = m_pVMRBaseFilter->QueryInterface(IID_IVMRMixerControl9, (void**) &m_pVMRMixerControl);
	hr = m_pVMRBaseFilter->QueryInterface(IID_IVMRMonitorConfig9, (void**) &m_pVMRMonitorConfig);

	return TRUE;
}


// Function name	: CVMR9Graph::BuildSoundRendered
// Description	    : build the DirectSound renderer
// Return type		: BOOL 
BOOL CVMR9Graph::BuildSoundRenderer()
{
	HRESULT hr;

	hr = AddFilterByClsid(m_pGraphBuilder, L"DirectSound", CLSID_DSoundRender, &m_pDirectSoundFilter);
	if (FAILED(hr)) {
		ReportError("Could not add the DirectSoundRenderer", hr);
		return FALSE;
	}
	return TRUE;
}

// Function name	: CVMR9Graph::RenderGraph
// Description	    : render the graph
// Return type		: BOOL 
BOOL CVMR9Graph::RenderGraph()
{
	HRESULT hr;

	if (m_pFilterGraph2 == NULL) {
		ReportError("Could not render the graph because it is not fully constructed", E_FAIL);
		return FALSE;
	}

	for (int i=0; i<10; i++) {
		IBaseFilter* pBaseFilter = m_srcFilterArray[i];
		if (pBaseFilter != NULL) {
			IPin* pPin;
      while ((pPin = GetPin(pBaseFilter, PINDIR_OUTPUT)) != NULL)
      {
        hr = m_pFilterGraph2->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);
        if (FAILED(hr))
        {
          ReportError("Unable to render the pin", hr);
          return FALSE;
        }
      }
		}
	}
	return TRUE;
}


// Function name	: CVMR9Graph::PreserveAspectRatio
// Description	    : set aspect ratio mode
// Return type		: BOOL 
// Argument         : BOOL bPreserve
BOOL CVMR9Graph::PreserveAspectRatio(BOOL bPreserve)
{
	if (m_pVMRWindowlessControl == NULL) {
		ReportError("Can't set aspect ratio, no VMR", E_FAIL);
		return FALSE;
	}

	if (bPreserve)
		m_pVMRWindowlessControl->SetAspectRatioMode(VMR9ARMode_LetterBox);
	else
		m_pVMRWindowlessControl->SetAspectRatioMode(VMR9ARMode_None);

	return TRUE;
}


// Function name	: CVMR9Graph::AddFilter
// Description	    : manually add a filter in the graph
// Return type		: IBaseFilter* : caller responsible of release
// Argument         : const char* pszName
// Argument         : const GUID& clsid
IBaseFilter* CVMR9Graph::AddFilter(const char* pszName, const GUID& clsid)
{
	USES_CONVERSION;

	HRESULT hr;

	IBaseFilter* pBaseFilter = NULL;

	if (pszName == NULL) {
		ReportError("Can't add filter, no valid name", E_INVALIDARG);
		return NULL;
	}

	hr = AddFilterByClsid(m_pGraphBuilder, A2W(pszName), clsid, &pBaseFilter);
	if (FAILED(hr)) {
		ReportError("Can't add filter", hr);
		return NULL;
	}

	return pBaseFilter;
}

// Function name	: CVMR9Graph::PlayGraph
// Description	    : run the graph
// Return type		: BOOL 
BOOL CVMR9Graph::PlayGraph()
{
	if (m_pMediaControl == NULL) {
		ReportError("Can't play, no graph", E_FAIL);
		return FALSE;
	}
	if (m_pVMRWindowlessControl == NULL) {
		ReportError("Can't play, no VMR", E_FAIL);
		return FALSE;
	}

	// MEDIA SIZE
	LONG  Width;
	LONG  Height;
	LONG  ARWidth;
	LONG  ARHeight;
	m_pVMRWindowlessControl->GetNativeVideoSize(&Width, &Height, &ARWidth, &ARHeight);

	RECT mediaRect;
	mediaRect.left = 0;
	mediaRect.top = 0;
	mediaRect.right = Width;
	mediaRect.bottom = Height;

	RECT wndRect;
	GetClientRect(m_hMediaWindow, &wndRect);

	m_pVMRWindowlessControl->SetVideoPosition(&mediaRect, &wndRect);

	// RUN
	m_pMediaControl->Run();

	return TRUE;
}


// Function name	: CVMR9Graph::StopGraph
// Description	    : stop the graph
// Return type		: BOOL 
BOOL CVMR9Graph::StopGraph()
{
	if (m_pMediaControl == NULL) {
		ReportError("Can't stop, no graph", E_FAIL);
		return FALSE;
	}

	m_pMediaControl->Stop();

	return TRUE;
}

OAFilterState CVMR9Graph::GetState()
{
  OAFilterState filterState;
  m_pMediaControl->GetState(500, &filterState);
  if (filterState == State_Running)
  {
    LONGLONG curPos;
    m_pMediaSeeking->GetCurrentPosition(&curPos);
    LONGLONG length;
    m_pMediaSeeking->GetDuration(&length);

    if (curPos >= length)
    {
      filterState = State_Stopped;
    }
  }

  return filterState;
}


// Function name	: CVMR9Graph::ResetGraph
// Description	    : reset the graph - clean interfaces
// Return type		: BOOL 
BOOL CVMR9Graph::ResetGraph()
{
	// STOP the graph
	if (m_pMediaControl != NULL) {
		m_pMediaControl->Stop();
	}

	try {
		ReleaseAllInterfaces();
	} catch(...) {
		ReportError("Can't reset graph, we have serious bugs...", E_FAIL);
		return FALSE;
	}

	return TRUE;
}


// Function name	: SetLayerZOrder
// Description	    : set z order of the layer
// Return type		: BOOL 
// Argument         : int nLayer
// Argument         : DWORD dwZOrder	: bigger is away
BOOL CVMR9Graph::SetLayerZOrder(int nLayer, DWORD dwZOrder)
{
	HRESULT hr;

	if (!IsValidLayer(nLayer)) {
		ReportError("Can't set order, incorect layer", E_INVALIDARG);
		return FALSE;
	}

	if (m_pVMRMixerControl == NULL) {
		ReportError("Can't set order, no VMR", E_FAIL);
		return FALSE;
	}

	hr = m_pVMRMixerControl->SetZOrder(nLayer, dwZOrder);
	if (FAILED(hr)) {
		ReportError("Can't set ZOrder", hr);
		return FALSE;
	}

	return TRUE;
}



