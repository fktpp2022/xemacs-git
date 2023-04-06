/* Unicode-encapsulation of Win32 library functions.
   Copyright (C) 2000, 2001, 2002, 2004, 2010 Ben Wing.

This file is part of XEmacs.

XEmacs is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs.  If not, see <http://www.gnu.org/licenses/>. */

/* Synched up with: Not in FSF. */

/* Authorship:

   Current primary author: Ben Wing <ben@xemacs.org>

   Created summer 2000 by Ben Wing.  Completed August 2001.  Completely
   written by Ben Wing.
   */

#define NEED_MSWINDOWS_COMMCTRL
#define NEED_MSWINDOWS_SHLOBJ

#include <config.h>
#include "lisp.h"

#include "console-msw.h"


/************************************************************************/
/*                              auto-generation                         */
/************************************************************************/

/* we use a simple script to control the auto-generation.

\(The following is copied from lib-src/make-mswin-unicode.pl.)

file specifies a file to start reading from.
yes indicates a function to be automatically Unicode-encapsulated.
   (All parameters either need no special processing or are LPTSTR or
   LPCTSTR.)
override indidates a function where the prototype can be overridden
   due to errors in Cygwin or Visual Studio.
soon indicates a function that should be automatically Unicode-encapsulated,
   but we're not ready to process it yet.
no indicates a function we don't support (it will be #defined to cause
   a compile error, with the text after the function included in the
   erroneous definition to indicate why we don't support it).
review indicates a function that we still need to review to determine whether
   or how to support it.  This has the same effect as `no', with a comment
   indicating that the function needs review.
skip indicates a function we support manually; only a comment about this
   will be generated.
split indicates a function with a split structure (different versions
   for Unicode and ANSI), but where the only difference is in pointer
   types, and the actual size does not differ.  The structure name
   should follow the function name, and it will be automatically
   Unicode-encapsulated with appropriate casts.
begin-bracket indicates a #if statement to be inserted here.
end-bracket indicates the corresponding #endif statement.
blank lines and lines beginning with // are ignored.

The generated files go into intl-auto-encap-win32.[ch].

To regenerate, go to the nt/ subdirectory and type

nmake -f xemacs.mak unicode-encapsulate

This does the following:

	cd $(SRC)
	perl ../lib-src/make-mswin-unicode.pl --h-output intl-auto-encap-win32.h intl-encap-win32.c

*/

/*

terminology used below:

"split-simple" means a structure where the A and W versions are the same
size, and the only differences are string pointer arguments. (This does NOT
include structures with a pointer to a split-sized structure within them.)
This can also refer to a function pointer whose only split arguments are
string pointers or split-simple structures.

"split-sized" means a structure where the A and W versions are different
sizes (typically because of an inline string argument), or where there's a
pointer to another split-sized structure.

"split-complex" 

begin-unicode-encapsulation-script

// dir c:\Program Files\Microsoft Visual Studio\VC98\Include\

file ACLAPI.h

skip GetNamedSecurityInfo Cygwin declaration makes first param const
review BuildExplicitAccessWithName
review BuildSecurityDescriptor
review BuildTrusteeWithName
review BuildTrusteeWithObjectsAndName
review BuildTrusteeWithObjectsAndSid
review BuildTrusteeWithSid
review GetAuditedPermissionsFromAcl
review GetEffectiveRightsFromAcl
review GetExplicitEntriesFromAcl
review GetTrusteeForm
review GetTrusteeName
review GetTrusteeType
review LookupSecurityDescriptorParts
review SetEntriesInAcl
review SetNamedSecurityInfo
review BuildImpersonateExplicitAccessWithName
review BuildImpersonateTrustee
review GetMultipleTrustee
review GetMultipleTrusteeOperation

file WINBASE.H

yes GetBinaryType
yes GetShortPathName
no GetLongPathName Win98/2K+ only
skip GetEnvironmentStrings misnamed ANSI version of the function
yes FreeEnvironmentStrings
yes FormatMessage
yes CreateMailslot
begin-bracket !defined (CYGWIN_HEADERS)
no EncryptFile Win2K+ only
no DecryptFile Win2K+ only
end-bracket
no OpenRaw error "The procedure entry point OpenRawW could not be located in the dynamic link library ADVAPI32.dll."
no QueryRecoveryAgents split-sized LPRECOVERY_AGENT_INFORMATION
yes lstrcmp
yes lstrcmpi
yes lstrcpyn
yes lstrcpy
yes lstrcat
yes lstrlen
yes CreateMutex
yes OpenMutex
yes CreateEvent
yes OpenEvent
yes CreateSemaphore
yes OpenSemaphore
yes CreateWaitableTimer
yes OpenWaitableTimer
yes CreateFileMapping
yes OpenFileMapping
yes GetLogicalDriveStrings
yes LoadLibrary
yes LoadLibraryEx
yes GetModuleFileName
yes GetModuleHandle
split CreateProcess LPSTARTUPINFO
yes FatalAppExit
split GetStartupInfo LPSTARTUPINFO
yes GetCommandLine
yes GetEnvironmentVariable
yes SetEnvironmentVariable
yes ExpandEnvironmentStrings
yes OutputDebugString
yes FindResource
yes FindResourceEx
skip EnumResourceTypes different prototypes in VC6 and VC7
skip EnumResourceNames different prototypes in VC6 and VC7
skip EnumResourceLanguages different prototypes in VC6 and VC7
yes BeginUpdateResource
yes UpdateResource
yes EndUpdateResource
yes GlobalAddAtom
yes GlobalFindAtom
yes GlobalGetAtomName
yes AddAtom
yes FindAtom
yes GetAtomName
yes GetProfileInt
yes GetProfileString
yes WriteProfileString
yes GetProfileSection
yes WriteProfileSection
yes GetPrivateProfileInt
yes GetPrivateProfileString
yes WritePrivateProfileString
yes GetPrivateProfileSection
yes WritePrivateProfileSection
yes GetPrivateProfileSectionNames
yes GetPrivateProfileStruct
yes WritePrivateProfileStruct
yes GetDriveType
yes GetSystemDirectory
yes GetTempPath
yes GetTempFileName
yes GetWindowsDirectory
yes SetCurrentDirectory
yes GetCurrentDirectory
yes GetDiskFreeSpace
yes GetDiskFreeSpaceEx
yes CreateDirectory
yes CreateDirectoryEx
yes RemoveDirectory
yes GetFullPathName
yes DefineDosDevice
yes QueryDosDevice
yes CreateFile
yes SetFileAttributes
yes GetFileAttributes
yes GetFileAttributesEx
no GetCompressedFileSize
yes DeleteFile
yes FindFirstFileEx
yes FindFirstFile
yes FindNextFile
yes SearchPath
yes CopyFile
yes CopyFileEx NT 4.0+ only
yes MoveFile
yes MoveFileEx
no MoveFileWithProgress NT 5.0+ only
no CreateHardLink NT 5.0+ only
yes CreateNamedPipe
yes GetNamedPipeHandleState
yes CallNamedPipe
yes WaitNamedPipe
yes SetVolumeLabel
yes GetVolumeInformation
yes ClearEventLog
yes BackupEventLog
yes OpenEventLog
yes RegisterEventSource
yes OpenBackupEventLog
yes ReadEventLog
yes ReportEvent
yes AccessCheckAndAuditAlarm
no AccessCheckByTypeAndAuditAlarm NT 5.0+ only
no AccessCheckByTypeResultListAndAuditAlarm NT 5.0+ only
yes ObjectOpenAuditAlarm
yes ObjectPrivilegeAuditAlarm
yes ObjectCloseAuditAlarm
yes PrivilegedServiceAuditAlarm
yes SetFileSecurity
yes GetFileSecurity
yes FindFirstChangeNotification
no ReadDirectoryChanges Unicode-only
yes IsBadStringPtr
yes LookupAccountSid
yes LookupAccountName
yes LookupPrivilegeValue
yes LookupPrivilegeName
yes LookupPrivilegeDisplayName
yes BuildCommDCB
yes BuildCommDCBAndTimeouts
yes CommConfigDialog
yes GetDefaultCommConfig
yes SetDefaultCommConfig
yes GetComputerName
yes SetComputerName
yes GetUserName
yes LogonUser
split CreateProcessAsUser LPSTARTUPINFO
no GetCurrentHwProfile split-sized LPHW_PROFILE_INFO; NT 4.0+ only
no GetVersionEx split-sized LPOSVERSIONINFO
no CreateJobObject NT 5.0+ only
no OpenJobObject NT 5.0+ only
review CheckNameLegalDOS8Dot3
review CreateActCtx
review CreateProcessWithLogon
review DeleteVolumeMountPoint
review DnsHostnameToComputerName
review FileEncryptionStatus
review FindActCtxSectionString
review FindFirstVolume
review FindFirstVolumeMountPoint
review FindNextVolume
review FindNextVolumeMountPoint
review GetFirmwareEnvironmentVariable
review GetComputerNameEx
review GetDllDirectory
review GetModuleHandleEx
review GetSystemWindowsDirectory
review GetSystemWow64Directory
review GetVolumeNameForVolumeMountPoint
review GetVolumePathName
review GetVolumePathNamesForVolumeName
review QueryActCtx
review ReplaceFile
review SetComputerNameEx
review SetDllDirectory
review SetFileShortName
review SetFirmwareEnvironmentVariable
review SetVolumeMountPoint
review VerifyVersionInfo

yes OpenMutex
yes OpenSemaphore
yes OpenWaitableTimer
yes CreateFileMapping
yes OpenFileMapping
yes GetStartupInfo
yes WaitNamedPipe
yes CreateNamedPipe
yes AccessCheckAndAuditAlarm
yes ObjectOpenAuditAlarm
yes ObjectPrivilegeAuditAlarm
yes ObjectCloseAuditAlarm
yes ObjectDeleteAuditAlarm
yes PrivilegedServiceAuditAlarm
yes SetFileSecurity
yes GetFileSecurity
yes CreateProcessAsUser

file WINUSER.H

skip MAKEINTRESOURCE macro
yes wvsprintf
no wsprintf varargs
yes LoadKeyboardLayout
yes GetKeyboardLayoutName
no CreateDesktop split-sized LPDEVMODE
yes OpenDesktop
split EnumDesktops DESKTOPENUMPROC // callback fun differs only in string pointer type
override HWINSTA CreateWindowStationW(LPWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES); error arg 1, VS6 prototype, missing const
yes OpenWindowStation
split EnumWindowStations WINSTAENUMPROC // callback fun differs only in string pointer type
yes GetUserObjectInformation
yes SetUserObjectInformation
yes RegisterWindowMessage
yes GetMessage
yes DispatchMessage
yes PeekMessage
yes SendMessage
no SendMessageTimeout VS6 has erroneous seventh parameter DWORD_PTR instead of PDWORD_PTR
yes SendNotifyMessage
yes SendMessageCallback
no BroadcastSystemMessage win95 version not split; NT 4.0+ only
no RegisterDeviceNotification NT 5.0+ only
yes PostMessage
yes PostThreadMessage
no PostAppMessage macro
skip DefWindowProc return value is conditionalized on _MAC, messes up parser
no CallWindowProc two versions, STRICT and non-STRICT
yes RegisterClass
yes UnregisterClass
split GetClassInfo LPWNDCLASS
yes RegisterClassEx
split GetClassInfoEx LPWNDCLASSEX NT 4.0+ only
yes CreateWindowEx
skip CreateWindow macro
yes CreateDialogParam
split CreateDialogIndirectParam LPCDLGTEMPLATE error in Cygwin prototype (no split) but fixable with typedef
no CreateDialog macro
no CreateDialogIndirect macro w/split LPCDLGTEMPLATE
yes DialogBoxParam
split DialogBoxIndirectParam LPCDLGTEMPLATE error in Cygwin prototype (no split) but fixable with typedef
no DialogBox macro
no DialogBoxIndirect macro w/split LPCDLGTEMPLATE
yes SetDlgItemText
yes GetDlgItemText
yes SendDlgItemMessage
no DefDlgProc return value is conditionalized on _MAC, messes up parser
begin-bracket !defined (CYGWIN_HEADERS)
yes CallMsgFilter
end-bracket
yes RegisterClipboardFormat
yes GetClipboardFormatName
yes CharToOem
yes OemToChar
yes CharToOemBuff
yes OemToCharBuff
yes CharUpper
yes CharUpperBuff
yes CharLower
yes CharLowerBuff
yes CharNext
yes CharPrev
no IsCharAlpha split CHAR
no IsCharAlphaNumeric split CHAR
no IsCharUpper split CHAR
no IsCharLower split CHAR
yes GetKeyNameText
skip VkKeyScan split CHAR
no VkKeyScanEx split CHAR; NT 4.0+ only
yes MapVirtualKey
yes MapVirtualKeyEx NT 4.0+ only
yes LoadAccelerators
yes CreateAcceleratorTable
yes CopyAcceleratorTable
yes TranslateAccelerator
yes LoadMenu
split LoadMenuIndirect MENUTEMPLATE
yes ChangeMenu
yes GetMenuString
yes InsertMenu
yes AppendMenu
yes ModifyMenu
split InsertMenuItem LPCMENUITEMINFO NT 4.0+ only
split GetMenuItemInfo LPMENUITEMINFO NT 4.0+ only
split SetMenuItemInfo LPCMENUITEMINFO NT 4.0+ only
yes DrawText
yes DrawTextEx NT 4.0+ only
yes GrayString
yes DrawState NT 4.0+ only
yes TabbedTextOut
yes GetTabbedTextExtent
yes SetProp
yes GetProp
yes RemoveProp
split EnumPropsEx PROPENUMPROCEX // callback fun differs only in string pointer type
split EnumProps PROPENUMPROC // callback fun differs only in string pointer type
yes SetWindowText
yes GetWindowText
yes GetWindowTextLength
yes MessageBox
yes MessageBoxEx
// split MessageBoxIndirect LPMSGBOXPARAMS NT 4.0+ only
no MessageBoxIndirect Cygwin has split MSGBOXPARAMS* instead of LPMSGBOXPARAMS
yes GetWindowLong
yes SetWindowLong
yes GetClassLong
yes SetClassLong
yes FindWindow
yes FindWindowEx NT 4.0+ only
yes GetClassName
no SetWindowsHook obsolete; two versions, STRICT and non-STRICT
yes SetWindowsHookEx
yes LoadBitmap
yes LoadCursor
yes LoadCursorFromFile
yes LoadIcon
yes LoadImage NT 4.0+ only
yes LoadString
yes IsDialogMessage
yes DlgDirList
yes DlgDirSelectEx
yes DlgDirListComboBox
yes DlgDirSelectComboBoxEx
yes DefFrameProc
yes DefMDIChildProc
override HWND CreateMDIWindowW(LPWSTR,LPWSTR,DWORD,int,int,int,int,HWND,HINSTANCE,LPARAM); error arg 1, VS6 prototype, missing const
yes WinHelp
no ChangeDisplaySettings split-sized LPDEVMODE
no ChangeDisplaySettingsEx split-sized LPDEVMODE; NT 5.0/Win98+ only
no EnumDisplaySettings split-sized LPDEVMODE
no EnumDisplayDevices split-sized PDISPLAY_DEVICE; NT 5.0+ only, no Win98
yes SystemParametersInfo probs w/ICONMETRICS, NONCLIENTMETRICS
no GetMonitorInfo NT 5.0/Win98+ only
no GetWindowModuleFileName NT 5.0+ only
no RealGetWindowClass NT 5.0+ only
no GetAltTabInfo NT 5.0+ only
review BroadcastSystemMessageEx
review EnumDisplaySettingsEx
skip GetClassLongPtr macro vs. function depending on arch, prefer function
review GetRawInputDeviceInfo
skip GetWindowLongPtr macro vs. function depending on arch, prefer function
skip SetClassLongPtr macro vs. function depending on arch, prefer function
skip SetWindowLongPtr macro vs. function depending on arch, prefer function

file WINGDI.H

begin-bracket defined (HAVE_MS_WINDOWS)
// split-sized LOGCOLORSPACE
// split-sized TEXTMETRIC
// split-sized NEWTEXTMETRIC
// split-sized NEWTEXTMETRICEX
// split-sized LOGFONT
// split-sized ENUMLOGFONT
// split-sized ENUMLOGFONTEX
// split-sized EXTLOGFONT, used in EMREXTCREATEFONTINDIRECTW (Unicode-only) and (???) in DEVINFO (DDK structure)
// split-sized DEVMODE
// split-sized DISPLAY_DEVICE, used in EnumDisplayDevices
// split-sized OUTLINETEXTMETRIC
// split-simple POLYTEXT
// split-simple GCP_RESULTS
// split-sized function pointer OLDFONTENUMPROC, same as FONTENUMPROC
// split-sized function pointer FONTENUMPROC
yes AddFontResource
yes CopyMetaFile
yes CreateDC
yes CreateFontIndirect
yes CreateFont
skip CreateIC split-sized DEVMODE
yes CreateMetaFile
yes CreateScalableFontResource
skip DeviceCapabilities split-sized DEVMODE
yes EnumFontFamiliesEx
no EnumFontFamilies split-complex FONTENUMPROC
no EnumFonts split-complex FONTENUMPROC
yes GetCharWidth
yes GetCharWidth32
yes GetCharWidthFloat
yes GetCharABCWidths
yes GetCharABCWidthsFloat
yes GetGlyphOutline
yes GetMetaFile
no GetOutlineTextMetrics split-sized LPOUTLINETEXTMETRIC
yes GetTextExtentPoint
yes GetTextExtentPoint32
yes GetTextExtentExPoint
split GetCharacterPlacement LPGCP_RESULTS NT 4.0+ only
no GetGlyphIndices NT 5.0+ only
no AddFontResourceEx NT 5.0+ only
no RemoveFontResourceEx NT 5.0+ only
// split-sized AXISINFO, used in AXESLIST; NT 5.0+ only
// split-sized AXESLIST, used in ENUMLOGFONTEXDV; NT 5.0+ only
// split-sized ENUMLOGFONTEXDV; NT 5.0+ only
skip CreateFontIndirectEx building problems with Visual Studio 8, unclear aetiology, possibly related to ENUMLOGFONTEXDV
// split-sized ENUMTEXTMETRIC, returned in EnumFontFamExProc, on NT 5.0+; NT 5.0+ only
yes ResetDC
yes RemoveFontResource
yes CopyEnhMetaFile
yes CreateEnhMetaFile
yes GetEnhMetaFile
yes GetEnhMetaFileDescription
yes GetTextMetrics
// split-simple DOCINFO
split StartDoc DOCINFO
yes GetObject
yes TextOut
yes ExtTextOut
split PolyTextOut POLYTEXT
yes GetTextFace
yes GetKerningPairs
// split-simple function pointer ICMENUMPROC
no GetLogColorSpace split-sized LPLOGCOLORSPACE; NT 4.0+ only
no CreateColorSpace split-sized LPLOGCOLORSPACE; NT 4.0+ only
yes GetICMProfile NT 4.0+ only, former error in Cygwin prototype but no more (Cygwin 1.7, 1-30-10)
yes SetICMProfile NT 4.0+ only
split EnumICMProfiles ICMENUMPROC NT 4.0+ only
no UpdateICMRegKey Deprecated, not used by our codebase
// non-split EMREXTTEXTOUT (A and W versions identical)
// non-split EMRPOLYTEXTOUT (A and W versions identical)
// Unicode-only EMREXTCREATEFONTINDIRECTW
no wglUseFontBitmaps causes link error
no wglUseFontOutlines causes link error
end-bracket

file WINSPOOL.H

begin-bracket defined (HAVE_MS_WINDOWS)
yes EnumPrinters #### problems with DEVMODE pointer in PRINTER_INFO_2
yes OpenPrinter
yes ResetPrinter
no SetJob split-sized DEVMODE pointer in split JOB_INFO_2
no GetJob split-sized DEVMODE pointer in split JOB_INFO_2
no EnumJobs split-sized DEVMODE pointer in split JOB_INFO_2
no AddPrinter split-sized DEVMODE pointer in split PRINTER_INFO_2
no SetPrinter split-sized DEVMODE pointer in split PRINTER_INFO_2
no GetPrinter split-sized DEVMODE pointer in split PRINTER_INFO_2
// other than DocumentProperties below, we don't use any of the others,
// and they all pretty much have complicated interfaces with lots of
// split structures, etc.
no AddPrinterDriver not used, complicated interface with split structures
no AddPrinterDriverEx not used, complicated interface with split structures
no EnumPrinterDrivers not used, complicated interface with split structures
no GetPrinterDriver not used, complicated interface with split structures
no GetPrinterDriverDirectory not used, complicated interface with split structures
no DeletePrinterDriver not used, complicated interface with split structures
no DeletePrinterDriverEx not used, complicated interface with split structures
no AddPerMachineConnection not used, complicated interface with split structures
no DeletePerMachineConnection not used, complicated interface with split structures
no EnumPerMachineConnections not used, complicated interface with split structures
no AddPrintProcessor not used, complicated interface with split structures
no EnumPrintProcessors not used, complicated interface with split structures
no GetPrintProcessorDirectory not used, complicated interface with split structures
no EnumPrintProcessorDatatypes not used, complicated interface with split structures
no DeletePrintProcessor not used, complicated interface with split structures
no StartDocPrinter not used, complicated interface with split structures
no AddJob not used, complicated interface with split structures
yes DocumentProperties
no AdvancedDocumentProperties not used, complicated interface with split structures
no GetPrinterData not used, complicated interface with split structures
no GetPrinterDataEx not used, complicated interface with split structures
no EnumPrinterData not used, complicated interface with split structures
no EnumPrinterDataEx not used, complicated interface with split structures
no EnumPrinterKey not used, complicated interface with split structures
no SetPrinterData not used, complicated interface with split structures
no SetPrinterDataEx not used, complicated interface with split structures
no DeletePrinterData not used, complicated interface with split structures
no DeletePrinterDataEx not used, complicated interface with split structures
no DeletePrinterKey not used, complicated interface with split structures
no PrinterMessageBox not used, complicated interface with split structures
no AddForm not used, complicated interface with split structures
no DeleteForm not used, complicated interface with split structures
no GetForm not used, complicated interface with split structures
no SetForm not used, complicated interface with split structures
no EnumForms not used, complicated interface with split structures
no EnumMonitors not used, complicated interface with split structures
no AddMonitor not used, complicated interface with split structures
no DeleteMonitor not used, complicated interface with split structures
no EnumPorts not used, complicated interface with split structures
no AddPort not used, complicated interface with split structures
no ConfigurePort not used, complicated interface with split structures
no DeletePort not used, complicated interface with split structures
no XcvData not used, complicated interface with split structures
no SetPort not used, complicated interface with split structures
no AddPrinterConnection not used, complicated interface with split structures
no DeletePrinterConnection not used, complicated interface with split structures
no AddPrintProvidor not used, complicated interface with split structures
no DeletePrintProvidor not used, complicated interface with split structures
no SetPrinterHTMLView not used, complicated interface with split structures
no GetPrinterHTMLView not used, complicated interface with split structures
review GetDefaultPrinter
end-bracket

file SHELLAPI.H

yes DragQueryFile
yes ShellExecute
yes FindExecutable
no CommandLineToArgv Unicode-only
yes ShellAbout
override HICON ExtractAssociatedIconW(HINSTANCE, LPWSTR, LPWORD); error arg2, Cygwin prototype, extra const
yes ExtractIcon
// split-simple DRAGINFO, used ??? (docs say "Not currently supported")
begin-bracket !defined (CYGWIN_HEADERS)
yes DoEnvironmentSubst NT 4.0+ only
end-bracket
no FindEnvironmentString causes link error; NT 4.0+ only
yes ExtractIconEx NT 4.0+ only, former error in Cygwin prototype but no more (Cygwin 1.7, 1-30-10)
// split-simple SHFILEOPSTRUCT, used in SHFileOperation
// split-simple SHNAMEMAPPING, used in SHFileOperation
split SHFileOperation LPSHFILEOPSTRUCT NT 4.0+ only
// split-simple SHELLEXECUTEINFO, used in ShellExecuteEx
split ShellExecuteEx LPSHELLEXECUTEINFO NT 4.0+ only
no WinExecError causes link error; NT 4.0+ only
begin-bracket !defined (CYGWIN_HEADERS)
yes SHQueryRecycleBin NT 4.0+ only
yes SHEmptyRecycleBin NT 4.0+ only
end-bracket
// split-sized NOTIFYICONDATA, used in Shell_NotifyIcon
no Shell_NotifyIcon split-sized NOTIFYICONDATA, NT 4.0+ only
// split-sized SHFILEINFO, used in SHGetFileInfo
skip SHGetFileInfo split-sized SHFILEINFO, NT 4.0+ only
no SHGetDiskFreeSpace causes link error; NT 4.0+ only
begin-bracket !defined (CYGWIN_HEADERS)
yes SHGetNewLinkInfo NT 4.0+ only
yes SHInvokePrinterCommand NT 4.0+ only
end-bracket

end-unicode-encapsulation-script

file COMMCTRL.H

yes ImageList_LoadImage
WC_HEADER
HDITEM
LPHDITEM
HDM_INSERTITEM
HDM_GETITEM
HDM_SETITEM
HDN_ITEMCHANGING
HDN_ITEMCHANGED
HDN_ITEMCLICK
HDN_ITEMDBLCLICK
HDN_DIVIDERDBLCLICK
HDN_BEGINTRACK
HDN_ENDTRACK
HDN_TRACK
HDN_GETDISPINFO
NMHEADER
LPNMHEADER
NMHDDISPINFO
LPNMHDDISPINFO
TOOLBARCLASSNAME
TBSAVEPARAMS
LPTBSAVEPARAMS
TB_GETBUTTONTEXT
TB_SAVERESTORE
TB_ADDSTRING
TBBUTTONINFO
LPTBBUTTONINFO
TB_GETBUTTONINFO
TB_SETBUTTONINFO
TB_INSERTBUTTON
TB_ADDBUTTONS
TBN_GETINFOTIP
NMTBGETINFOTIP
LPNMTBGETINFOTIP
TBN_GETDISPINFO
LPNMTBDISPINFO
TBN_GETBUTTONINFO
NMTOOLBAR
LPNMTOOLBAR
REBARCLASSNAME
REBARBANDINFO
LPREBARBANDINFO
LPCREBARBANDINFO
RB_INSERTBAND
RB_SETBANDINFO
RB_GETBANDINFO
TOOLTIPS_CLASS
TTTOOLINFO
PTOOLINFO
LPTTTOOLINFO
TTM_ADDTOOL
TTM_DELTOOL
TTM_NEWTOOLRECT
TTM_GETTOOLINFO
TTM_SETTOOLINFO
TTM_HITTEST
TTM_GETTEXT
TTM_UPDATETIPTEXT
TTM_ENUMTOOLS
TTM_GETCURRENTTOOL
TTHITTESTINFO
LPTTHITTESTINFO
TTN_GETDISPINFO
NMTTDISPINFO
LPNMTTDISPINFO
CreateStatusWindow
DrawStatusText
STATUSCLASSNAME
SB_GETTEXT
SB_SETTEXT
SB_GETTEXTLENGTH
SB_SETTIPTEXT
SB_GETTIPTEXT
TRACKBAR_CLASS
UPDOWN_CLASS
PROGRESS_CLASS
HOTKEY_CLASS
WC_LISTVIEW
LVITEM
LPLVITEM
LPSTR_TEXTCALLBACK
LVM_GETITEM
LVM_SETITEM
LVM_INSERTITEM
LVFINDINFO
LVM_FINDITEM
LVM_GETSTRINGWIDTH
LVM_EDITLABEL
LVCOLUMN
LPLVCOLUMN
LVM_GETCOLUMN
LVM_SETCOLUMN
LVM_GETITEMTEXT
LVM_SETITEMTEXT
LVM_GETISEARCHSTRING
LVBKIMAGE
LPLVBKIMAGE
LVM_SETBKIMAGE
LVM_GETBKIMAGE
LVN_ODFINDITEM
LVN_BEGINLABELEDIT
LVN_ENDLABELEDIT
LVN_GETDISPINFO
LVN_SETDISPINFO
NMLVDISPINFO
LVN_GETINFOTIP
NMLVGETINFOTIP
LPNMLVGETINFOTIP
WC_TREEVIEW
TVITEM
LPTVITEM
TVINSERTSTRUCT
LPTVINSERTSTRUCT
TVM_INSERTITEM
TVM_GETITEM
TVM_SETITEM
TVM_EDITLABEL
TVM_GETISEARCHSTRING
NMTREEVIEW
LPNMTREEVIEW
NMTVDISPINFO
LPNMTVDISPINFO
TVN_SELCHANGING
TVN_SELCHANGED
TVN_GETDISPINFO
TVN_SETDISPINFO
TVN_ITEMEXPANDING
TVN_ITEMEXPANDED
TVN_BEGINDRAG
TVN_BEGINRDRAG
TVN_DELETEITEM
TVN_BEGINLABELEDIT
TVN_ENDLABELEDIT
TVN_GETINFOTIP
NMTVGETINFOTIP
LPNMTVGETINFOTIP
WC_COMBOBOXEX
COMBOBOXEXITEM
PCOMBOBOXEXITEM
PCCOMBOBOXEXITEM
CBEM_INSERTITEM
CBEM_SETITEM
CBEM_GETITEM
NMCOMBOBOXEX
PNMCOMBOBOXEX
CBEN_GETDISPINFO
CBEN_DRAGBEGIN
CBEN_ENDEDIT
NMCBEDRAGBEGIN
LPNMCBEDRAGBEGIN
PNMCBEDRAGBEGIN
NMCBEENDEDIT
LPNMCBEENDEDIT
PNMCBEENDEDIT
WC_TABCONTROL
TCITEMHEADER
LPTCITEMHEADER
TCITEM
LPTCITEM
TCM_GETITEM
TCM_SETITEM
TCM_INSERTITEM
ANIMATE_CLASS
ACM_OPEN
MONTHCAL_CLASS
DATETIMEPICK_CLASS
DTM_SETFORMAT
DTN_USERSTRING
NMDATETIMESTRING
LPNMDATETIMESTRING
DTN_WMKEYDOWN
NMDATETIMEWMKEYDOWN
LPNMDATETIMEWMKEYDOWN
DTN_FORMAT
NMDATETIMEFORMAT
LPNMDATETIMEFORMAT
DTN_FORMATQUERY
NMDATETIMEFORMATQUERY
LPNMDATETIMEFORMATQUERY
WC_IPADDRESS
WC_PAGESCROLLER
WC_NATIVEFONTCTL

begin-unicode-encapsulation-script

file COMMDLG.H

begin-bracket defined (HAVE_MS_WINDOWS)
split GetOpenFileName LPOPENFILENAME
split GetSaveFileName LPOPENFILENAME
yes GetFileTitle
no CommDlg_OpenSave_GetSpec macro
no CommDlg_OpenSave_GetFilePath macro
no CommDlg_OpenSave_GetFolderPath macro
split ChooseColor LPCHOOSECOLOR
split FindText LPFINDREPLACE
split ReplaceText LPFINDREPLACE
no AfxReplaceText mac only
no ChooseFont split-sized LPLOGFONT in LPCHOOSEFONT
// LBSELCHSTRING
// SHAREVISTRING
// FILEOKSTRING
// COLOROKSTRING
// SETRGBSTRING
// HELPMSGSTRING
// FINDMSGSTRING
yes PrintDlg
yes PageSetupDlg
review PrintDlgEx
end-bracket

file DDE.H

// nothing

file DDEML.H

yes DdeInitialize
yes DdeCreateStringHandle former error in Cygwin prototype, but no more (Cygwin 1.7, 1-30-10)
yes DdeQueryString
// #### split-sized (or split-simple??? not completely obvious) structure MONHSZSTRUCT, used when DDE event MF_HSZ_INFO is sent as part of the XTYP_MONITOR transaction sent to a DDE callback; not yet handled

file IMM.H

begin-bracket defined (HAVE_MS_WINDOWS)
yes ImmInstallIME
yes ImmGetDescription
yes ImmGetIMEFileName
yes ImmGetCompositionString
skip ImmSetCompositionString different prototypes in VC6 and VC7
yes ImmGetCandidateListCount
yes ImmGetCandidateList
yes ImmGetGuideLine
yes ImmGetCompositionFont split-sized LOGFONT
yes ImmSetCompositionFont split-sized LOGFONT
yes ImmConfigureIME // split-simple REGISTERWORD
yes ImmEscape // strings of various sorts
yes ImmGetConversionList
yes ImmIsUIMessage
yes ImmRegisterWord
yes ImmUnregisterWord
no ImmGetRegisterWordStyle split-sized STYLEBUF
split ImmEnumRegisterWord REGISTERWORDENUMPROC
no ImmGetImeMenuItems split-sized IMEMENUITEMINFO
end-bracket

file MMSYSTEM.H

yes sndPlaySound
yes PlaySound
no waveOutGetDevCaps split-sized LPWAVEOUTCAPS
yes waveOutGetErrorText
no waveInGetDevCaps split-sized LPWAVEINCAPS
yes waveInGetErrorText
no midiOutGetDevCaps split-sized LPMIDIOUTCAPS
yes midiOutGetErrorText
no midiInGetDevCaps split-sized LPMIDIOUTCAPS
yes midiInGetErrorText
no auxGetDevCaps split-sized LPAUXCAPS
no mixerGetDevCaps split-sized LPMIXERCAPS
no mixerGetLineInfo split-sized LPMIXERLINE
no mixerGetLineControls split-sized LPMIXERCONTROL
no mixerGetControlDetails split-sized LPMIXERCONTROL in LPMIXERLINECONTROLS in LPMIXERCONTROLDETAILS
no joyGetDevCaps split-sized LPJOYCAPS
yes mmioStringToFOURCC
yes mmioInstallIOProc
yes mmioOpen
yes mmioRename
yes mciSendCommand
yes mciSendString
yes mciGetDeviceID
begin-bracket !defined (MINGW)
no mciGetDeviceIDFromElementID missing from Win98se version of ADVAPI32.dll
end-bracket
yes mciGetErrorString

file WINNETWK.H

begin-bracket defined (HAVE_MS_WINDOWS)
yes WNetAddConnection
split WNetAddConnection2 LPNETRESOURCE
split WNetAddConnection3 LPNETRESOURCE
yes WNetCancelConnection
yes WNetCancelConnection2
yes WNetGetConnection
split WNetUseConnection LPNETRESOURCE
split WNetConnectionDialog1 LPCONNECTDLGSTRUCT contains split-simple LPNETRESOURCE
split WNetDisconnectDialog1 LPDISCDLGSTRUCT
split WNetOpenEnum LPNETRESOURCE
yes WNetEnumResource
yes WNetGetUniversalName
yes WNetGetUser
yes WNetGetProviderName
yes WNetGetNetworkInformation
// split-simple function pointer PFNGETPROFILEPATH
// split-simple function pointer PFNRECONCILEPROFILE
// split-simple function pointer PFNPROCESSPOLICIES
yes WNetGetLastError
split MultinetGetConnectionPerformance LPNETRESOURCE
review WNetSetConnection
review WNetGetResourceInformation
review WNetGetResourceParent
end-bracket

// file IME.H -- doesn't exist under Cygwin

// no SendIMEMessageEx obsolete, no docs available

file OBJBASE.H

// nothing

file SHLOBJ.H

// #### split code for IContextMenu not yet written
// split flag constant GCS_VERB of IContextMenu::GetCommandString
// split flag constant GCS_HELPTEXT of IContextMenu::GetCommandString
// split flag constant GCS_VALIDATE of IContextMenu::GetCommandString
// split string constant CMDSTR_NEWFOLDER of CMINVOKECOMMANDINFO.lpVerb or CMINVOKECOMMANDINFOEX.lpVerbW of IContextMenu::InvokeCommand
// split string constant CMDSTR_VIEWLIST of same
// split string constant CMDSTR_VIEWDETAILS of same
// #### split code for IExtractIcon, IShellLink, IShellExecuteHook, INewShortcutHook, ICopyHook, IFileViewer not yet written
// split interface IExtractIcon
// split interface IShellLink
// split interface IShellExecuteHook
// split interface INewShortcutHook
// split interface ICopyHook
// split interface IFileViewer
skip SHGetPathFromIDList Cygwin qualifiers confuses parser
skip SHGetSpecialFolderPath error in Cygwin prototype, missing from Cygwin libraries
// split-simple structure BROWSEINFO used in SHBrowseForFolder
yes SHBrowseForFolder
// split message BFFM_SETSTATUSTEXT handled in qxeSendMessage
// split message BFFM_SETSELECTION handled in qxeSendMessage
// split message BFFM_VALIDATEFAILED handled in qxeSHBrowseForFolder intercept proc
// #### code to handle split clipboard formats not yet written.  this will
// #### be tricky -- all functions that use such clipboard formats need to
// #### be split, and the data itself munged.  this may be too much effort,
// #### and we may just need to require that the app itself does the
// #### splitting.
// split clipboard format CFSTR_FILEDESCRIPTOR
// split clipboard format CFSTR_FILENAME
// split clipboard format CFSTR_FILENAMEMAP
// split-sized structure FILEDESCRIPTOR
// split-sized structure FILEGROUPDESCRIPTOR
// split flag SHCNF_PATH; we intercept SHChangeNotify
// split flag SHCNF_PRINTER; we intercept SHChangeNotify
// split flag SHARD_PATH; we intercept SHAddToRecentDocs
review SHGetFolderPath
review SHGetIconOverlayIndex
review SHCreateDirectoryEx
review SHGetFolderPathAndSubDir

file WINNLS.H

no LOCALE_ENUMPROC not used, not examined yet
no CODEPAGE_ENUMPROC not used, not examined yet
no DATEFMT_ENUMPROC not used, not examined yet
no DATEFMT_ENUMPROCEX not used, not examined yet
no TIMEFMT_ENUMPROC not used, not examined yet
no CALINFO_ENUMPROC not used, not examined yet
no CALINFO_ENUMPROCEX not used, not examined yet
no GetCPInfoEx not used, not examined yet
no CompareString not used, not examined yet
no LCMapString not used, not examined yet
yes GetLocaleInfo
yes SetLocaleInfo
no GetTimeFormat not used, not examined yet
no GetDateFormat not used, not examined yet
no GetNumberFormat not used, not examined yet
no GetCurrencyFormat not used, not examined yet
no EnumCalendarInfo not used, not examined yet
no EnumCalendarInfoEx not used, not examined yet
no EnumTimeFormats not used, not examined yet
no EnumDateFormats not used, not examined yet
no EnumDateFormatsEx not used, not examined yet
no GetStringTypeEx not used, not examined yet
no GetStringType no such fun; A and W versions have different nos. of args
no FoldString not used, not examined yet
no EnumSystemLocales not used, not examined yet
no EnumSystemCodePages not used, not examined yet
review GetCalendarInfo
review GetGeoInfo
review SetCalendarInfo
review EnumSystemLanguageGroups
review EnumLanguageGroupLocales
review EnumUILanguages

end-unicode-encapsulation-script

file WINVER.H

VerFindFile
VerInstallFile
GetFileVersionInfoSize
GetFileVersionInfo
VerLanguageName
VerQueryValue

begin-unicode-encapsulation-script

file WINCON.H

yes PeekConsoleInput
yes ReadConsoleInput
yes WriteConsoleInput
yes ReadConsoleOutput
yes WriteConsoleOutput
yes ReadConsoleOutputCharacter
yes WriteConsoleOutputCharacter
no FillConsoleOutputCharacter split CHAR
yes ScrollConsoleScreenBuffer
yes GetConsoleTitle
yes SetConsoleTitle
skip ReadConsole last argument PCONSOLE_READCONSOLE_CONTROL pInputControl in recent VisualStudio, LPVOID in W32API
yes WriteConsole

file WINREG.H

yes RegConnectRegistry former error in Cygwin prototype, but no more (Cygwin 1.7, 1-30-10)
yes RegCreateKey
yes RegCreateKeyEx
yes RegDeleteKey
yes RegDeleteValue
yes RegEnumKey
yes RegEnumKeyEx
yes RegEnumValue
yes RegLoadKey
yes RegOpenKey
yes RegOpenKeyEx
yes RegQueryInfoKey
yes RegQueryValue
split RegQueryMultipleValues PVALENT
yes RegQueryValueEx
yes RegReplaceKey
yes RegRestoreKey
yes RegSaveKey
yes RegSetValue
yes RegSetValueEx
yes RegUnLoadKey
yes InitiateSystemShutdown
override BOOL AbortSystemShutdownW(LPWSTR); error arg 1, Cygwin prototype, extra const
review RegDeleteKeyEx

file EXCPT.H

// nothing

file STDARG.H

// nothing

file CDERR.H

// nothing

file WINPERF.H

// nothing

file RPC.H

// nothing

file NB30.H

// nothing

file FILEAPI.H
yes CreateFile
yes GetFileAttributes
yes DefineDosDevice
yes FindFirstChangeNotification
yes GetDiskFreeSpace
yes GetDriveType
yes GetFileAttributes
yes GetFullPathName
yes GetLogicalDriveStrings
yes GetShortPathName
yes QueryDosDevice
yes GetTempFileName
yes GetVolumeInformation
yes CreateDirectory
yes DeleteFile
yes GetDiskFreeSpaceEx
yes GetFileAttributes
yes GetFileAttributesEx
yes RemoveDirectory
yes SetFileAttributes
yes GetTempPath

file debugapi.h
yes OutputDebugString

file memoryapi.h
yes CreateFileMapping
yes OpenFileMapping

file namedpipeapi.h
yes CreateNamedPipe
yes WaitNamedPipe

file processenv.h
yes GetCommandLine
yes GetCurrentDirectory
yes SearchPath
yes SetCurrentDirectory
yes ExpandEnvironmentStrings
yes FreeEnvironmentStrings
yes GetEnvironmentVariable
yes SetEnvironmentVariable

file synchapi.h
yes OpenMutex
yes OpenSemaphore
yes OpenEvent
yes CreateMutex
yes CreateEvent
yes OpenWaitableTimer

file sysinfoapi.h
yes GetSystemDirectory
yes GetWindowsDirectory

file securitybaseapi.h
yes AccessCheckAndAuditAlarm
yes GetFileSecurity
yes ObjectCloseAuditAlarm
yes ObjectDeleteAuditAlarm
yes ObjectOpenAuditAlarm
yes ObjectPrivilegeAuditAlarm
yes PrivilegedServiceAuditAlarm
yes SetFileSecurity

file processthreadsapi.h
yes GetStartupInfo
yes CreateProcessAsUser
yes CreateProcess

file LIBLOADERAPI.H
yes LoadLibrary
yes LoadLibraryEx
yes FindResourceEx
yes GetModuleHandle
yes GetModuleFileName

end-unicode-encapsulation-script

file WINSOCK2.H

SO_PROTOCOL_INFO
SERVICE_TYPE_VALUE_SAPID
SERVICE_TYPE_VALUE_TCPPORT
SERVICE_TYPE_VALUE_UDPPORT
SERVICE_TYPE_VALUE_OBJECTID
WSADuplicateSocket
LPFN_WSADUPLICATESOCKET
WSAEnumProtocols
LPFN_WSAENUMPROTOCOLS
WSASocket
LPFN_WSASOCKET
WSAAddressToString
LPFN_WSAADDRESSTOSTRING
WSAStringToAddress
LPFN_WSASTRINGTOADDRESS
WSALookupServiceBegin
LPFN_WSALOOKUPSERVICEBEGIN
WSALookupServiceNext
LPFN_WSALOOKUPSERVICENEXT
WSAInstallServiceClass
LPFN_WSAINSTALLSERVICECLASS
WSAGetServiceClassInfo
LPFN_WSAGETSERVICECLASSINFO
WSAEnumNameSpaceProviders
LPFN_WSAENUMNAMESPACEPROVIDERS
WSAGetServiceClassNameByClassId
LPFN_WSAGETSERVICECLASSNAMEBYCLASSID
WSASetService
LPFN_WSASETSERVICE

file WINCRYPT.H

MS_DEF_PROV_
MS_ENHANCED_PROV_
MS_DEF_RSA_SIG_PROV_
MS_DEF_RSA_SCHANNEL_PROV_
MS_ENHANCED_RSA_SCHANNEL_PROV_
MS_DEF_DSS_PROV_
MS_DEF_DSS_DH_PROV_
CryptAcquireContext
CryptSignHash
CryptVerifySignature
CryptSetProvider
CryptSetProviderEx
CryptGetDefaultProvider
CryptEnumProviderTypes
CryptEnumProviders
CERT_STORE_PROV_FILENAME_
CERT_STORE_PROV_SYSTEM_
sz_CERT_STORE_PROV_FILENAME_
sz_CERT_STORE_PROV_SYSTEM_
CERT_STORE_SAVE_TO_FILENAME_
CERT_FIND_SUBJECT_STR_
CERT_FIND_ISSUER_STR_
CertRDNValueToStr
CertNameToStr
CertStrToName
CertOpenSystemStore
CertAddEncodedCertificateToSystemStore

*/

/* the functions below are examples of hand-written Unicode-splitting
   code.  note that it needs to be written very carefully and with
   intimate knowledge of the structures involved, and can sometimes be
   very hairy (EnumFontFamiliesEx is the most extreme example).  it can
   be argued with some justification that this behind-the-scenes magic
   is confusing and potentially dangerous, and shouldn't be done.  but
   making the calling code deal with the results in extremely hard-to-
   read code and is very error-prone. */


/************************************************************************/
/*        would be encapsulatable but for parsing problems              */
/************************************************************************/

/* NOTE: return value is conditionalized on _MAC, messes up parser */
LRESULT
qxeDefWindowProc (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProcW (hWnd, Msg, wParam, lParam);
}


/* NOTE: two versions, STRICT and non-STRICT */
LRESULT
qxeCallWindowProc (WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return CallWindowProcW (lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

/* NOTE: return value is conditionalized on _MAC, messes up parser */
LRESULT
qxeDefDlgProc (HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return DefDlgProcW (hDlg, Msg, wParam, lParam);
}
/* This one has two entry points called GetEnvironmentStringsW and
   GetEnvironmentStrings. (misnamed A version) */
Extbyte *
qxeGetEnvironmentStrings (void)
{
  return (Extbyte *) GetEnvironmentStringsW ();
}


/********************************************************************************/
/* would be encapsulatable but for header changes in different versions of VC++ */
/********************************************************************************/

#if MSC_VERSION >= 1300

BOOL
qxeEnumResourceTypes (HMODULE hModule, ENUMRESTYPEPROCW lpEnumFunc, LONG lParam)
{
  return EnumResourceTypesW (hModule, lpEnumFunc, lParam);
}

BOOL
qxeEnumResourceNames (HMODULE hModule, const Extbyte * lpType, ENUMRESNAMEPROCW lpEnumFunc, LONG lParam)
{
  return EnumResourceNamesW (hModule, (LPCWSTR) lpType, lpEnumFunc, lParam);
}

BOOL
qxeEnumResourceLanguages (HMODULE hModule, const Extbyte * lpType, const Extbyte * lpName, ENUMRESLANGPROCW lpEnumFunc, LONG lParam)
{
  return EnumResourceLanguagesW (hModule, (LPCWSTR) lpType, (LPCWSTR) lpName, lpEnumFunc, lParam);
}

#else

BOOL
qxeEnumResourceTypes (HMODULE hModule, ENUMRESTYPEPROC lpEnumFunc, LONG lParam)
{
  return EnumResourceTypesW (hModule, (ENUMRESTYPEPROCW) lpEnumFunc, lParam);
}

BOOL
qxeEnumResourceNames (HMODULE hModule, const Extbyte * lpType, ENUMRESNAMEPROC lpEnumFunc, LONG lParam)
{
  return EnumResourceNamesW (hModule, (LPCWSTR) lpType, (ENUMRESNAMEPROCW) lpEnumFunc, lParam);
}

BOOL
qxeEnumResourceLanguages (HMODULE hModule, const Extbyte * lpType, const Extbyte * lpName, ENUMRESLANGPROC lpEnumFunc, LONG lParam)
{
  return EnumResourceLanguagesW (hModule, (LPCWSTR) lpType, (LPCWSTR) lpName, (ENUMRESLANGPROCW) lpEnumFunc, lParam);
}

#endif /* MSC_VERSION >= 1300 */

/************************************************************************/
/*                                files                                 */
/************************************************************************/

HANDLE
qxeFindFirstFile (const Extbyte *lpFileName,
		  WIN32_FIND_DATAW *lpFindFileData)
{
  return FindFirstFileW ((LPCWSTR) lpFileName, lpFindFileData);
}

BOOL
qxeFindNextFile (HANDLE hFindFile, WIN32_FIND_DATAW *lpFindFileData)
{
  return FindNextFileW (hFindFile, lpFindFileData);
}


/************************************************************************/
/*                                shell                                 */
/************************************************************************/

DWORD
qxeSHGetFileInfo (const Extbyte *pszPath, DWORD dwFileAttributes,
		  SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags)
{
  return SHGetFileInfoW ((LPCWSTR) pszPath, dwFileAttributes,
			 psfi, cbFileInfo, uFlags);
}

VOID
qxeSHAddToRecentDocs (UINT uFlags, LPCVOID pv)
{
  /* pv can be a string pointer; this is handled by Unicode-splitting the
     flag SHARD_PATH rather than the function itself.  Fix up the flag to
     be correct.  We write it symmetrically so it doesn't matter whether
     UNICODE is defined. */
  if (uFlags & SHARD_PATHA)
    {
      uFlags |= SHARD_PATHW;
      uFlags &= ~SHARD_PATHA;
    }
  SHAddToRecentDocs (uFlags, pv);
}

VOID
qxeSHChangeNotify (LONG wEventId, UINT uFlags, LPCVOID dwItem1,
		   LPCVOID dwItem2)
{
  /* works like SHAddToRecentDocs */
  if (uFlags & SHCNF_PATHA)
    {
      uFlags |= SHCNF_PATHW;
      uFlags &= ~SHCNF_PATHA;
    }
  if (uFlags & SHCNF_PRINTERA)
    {
      uFlags |= SHCNF_PRINTERW;
      uFlags &= ~SHCNF_PRINTERA;
    }
  SHChangeNotify (wEventId, uFlags, dwItem1, dwItem2);
}

BOOL
qxeSHGetPathFromIDList (LPCITEMIDLIST pidl, Extbyte *pszPath)
{
  return SHGetPathFromIDListW (pidl, (LPWSTR) pszPath);
}

HRESULT
qxeSHGetDataFromIDList (IShellFolder *psf, LPCITEMIDLIST pidl, int nFormat,
			PVOID pv, int cb)
{
  return SHGetDataFromIDListW (psf, pidl, nFormat, pv, cb);
}

/* end of intl-encap-win32.c */
