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
// File informations
#include "file_info.h"
// Include other necessary classes
#include <string>

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










// Filter pins
const AMOVIESETUP_MEDIATYPE sudPinTypes[] =
{
	// Support RGB colorspace
	{
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB24
	},
	// Support RGBA colorspace
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
	MERIT_NORMAL,       // Filter merit
	2,                      // Number of pins
	sudpPins                // Pin information
};

// Filter definition as COM objects
CFactoryTemplate g_Templates[] = {
	{
		FILTER_NAMEW,	// Filter name
		&CLSID_SSBRenderer,		// Filter CLSID
		/*TODO*/NULL,	// Filter instance creation
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