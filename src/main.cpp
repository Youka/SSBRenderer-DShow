/*
Project: SSBRenderer-DShow
File: main.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

// Include windows 'Globally Unique Identifier'
#include <initguid.h>
#include <CGuid.h>
// Include windows streams
#include <streams.h>

// DLL instance getter for VC compilers
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define DLL_INSTANCE reinterpret_cast<HINSTANCE>(&__ImageBase)


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
	&CLSID_LVSVideoFilter,         // Filter CLSID
	FILTER_NAMEW,       // Filter name
	MERIT_NORMAL,       // Filter merit
	2,                      // Number of pins
	sudpPins                // Pin information
};

// Filter definition as COM objects
CFactoryTemplate g_Templates[] = {
	{
		FILTER_NAMEW,	// Filter name
		&CLSID_LVSVideoFilter,		// Filter CLSID
		LVSVideoFilter::CreateInstance,	// Filter instance creation
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