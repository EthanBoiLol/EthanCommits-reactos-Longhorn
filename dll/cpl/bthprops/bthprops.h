#pragma once

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <winreg.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <limits.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dbt.h>
#include <regstr.h>

#include "resource.h"

// Globals
extern HINSTANCE hApplet;

// defines
#define NUM_APPLETS    (1)

// global structures
typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
}APPLET, *PAPPLET;



// bthprops.c
LONG
APIENTRY
InitApplet(
    HWND hwnd,
    UINT uMsg,
    LPARAM wParam,
    LPARAM lParam);
