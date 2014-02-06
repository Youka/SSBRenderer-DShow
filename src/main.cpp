/*
Project: SSBRenderer-DShow
File: main.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the	product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

// Include windows 'Globally Unique Identifier'
#include <initguid.h>
#include <CGuid.h>
// Include windows streams
#include <streams.h>
// Include SSBRenderer
#include <user.h>
// File informations
#include "file_info.h"
// Include other necessary classes
#include "textconv.hpp"
#include "imgconv.hpp"

// DLL instance getter for VC compilers
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define DLL_INSTANCE reinterpret_cast<HINSTANCE>(&__ImageBase)

// GUIDs for filter
// {637A5671-7D1B-4b0c-8DB8-2E90BF253D38}
DEFINE_GUID(CLSID_SSBRenderer,
0x637a5671, 0x7d1b, 0x4b0c, 0x8d, 0xb8, 0x2e, 0x90, 0xbf, 0x25, 0x3d, 0x38);

// {FD7CD574-A481-4626-9816-2F30347009B7}
DEFINE_GUID(IID_ISSBRendererConfig,
0xfd7cd574, 0xa481, 0x4626, 0x98, 0x16, 0x2f, 0x30, 0x34, 0x70, 0x9, 0xb7);

// Filter configuration interface
interface ISSBRendererConfig : public IUnknown{
	virtual std::string GetFile() = 0;
	virtual void SetFile(std::string) = 0;
};

// Taskicon
class TaskIcon{
	private:
		// Notify icon data
		wchar_t name[64];
		struct WindowData{
			void (*on_click)(void*);
			void* userdata;
		}data;
		HWND wnd;
		NOTIFYICONDATA icon_data;
		// Messages processing
		static LRESULT CALLBACK get_window_messages(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam){
			switch(msg){
				case WM_USER+1:
					switch(lParam){
						case WM_LBUTTONDOWN:
							{
								WindowData* data = reinterpret_cast<WindowData*>(GetWindowLongPtrW(wnd, GWLP_USERDATA));
								data->on_click(data->userdata);
							}
							break;
					}
			}
			return DefWindowProc(wnd, msg, wParam, lParam);
		}
	public:
		TaskIcon(UINT id, const wchar_t* name, HICON ico, void(*on_click)(void*), void* userdata){
			// Save data in right format
			wcsncpy(this->name, name, 64); this->name[63] = L'\0';
			this->data.on_click = on_click;
			this->data.userdata = userdata;
			// Register window class
			WNDCLASSW wnd_class = {0};
			wnd_class.lpfnWndProc = get_window_messages;
			wnd_class.hInstance = DLL_INSTANCE;
			wnd_class.hIcon = ico;
			wnd_class.lpszClassName = this->name;
			RegisterClass(&wnd_class);
			// Create window
			this->wnd = CreateWindowW(this->name, L"Taskicon dummy window", 0x0, 0, 0, 100, 100, NULL, NULL, DLL_INSTANCE, NULL);
			SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG>(&this->data));
			// Fill icon data
			this->icon_data.cbSize = sizeof(NOTIFYICONDATA);
			this->icon_data.hWnd = this->wnd;	// Window for notify messages
			this->icon_data.uID = id;	// Icon identifier
			this->icon_data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;	// Messages + Icon + Tooltip
			this->icon_data.uCallbackMessage = WM_USER + 1;	// Message identifier
			this->icon_data.hIcon = ico;	// Taskbar image
			wcscpy(this->icon_data.szTip, this->name);	// Tooltip
			// Add icon to taskbar
			Shell_NotifyIconW(NIM_ADD, &this->icon_data);
		}
		~TaskIcon(){
			// Remove icon from taskbar
			Shell_NotifyIconW(NIM_DELETE, &this->icon_data);
			// Destroy window
			DestroyWindow(this->wnd);
			// Unregister window class
			UnregisterClassW(this->name, DLL_INSTANCE);
		}
};

// Procedur on taskicon left-click
void taskicon_clicked(void* userdata){
	// Get config interface
	ISSBRendererConfig* config = reinterpret_cast<ISSBRendererConfig*>(userdata);
	// Setup file selection dialog
	std::string file = config->GetFile();
	std::wstring filew = utf8_to_utf16(file);
	wchar_t cfile[MAX_PATH]; wcsncpy(cfile, filew.c_str(), MAX_PATH); cfile[MAX_PATH-1] = L'\0';
	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = NULL;
	ofn.hInstance = DLL_INSTANCE;
	ofn.lpstrFilter = L"SSB file (*.ssb)\0*.ssb\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = cfile;
	ofn.nMaxFile = sizeof(cfile);
	ofn.lpstrTitle = L"Choose SSB file";
	ofn.Flags = OFN_FILEMUSTEXIST;
	// Show file selection dialog
	if(GetOpenFileNameW(&ofn)){
		filew = cfile;
		file = utf16_to_utf8(filew);
		config->SetFile(file);
	}else
		config->SetFile("");
}

// Video filter
class SSBRenderer : public CVideoTransformFilter, public ISSBRendererConfig{
	private:
		// Renderer
		ssb_renderer renderer;
		// Control as taskicon
		TaskIcon* control;
		// Critical section for save configuration access from other interfaces
		CCritSec crit_section;
		// Configuration
		std::string filename;
		// Ctor
		SSBRenderer(IUnknown* unknown) : CVideoTransformFilter(FILTER_NAMEW, unknown, CLSID_SSBRenderer), renderer(NULL){
			this->control = new TaskIcon(0x200, FILTER_NAMEW, LoadIconW(DLL_INSTANCE, MAKEINTRESOURCEW(FILTER_LOGO)), taskicon_clicked, static_cast<ISSBRendererConfig*>(this));
		}
	public:
		// Create class instance
		static CUnknown* CALLBACK CreateInstance(LPUNKNOWN unknown, HRESULT* result){
			SSBRenderer* filter = new SSBRenderer(unknown);
			if(!filter)
				*result = E_OUTOFMEMORY;
			return filter;
		}
		// Dtor
		~SSBRenderer(){
			ssb_free_renderer(this->renderer);
			delete this->control;
		}
		// Check validation of input media stream
		HRESULT CheckInputType(const CMediaType* In){
			// Valid pointer?
			CheckPointer(In, E_POINTER);
			// Valid stream type?
			if(In->majortype != MEDIATYPE_Video || (In->subtype != MEDIASUBTYPE_RGB24 && In->subtype != MEDIASUBTYPE_RGB32) ||
				In->formattype != FORMAT_VideoInfo || In->cbFormat < sizeof(VIDEOINFOHEADER))
				return VFW_E_TYPE_NOT_ACCEPTED;
			// Valid bitmap?
			BITMAPINFOHEADER* bmp = &reinterpret_cast<VIDEOINFOHEADER*>(In->pbFormat)->bmiHeader;
			if((bmp->biBitCount != 24 && bmp->biBitCount != 32) || bmp->biCompression != BI_RGB)
				return VFW_E_TYPE_NOT_ACCEPTED;
			// Media type accepted
			return S_OK;
		}
		// Prefered output media stream type
		HRESULT GetMediaType(int position, CMediaType* Out){
			// Valid pointer?
			CheckPointer(Out, E_POINTER);
			// Input pin isn't connected
			if(!this->m_pInput->IsConnected())
				return VFW_E_NOT_CONNECTED;
			// Item pick error
			if(position < 0)
				return E_ACCESSDENIED;
			// No further items
			if(position > 0)
				return VFW_S_NO_MORE_ITEMS;
			// Output type = input type
			HRESULT hr = this->m_pInput->ConnectionMediaType(Out);
			if(FAILED(hr))
				return hr;
			// Output accepted
			return S_OK;
		}
		// Checks compatibility of input & output pin
		HRESULT CheckTransform(const CMediaType* In, const CMediaType* Out){
			// Valid pointers?
			CheckPointer(In, E_POINTER);
			CheckPointer(Out, E_POINTER);
			// In- & output the same?
			if(this->CheckInputType(In) == S_OK && *In == *Out)
				return S_OK;
			else
				return VFW_E_INVALIDMEDIATYPE;
		}
		// Allocate buffers for in- & output
		HRESULT DecideBufferSize(IMemAllocator* alloc, ALLOCATOR_PROPERTIES* props){
			// Valid pointers?
			CheckPointer(alloc, E_POINTER);
			CheckPointer(props, E_POINTER);
			// Input pin isn't connected
			if(!this->m_pInput->IsConnected())
				return VFW_E_NOT_CONNECTED;
			// Set buffer size
			props->cBuffers = 1;
			props->cbBuffer = this->m_pInput->CurrentMediaType().GetSampleSize();
			// Allocate buffer memory
			ALLOCATOR_PROPERTIES actual;
			HRESULT hr = alloc->SetProperties(props,&actual);
			if (FAILED(hr))
				return hr;
			// Enough memory allocated?
			if (actual.cBuffers < props->cBuffers ||
				actual.cbBuffer < props->cbBuffer)
						return E_OUTOFMEMORY;
			// Got memory
			return S_OK;
		}
		// Frame processing
		HRESULT Transform(IMediaSample* In, IMediaSample* Out){
			// Valid pointers?
			CheckPointer(In, E_POINTER);
			CheckPointer(Out, E_POINTER);
			// Get bitmap info
			BITMAPINFOHEADER *bmp_in = &reinterpret_cast<VIDEOINFOHEADER*>(this->m_pInput->CurrentMediaType().pbFormat)->bmiHeader;
			BITMAPINFOHEADER *bmp_out = &reinterpret_cast<VIDEOINFOHEADER*>(this->m_pOutput->CurrentMediaType().pbFormat)->bmiHeader;
			// Calculate pitches (from BITMAPINFOHEADER remarks)
			int pitch_src = (((bmp_in->biWidth * bmp_in->biBitCount) + 31) & ~31) >> 3;
			int pitch_dst = (((bmp_out->biWidth * bmp_in->biBitCount) + 31) & ~31) >> 3;
			// Get absolute frame height
			int abs_height = ::abs(bmp_in->biHeight);
			// Set output size
			Out->SetActualDataLength(abs_height * pitch_dst);
			// Get frame pointers
			BYTE *src, *dst;
			HRESULT hr;
			hr = In->GetPointer(&src);
			if(FAILED(hr))
				return hr;
			hr = Out->GetPointer(&dst);
			if(FAILED(hr))
				return hr;
			// Copy image to output
			if(pitch_src == pitch_dst)
				std::copy(src, src+In->GetActualDataLength(), dst);
			else{
				unsigned char* psrc = src, *pdst = dst;
				for(int y = 0; y < abs_height; y++){
					std::copy(psrc, psrc+pitch_src, pdst);
					psrc += pitch_src;
					pdst += pitch_dst;
				}
			}
			// Get time
			LONGLONG start, end;
			hr = In->GetTime(&start, &end);
			if(FAILED(hr))
				return hr;
			// Filter output
			if(this->renderer){
				if(bmp_in->biHeight < 0)
					frame_flip_y(dst, pitch_dst, abs_height);
				ssb_render(this->renderer, dst, pitch_dst, start / 10000);
				if(bmp_out->biHeight < 0)
					frame_flip_y(dst, pitch_dst, abs_height);
			}else if(bmp_in->biHeight != bmp_out->biHeight)
				frame_flip_y(dst, pitch_dst, abs_height);
			// Frame successfully filtered
			return S_OK;
		}
		// Start frame streaming
		HRESULT StartStreaming(){
			// Free previous renderer (in case of buggy twice start function call)
			if(this->renderer){
				ssb_free_renderer(this->renderer);
				this->renderer = NULL;
			}
			// Get video infos
			BITMAPINFOHEADER *bmp = &reinterpret_cast<VIDEOINFOHEADER*>(this->m_pInput->CurrentMediaType().pbFormat)->bmiHeader;
			// Get filename for renderer
			std::string filename = this->GetFile();
			// Create renderer
			if(!filename.empty())
				if(!(this->renderer = ssb_create_renderer(bmp->biWidth, ::abs(bmp->biHeight), bmp->biBitCount == 32 ? SSB_BGRX : SSB_BGR, filename.c_str(), NULL)))
					return VFW_E_WRONG_STATE;
			// Continue with default behaviour
			return CVideoTransformFilter::StartStreaming();
		}
		// Stop frame streaming
		HRESULT StopStreaming(){
			// Free renderer
			if(this->renderer){
				ssb_free_renderer(this->renderer);
				this->renderer = NULL;
			}
			// Continue with default behaviour
			return CVideoTransformFilter::StopStreaming();
		}
		// Number of filter pins
		int GetPinCount(){
			return 2;
		}
		// Get filter pins
		CBasePin* GetPin(int n){
			// Pick pin by index
			switch(n){
				case 0:
					// Input pin
					if (!this->m_pInput){
						// Create new one
						HRESULT hr = S_OK;
						this->m_pInput = new CTransformInputPin(L"Video input", this, &hr, L"Video input");
						if (FAILED(hr))
							return NULL;
					}
					return this->m_pInput;
				case 1:
					// Output pin
					if (!this->m_pOutput){
						// Create new one
						HRESULT hr = S_OK;
						this->m_pOutput = new CTransformOutputPin(L"Video output", this, &hr, L"Video output");
						if (FAILED(hr))
							return NULL;
					}
					return this->m_pOutput;
				default:
					// Not expected pin
					return NULL;
			}
		}
		// Answer to interface requests from outside
		HRESULT CALLBACK NonDelegatingQueryInterface(REFIID riid, __deref_out void**ppv){
			// Valid pointer?
			CheckPointer(ppv, E_POINTER);
			// Return filter configuration interface
			if(riid == IID_ISSBRendererConfig)
				return GetInterface(reinterpret_cast<ISSBRendererConfig*>(this), ppv);
			// Return default interfaces
			return CVideoTransformFilter::NonDelegatingQueryInterface(riid, ppv);
		}
		// Define COM object base methods
		DECLARE_IUNKNOWN;
		// Filter configuration interface
		std::string GetFile(){
			// Lock critical section for thread-safety
			CAutoLock lock(&this->crit_section);
			// Return filename copy
			return this->filename;
		}
		void SetFile(std::string filename){
			// Lock critical section for thread-safety
			CAutoLock lock(&this->crit_section);
			// Copy parameter to filename
			this->filename = filename;
		}
};

// Filter pins
const AMOVIESETUP_MEDIATYPE sudPinTypes[] =
{
	// Support RGB24 colorspace
	{
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB24
	},
	// Support RGB32 colorspace
	{
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB32
	}
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{ L"Input",             // Pin string name
		FALSE,                // Is it rendered
		FALSE,                // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		2,                    // Number of media types
		sudPinTypes          // Media types
	},
	{ L"Output",            // Pin string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		2,                    // Number of media types
		sudPinTypes          // Media types
	}
};

// Filter setup
const AMOVIESETUP_FILTER sudFilter =
{
	&CLSID_SSBRenderer,         // Filter CLSID
	FILTER_NAMEW,       // Filter name
	MERIT_DO_NOT_USE,       // Filter merit
	2,                      // Number of pins
	sudpPins                // Pin information
};

// Filter definition as COM objects
CFactoryTemplate g_Templates[] = {
	{
		FILTER_NAMEW,	// Filter name
		&CLSID_SSBRenderer,		// Filter CLSID
		SSBRenderer::CreateInstance,	// Filter instance creation
		NULL,		// Init routine
		&sudFilter		// Filter setup
	}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(CFactoryTemplate);	// Number of COM objects

// Register filter to server
STDAPI DllRegisterServer(){
    return AMovieDllRegisterServer2( TRUE );
}

// Unregister filter from server
STDAPI DllUnregisterServer(){
    return AMovieDllRegisterServer2( FALSE );
}

// Further DLL initializations for DirectShow
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// DLL entry point
BOOL APIENTRY DllMain(HANDLE module, DWORD reason, LPVOID reserved){
	return DllEntryPoint(reinterpret_cast<HINSTANCE>(module), reason, reserved);
}