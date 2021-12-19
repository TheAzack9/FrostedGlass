#include <Windows.h>
#include "../API/RainmeterAPI.h"
#include <string>
#include <sstream>
#include <VersionHelpers.h>
#include <dwmapi.h>

// Overview: This is a blank canvas on which to build your plugin.

// Note: GetString and ExecuteBang have been commented out. If you need
// GetString and/or ExecuteBang and you have read what they are used for
// from the SDK docs, uncomment the function(s). Otherwise leave them
// commented out (or get rid of them)!

const int WCA_ACCENT_POLICY = 19;

enum AccentFlags
{
	LEFTBORDER = 0x20,
	TOPBORDER = 0x40,
	RIGHTBORDER = 0x80,
	BOTTOMBORDER = 0x100,
	ALLBORDERS = (LEFTBORDER | TOPBORDER | RIGHTBORDER | BOTTOMBORDER)
};

enum class AccentState
{
	DISABLED = 0,
	GRADIENT = 1, // not used (only gives a solid color based on Gradient Color)
	TRANSPARENTGRADIENT = 2, // not used (Always gives light blue for me >.>)
	BLURBEHIND = 3,
	ACRYLIC = 4,
	INVALID = 5
};

struct ACCENTPOLICY
{
	int nAccentState;
	int nFlags;
	unsigned int nColor;
	int nAnimationId;
};

struct WINCOMPATTRDATA
{
	int nAttribute;
	PVOID pData;
	ULONG ulDataSize;
};

HINSTANCE hModule;
typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
pSetWindowCompositionAttribute SetWindowCompositionAttribute;
typedef BOOL(WINAPI* pGetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
pGetWindowCompositionAttribute GetWindowCompositionAttribute;

HMODULE hDwmApi;
typedef HRESULT(WINAPI* pDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
pDwmSetWindowAttribute SetWindowAttribute;

int references = 0;

bool compare(std::wstring& in, const std::wstring& search)
{
	int end = 0;
	while (in.size() > end && in[end] == L' ') ++end;
	in.erase(0, end);
	if (_wcsnicmp(in.c_str(), search.c_str(), search.size()) == 0)
	{
		in.erase(0, search.size());
		return true;
	}
	return false;
}

bool IsWindowsBuildOrGreater (WORD wMajorVersion, WORD wMinorVersion, DWORD dwBuildNumber) {
    OSVERSIONINFOEXW osvi = { sizeof (osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
    DWORDLONG mask = 0;

    mask = VerSetConditionMask(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    mask = VerSetConditionMask(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
    mask = VerSetConditionMask(mask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

    osvi.dwMajorVersion = wMajorVersion;
    osvi.dwMinorVersion = wMinorVersion;
    osvi.dwBuildNumber = dwBuildNumber;

    return VerifyVersionInfoW (&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask) != FALSE;
}

bool SetSkinAccent(HWND hwnd, const int& border, const AccentState& state)
{

	if (hModule && SetWindowCompositionAttribute)
	{
		ACCENTPOLICY policy = { (int)state, border, 0x01000000, 1 }; // set black with 1 in transparency because acrylic does not like transparent
		WINCOMPATTRDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY) };
		SetWindowCompositionAttribute(hwnd, &data);
		return true;
	}
	return false;
}

void loadModule()
{
	if (!hModule || SetWindowCompositionAttribute == NULL) {
		hModule = LoadLibrary(TEXT("user32.dll"));
		SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");

		if (SetWindowCompositionAttribute == NULL)
		{
			RmLog(LOG_ERROR, L"Could not load the SetWindowCompositionAttribute function from user32.dll, did microsoft remove it?");
		}

		GetWindowCompositionAttribute = (pGetWindowCompositionAttribute)GetProcAddress(hModule, "GetWindowCompositionAttribute");

		if (GetWindowCompositionAttribute == NULL)
		{
			RmLog(LOG_ERROR, L"Could not load the GetWindowCompositionAttribute function from user32.dll, did microsoft remove it?");
		}

		hDwmApi = LoadLibrary(L"DWMAPI.dll");
		SetWindowAttribute = (pDwmSetWindowAttribute)GetProcAddress(hDwmApi, "DwmSetWindowAttribute");

		if (SetWindowAttribute == NULL) {
			RmLog(LOG_ERROR, L"Could not load the DwmSetWindowAttribute function from DWMAPI.dll");
		}
	}
	references++;
}

void unloadModule()
{
	references--;
	if(references <= 0)
	{
		FreeLibrary(hModule);
		FreeLibrary(hDwmApi);
		SetWindowCompositionAttribute = NULL;
		SetWindowAttribute = NULL;
	}
}

struct Measure
{
	HWND skin;
	bool isBlurred;
	AccentState prevState = (AccentState)0;
	int prevBorder = 0;
	DWM_WINDOW_CORNER_PREFERENCE prevCorner = DWMWCP_DONOTROUND;
	bool doWarn = true;
};

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* m = new Measure;
	m->skin = RmGetSkinWindow(rm);
	loadModule();
	*data = m;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* m = (Measure*)data;
	m->skin = RmGetSkinWindow(rm);

	std::wstring type = RmReadString(rm, L"Type", L"Blur");

	AccentState accent = AccentState::BLURBEHIND;
        if (_wcsicmp(type.c_str(), L"ACRYLIC") == 0) accent = AccentState::ACRYLIC;
	if (_wcsicmp(type.c_str(), L"NONE") == 0) accent = AccentState::DISABLED;

	if(!IsWindows10OrGreater() && !IsWindowsBuildOrGreater(10, 0, 17063) && accent == AccentState::ACRYLIC)
	{
		if (m->doWarn)
			RmLogF(rm, LOG_WARNING, L"Acrylic is not supported on windows 10 builds until build 17063. Falling back to blur.");
		accent = AccentState::BLURBEHIND;
	}

	if(!IsWindows10OrGreater() && (accent == AccentState::ACRYLIC || accent == AccentState::BLURBEHIND))
	{
		if(m->doWarn)
			RmLogF(rm, LOG_WARNING, L"This plugin is not supported on other platforms than Windows 10");

		accent = AccentState::DISABLED;
		m->doWarn = m->prevState != accent;
		m->prevState = accent;
		return;
	}

	std::wstring borderTypes = RmReadString(rm, L"Border", L"");
	int borders = 0;

	while(!borderTypes.empty())
	{
		if (compare(borderTypes, L"TOP")) borders |= TOPBORDER;
		else if (compare(borderTypes, L"LEFT")) borders |= LEFTBORDER;
		else if (compare(borderTypes, L"RIGHT")) borders |= RIGHTBORDER;
		else if (compare(borderTypes, L"BOTTOM")) borders |= BOTTOMBORDER;
		else if (compare(borderTypes, L"NONE")) break;
		else if (compare(borderTypes, L"ALL"))
		{
			borders = ALLBORDERS;
			break;
		}

		if(!borderTypes.empty() && !compare(borderTypes, L"|"))
		{
			if(m->doWarn)
				RmLogF(rm, LOG_ERROR, L"Invalid border format, expected | between tokens");

			borders = 0;
			break;
		}
	}

	// backwards compability
	if (RmReadInt(rm, L"BlurEnabled", 1) == 0) accent = AccentState::DISABLED;

	// If nothing changed, do nothing
	if (m->prevState == accent && m->prevBorder == borders) return;

	if(!(SetSkinAccent(RmGetSkinWindow(rm), borders, AccentState::DISABLED) &&
		SetSkinAccent(RmGetSkinWindow(rm), borders, accent)))  {
		RmLogF(rm, LOG_ERROR, L"Could not load library user32.dll for some unknown reason.");
	}

	DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_DONOTROUND;
    std::wstring cornerType = RmReadString(rm, L"Corner", L"");
    if (_wcsicmp(cornerType.c_str(), L"ROUND") == 0) {
        corner = DWMWCP_ROUND;
    } else if (_wcsicmp(cornerType.c_str(), L"ROUNDSMALL") == 0) {
        corner = DWMWCP_ROUNDSMALL;
    }

    if (IsWindowsBuildOrGreater(10, 0, 22000))
	{
		if (SetWindowAttribute) {
			SetWindowAttribute(RmGetSkinWindow(rm), DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof corner);
		}
    } else if (corner != DWMWCP_DONOTROUND) {
		if (m->doWarn)
			RmLogF(rm, LOG_WARNING, L"Round corner is not supported on windows 11 builds until build 22000. Falling back to do not round.");

        corner = DWMWCP_DONOTROUND;
	}

	m->doWarn = m->prevState != accent || m->prevBorder != borders || m->prevCorner != corner;
	m->prevState = accent;
	m->prevBorder = borders;
	m->prevCorner = corner;
}

PLUGIN_EXPORT double Update(void* data)
{
	return 0.0;
}

//PLUGIN_EXPORT LPCWSTR GetString(void* data)
//{
//	Measure* measure = (Measure*)data;
//	return L"";
//}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* m = (Measure*)data;
	std::wstring sargs = args;
	bool failed = false;

	if(compare(sargs, L"ENABLEBLUR"))
	{
		failed = SetSkinAccent(m->skin, 0, AccentState::BLURBEHIND);
		m->isBlurred = true;
	}
	else if (compare(sargs, L"DISABLEBLUR"))
	{
		failed = SetSkinAccent(m->skin, 0, AccentState::DISABLED);
		m->isBlurred = false;
	}
	else if (compare(sargs, L"TOGGLEBLUR"))
	{
		failed = SetSkinAccent(m->skin, 0, m->isBlurred ? AccentState::DISABLED : AccentState::BLURBEHIND);
		m->isBlurred = !m->isBlurred;
	}

	if(failed)
	{
		// Error
		RmLog(LOG_ERROR, L"Could not load library user32.dll for some unknown reason.");
	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* m = (Measure*)data;
	// Disable the effect, the skin is unloaded
	if(!SetSkinAccent(m->skin, 0, AccentState::DISABLED))
	{
		// Error
		RmLog(LOG_ERROR, L"Could not load library user32.dll for some unknown reason.");
	}

	unloadModule();
	delete m;
}
