/* Automatically-generated Unicode-encapsulation file,
   using the command

   ../lib-src/make-mswin-unicode.pl --h-output intl-auto-encap-win32.h intl-encap-win32.c

   Do not edit.  See `make-mswin-unicode.pl'.
*/


/* Processing file WINSPOOL.H */


/*----------------------------------------------------------------------*/
/*                      Processing file WINSPOOL.H                      */
/*----------------------------------------------------------------------*/

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EnumPrinters
#define EnumPrinters error_use_qxeEnumPrinters_or_EnumPrintersA_and_EnumPrintersW
#endif
/* NOTE: #### problems with DEVMODE pointer in PRINTER_INFO_2 */
DECLARE_INLINE_HEADER (
BOOL qxeEnumPrinters (DWORD Flags, Extbyte * Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
)
{
  return EnumPrintersW (Flags, (LPWSTR) Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenPrinter
#define OpenPrinter error_use_qxeOpenPrinter_or_OpenPrinterA_and_OpenPrinterW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeOpenPrinter (Extbyte * pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault)
)
{
  return OpenPrinterW ((LPWSTR) pPrinterName, phPrinter, pDefault);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ResetPrinter
#define ResetPrinter error_use_qxeResetPrinter_or_ResetPrinterA_and_ResetPrinterW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeResetPrinter (HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
)
{
  return ResetPrinterW (hPrinter, pDefault);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef SetJob
#define SetJob error_split_sized_DEVMODE_pointer_in_split_JOB_INFO_2
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetJob
#define GetJob error_split_sized_DEVMODE_pointer_in_split_JOB_INFO_2
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumJobs
#define EnumJobs error_split_sized_DEVMODE_pointer_in_split_JOB_INFO_2
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddPrinter
#define AddPrinter error_split_sized_DEVMODE_pointer_in_split_PRINTER_INFO_2
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef SetPrinter
#define SetPrinter error_split_sized_DEVMODE_pointer_in_split_PRINTER_INFO_2
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetPrinter
#define GetPrinter error_split_sized_DEVMODE_pointer_in_split_PRINTER_INFO_2
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddPrinterDriver
#define AddPrinterDriver error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddPrinterDriverEx
#define AddPrinterDriverEx error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumPrinterDrivers
#define EnumPrinterDrivers error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetPrinterDriver
#define GetPrinterDriver error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetPrinterDriverDirectory
#define GetPrinterDriverDirectory error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrinterDriver
#define DeletePrinterDriver error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrinterDriverEx
#define DeletePrinterDriverEx error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddPrintProcessor
#define AddPrintProcessor error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumPrintProcessors
#define EnumPrintProcessors error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetPrintProcessorDirectory
#define GetPrintProcessorDirectory error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumPrintProcessorDatatypes
#define EnumPrintProcessorDatatypes error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrintProcessor
#define DeletePrintProcessor error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef StartDocPrinter
#define StartDocPrinter error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddJob
#define AddJob error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DocumentProperties
#define DocumentProperties error_use_qxeDocumentProperties_or_DocumentPropertiesA_and_DocumentPropertiesW
#endif
DECLARE_INLINE_HEADER (
LONG qxeDocumentProperties (HWND hWnd, HANDLE hPrinter, Extbyte * pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput, DWORD fMode)
)
{
  return DocumentPropertiesW (hWnd, hPrinter, (LPWSTR) pDeviceName, pDevModeOutput, pDevModeInput, fMode);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AdvancedDocumentProperties
#define AdvancedDocumentProperties error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetPrinterData
#define GetPrinterData error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetPrinterDataEx
#define GetPrinterDataEx error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumPrinterData
#define EnumPrinterData error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumPrinterDataEx
#define EnumPrinterDataEx error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumPrinterKey
#define EnumPrinterKey error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef SetPrinterData
#define SetPrinterData error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef SetPrinterDataEx
#define SetPrinterDataEx error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrinterData
#define DeletePrinterData error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrinterDataEx
#define DeletePrinterDataEx error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrinterKey
#define DeletePrinterKey error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef PrinterMessageBox
#define PrinterMessageBox error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddForm
#define AddForm error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeleteForm
#define DeleteForm error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetForm
#define GetForm error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef SetForm
#define SetForm error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumForms
#define EnumForms error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumMonitors
#define EnumMonitors error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddMonitor
#define AddMonitor error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeleteMonitor
#define DeleteMonitor error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumPorts
#define EnumPorts error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddPort
#define AddPort error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef ConfigurePort
#define ConfigurePort error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePort
#define DeletePort error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef XcvData
#define XcvData error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetDefaultPrinter
#define GetDefaultPrinter error_Function_needs_review_to_determine_how_to_handle_it
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef SetPort
#define SetPort error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddPrinterConnection
#define AddPrinterConnection error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrinterConnection
#define DeletePrinterConnection error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddPrintProvidor
#define AddPrintProvidor error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef DeletePrintProvidor
#define DeletePrintProvidor error_not_used__complicated_interface_with_split_structures
#endif /* defined (HAVE_MS_WINDOWS) */


/* Processing file WINGDI.H */


/*----------------------------------------------------------------------*/
/*                       Processing file WINGDI.H                       */
/*----------------------------------------------------------------------*/

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef AddFontResource
#define AddFontResource error_use_qxeAddFontResource_or_AddFontResourceA_and_AddFontResourceW
#endif
DECLARE_INLINE_HEADER (
int qxeAddFontResource (const Extbyte * arg1)
)
{
  return AddFontResourceW ((LPCWSTR) arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CopyMetaFile
#define CopyMetaFile error_use_qxeCopyMetaFile_or_CopyMetaFileA_and_CopyMetaFileW
#endif
DECLARE_INLINE_HEADER (
HMETAFILE qxeCopyMetaFile (HMETAFILE arg1, const Extbyte * arg2)
)
{
  return CopyMetaFileW (arg1, (LPCWSTR) arg2);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateDC
#define CreateDC error_use_qxeCreateDC_or_CreateDCA_and_CreateDCW
#endif
DECLARE_INLINE_HEADER (
HDC qxeCreateDC (const Extbyte * pwszDriver, const Extbyte * pwszDevice, const Extbyte * pszPort, const DEVMODEW * pdm)
)
{
  return CreateDCW ((LPCWSTR) pwszDriver, (LPCWSTR) pwszDevice, (LPCWSTR) pszPort, pdm);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateFontIndirect
#define CreateFontIndirect error_use_qxeCreateFontIndirect_or_CreateFontIndirectA_and_CreateFontIndirectW
#endif
DECLARE_INLINE_HEADER (
HFONT qxeCreateFontIndirect (const LOGFONTW * lplf)
)
{
  return CreateFontIndirectW (lplf);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateFont
#define CreateFont error_use_qxeCreateFont_or_CreateFontA_and_CreateFontW
#endif
DECLARE_INLINE_HEADER (
HFONT qxeCreateFont (int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily, const Extbyte * pszFaceName)
)
{
  return CreateFontW (cHeight, cWidth, cEscapement, cOrientation, cWeight, bItalic, bUnderline, bStrikeOut, iCharSet, iOutPrecision, iClipPrecision, iQuality, iPitchAndFamily, (LPCWSTR) pszFaceName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
/* Skipping CreateIC because split-sized DEVMODE */
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateMetaFile
#define CreateMetaFile error_use_qxeCreateMetaFile_or_CreateMetaFileA_and_CreateMetaFileW
#endif
DECLARE_INLINE_HEADER (
HDC qxeCreateMetaFile (const Extbyte * pszFile)
)
{
  return CreateMetaFileW ((LPCWSTR) pszFile);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateScalableFontResource
#define CreateScalableFontResource error_use_qxeCreateScalableFontResource_or_CreateScalableFontResourceA_and_CreateScalableFontResourceW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCreateScalableFontResource (DWORD fdwHidden, const Extbyte * lpszFont, const Extbyte * lpszFile, const Extbyte * lpszPath)
)
{
  return CreateScalableFontResourceW (fdwHidden, (LPCWSTR) lpszFont, (LPCWSTR) lpszFile, (LPCWSTR) lpszPath);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
/* Skipping DeviceCapabilities because split-sized DEVMODE */
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EnumFontFamiliesEx
#define EnumFontFamiliesEx error_use_qxeEnumFontFamiliesEx_or_EnumFontFamiliesExA_and_EnumFontFamiliesExW
#endif
DECLARE_INLINE_HEADER (
int qxeEnumFontFamiliesEx (HDC hdc, LPLOGFONTW lpLogfont, FONTENUMPROCW lpProc, LPARAM lParam, DWORD dwFlags)
)
{
  return EnumFontFamiliesExW (hdc, lpLogfont, lpProc, lParam, dwFlags);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumFontFamilies
#define EnumFontFamilies error_split_complex_FONTENUMPROC
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef EnumFonts
#define EnumFonts error_split_complex_FONTENUMPROC
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCharWidth
#define GetCharWidth error_use_qxeGetCharWidth_or_GetCharWidthA_and_GetCharWidthW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetCharWidth (HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer)
)
{
  return GetCharWidthW (hdc, iFirst, iLast, lpBuffer);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCharWidth32
#define GetCharWidth32 error_use_qxeGetCharWidth32_or_GetCharWidth32A_and_GetCharWidth32W
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetCharWidth32 (HDC hdc, UINT iFirst, UINT iLast, LPINT lpBuffer)
)
{
  return GetCharWidth32W (hdc, iFirst, iLast, lpBuffer);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCharWidthFloat
#define GetCharWidthFloat error_use_qxeGetCharWidthFloat_or_GetCharWidthFloatA_and_GetCharWidthFloatW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetCharWidthFloat (HDC hdc, UINT iFirst, UINT iLast, PFLOAT lpBuffer)
)
{
  return GetCharWidthFloatW (hdc, iFirst, iLast, lpBuffer);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCharABCWidths
#define GetCharABCWidths error_use_qxeGetCharABCWidths_or_GetCharABCWidthsA_and_GetCharABCWidthsW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetCharABCWidths (HDC hdc, UINT wFirst, UINT wLast, LPABC lpABC)
)
{
  return GetCharABCWidthsW (hdc, wFirst, wLast, lpABC);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCharABCWidthsFloat
#define GetCharABCWidthsFloat error_use_qxeGetCharABCWidthsFloat_or_GetCharABCWidthsFloatA_and_GetCharABCWidthsFloatW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetCharABCWidthsFloat (HDC hdc, UINT iFirst, UINT iLast, LPABCFLOAT lpABC)
)
{
  return GetCharABCWidthsFloatW (hdc, iFirst, iLast, lpABC);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetGlyphOutline
#define GetGlyphOutline error_use_qxeGetGlyphOutline_or_GetGlyphOutlineA_and_GetGlyphOutlineW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetGlyphOutline (HDC hdc, UINT uChar, UINT fuFormat, LPGLYPHMETRICS lpgm, DWORD cjBuffer, LPVOID pvBuffer, const MAT2 * lpmat2)
)
{
  return GetGlyphOutlineW (hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetMetaFile
#define GetMetaFile error_use_qxeGetMetaFile_or_GetMetaFileA_and_GetMetaFileW
#endif
DECLARE_INLINE_HEADER (
HMETAFILE qxeGetMetaFile (const Extbyte * lpName)
)
{
  return GetMetaFileW ((LPCWSTR) lpName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetOutlineTextMetrics
#define GetOutlineTextMetrics error_split_sized_LPOUTLINETEXTMETRIC
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTextExtentPoint
#define GetTextExtentPoint error_use_qxeGetTextExtentPoint_or_GetTextExtentPointA_and_GetTextExtentPointW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetTextExtentPoint (HDC hdc, const Extbyte * lpString, int c, LPSIZE lpsz)
)
{
  return GetTextExtentPointW (hdc, (LPCWSTR) lpString, c, lpsz);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTextExtentPoint32
#define GetTextExtentPoint32 error_use_qxeGetTextExtentPoint32_or_GetTextExtentPoint32A_and_GetTextExtentPoint32W
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetTextExtentPoint32 (HDC hdc, const Extbyte * lpString, int c, LPSIZE psizl)
)
{
  return GetTextExtentPoint32W (hdc, (LPCWSTR) lpString, c, psizl);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTextExtentExPoint
#define GetTextExtentExPoint error_use_qxeGetTextExtentExPoint_or_GetTextExtentExPointA_and_GetTextExtentExPointW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetTextExtentExPoint (HDC hdc, const Extbyte * lpszString, int cchString, int nMaxExtent, LPINT lpnFit, LPINT lpnDx, LPSIZE lpSize)
)
{
  return GetTextExtentExPointW (hdc, (LPCWSTR) lpszString, cchString, nMaxExtent, lpnFit, lpnDx, lpSize);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCharacterPlacement
#define GetCharacterPlacement error_use_qxeGetCharacterPlacement_or_GetCharacterPlacementA_and_GetCharacterPlacementW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
DWORD qxeGetCharacterPlacement (HDC hdc, const Extbyte * lpString, int nCount, int nMexExtent, LPGCP_RESULTSW lpResults, DWORD dwFlags)
)
{
  return GetCharacterPlacementW (hdc, (LPCWSTR) lpString, nCount, nMexExtent, lpResults, dwFlags);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetGlyphIndices
#define GetGlyphIndices error_NT_5_0__only
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef AddFontResourceEx
#define AddFontResourceEx error_NT_5_0__only
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef RemoveFontResourceEx
#define RemoveFontResourceEx error_NT_5_0__only
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
/* Skipping CreateFontIndirectEx because building problems with Visual Studio 8, unclear aetiology, possibly related to ENUMLOGFONTEXDV */
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ResetDC
#define ResetDC error_use_qxeResetDC_or_ResetDCA_and_ResetDCW
#endif
DECLARE_INLINE_HEADER (
HDC qxeResetDC (HDC hdc, const DEVMODEW * lpdm)
)
{
  return ResetDCW (hdc, lpdm);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RemoveFontResource
#define RemoveFontResource error_use_qxeRemoveFontResource_or_RemoveFontResourceA_and_RemoveFontResourceW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeRemoveFontResource (const Extbyte * lpFileName)
)
{
  return RemoveFontResourceW ((LPCWSTR) lpFileName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CopyEnhMetaFile
#define CopyEnhMetaFile error_use_qxeCopyEnhMetaFile_or_CopyEnhMetaFileA_and_CopyEnhMetaFileW
#endif
DECLARE_INLINE_HEADER (
HENHMETAFILE qxeCopyEnhMetaFile (HENHMETAFILE hEnh, const Extbyte * lpFileName)
)
{
  return CopyEnhMetaFileW (hEnh, (LPCWSTR) lpFileName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateEnhMetaFile
#define CreateEnhMetaFile error_use_qxeCreateEnhMetaFile_or_CreateEnhMetaFileA_and_CreateEnhMetaFileW
#endif
DECLARE_INLINE_HEADER (
HDC qxeCreateEnhMetaFile (HDC hdc, const Extbyte * lpFilename, const RECT * lprc, const Extbyte * lpDesc)
)
{
  return CreateEnhMetaFileW (hdc, (LPCWSTR) lpFilename, lprc, (LPCWSTR) lpDesc);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetEnhMetaFile
#define GetEnhMetaFile error_use_qxeGetEnhMetaFile_or_GetEnhMetaFileA_and_GetEnhMetaFileW
#endif
DECLARE_INLINE_HEADER (
HENHMETAFILE qxeGetEnhMetaFile (const Extbyte * lpName)
)
{
  return GetEnhMetaFileW ((LPCWSTR) lpName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetEnhMetaFileDescription
#define GetEnhMetaFileDescription error_use_qxeGetEnhMetaFileDescription_or_GetEnhMetaFileDescriptionA_and_GetEnhMetaFileDescriptionW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetEnhMetaFileDescription (HENHMETAFILE hemf, UINT cchBuffer, Extbyte * lpDescription)
)
{
  return GetEnhMetaFileDescriptionW (hemf, cchBuffer, (LPWSTR) lpDescription);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTextMetrics
#define GetTextMetrics error_use_qxeGetTextMetrics_or_GetTextMetricsA_and_GetTextMetricsW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetTextMetrics (HDC hdc, LPTEXTMETRICW lptm)
)
{
  return GetTextMetricsW (hdc, lptm);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef StartDoc
#define StartDoc error_use_qxeStartDoc_or_StartDocA_and_StartDocW
#endif
DECLARE_INLINE_HEADER (
int qxeStartDoc (HDC hdc, const DOCINFOW * lpdi)
)
{
  return StartDocW (hdc, lpdi);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetObject
#define GetObject error_use_qxeGetObject_or_GetObjectA_and_GetObjectW
#endif
DECLARE_INLINE_HEADER (
int qxeGetObject (HANDLE h, int c, LPVOID pv)
)
{
  return GetObjectW (h, c, pv);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef TextOut
#define TextOut error_use_qxeTextOut_or_TextOutA_and_TextOutW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeTextOut (HDC hdc, int x, int y, const Extbyte * lpString, int c)
)
{
  return TextOutW (hdc, x, y, (LPCWSTR) lpString, c);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ExtTextOut
#define ExtTextOut error_use_qxeExtTextOut_or_ExtTextOutA_and_ExtTextOutW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeExtTextOut (HDC hdc, int x, int y, UINT options, const RECT * lprect, const Extbyte * lpString, UINT c, const INT * lpDx)
)
{
  return ExtTextOutW (hdc, x, y, options, lprect, (LPCWSTR) lpString, c, lpDx);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PolyTextOut
#define PolyTextOut error_use_qxePolyTextOut_or_PolyTextOutA_and_PolyTextOutW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePolyTextOut (HDC hdc, const POLYTEXTW * ppt, int nstrings)
)
{
  return PolyTextOutW (hdc, ppt, nstrings);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTextFace
#define GetTextFace error_use_qxeGetTextFace_or_GetTextFaceA_and_GetTextFaceW
#endif
DECLARE_INLINE_HEADER (
int qxeGetTextFace (HDC hdc, int c, Extbyte * lpName)
)
{
  return GetTextFaceW (hdc, c, (LPWSTR) lpName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetKerningPairs
#define GetKerningPairs error_use_qxeGetKerningPairs_or_GetKerningPairsA_and_GetKerningPairsW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetKerningPairs (HDC hdc, DWORD nPairs, LPKERNINGPAIR lpKernPair)
)
{
  return GetKerningPairsW (hdc, nPairs, lpKernPair);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef GetLogColorSpace
#define GetLogColorSpace error_split_sized_LPLOGCOLORSPACE__NT_4_0__only
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef CreateColorSpace
#define CreateColorSpace error_split_sized_LPLOGCOLORSPACE__NT_4_0__only
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetICMProfile
#define GetICMProfile error_use_qxeGetICMProfile_or_GetICMProfileA_and_GetICMProfileW
#endif
/* NOTE: NT 4.0+ only, former error in Cygwin prototype but no more (Cygwin 1.7, 1-30-10) */
DECLARE_INLINE_HEADER (
BOOL qxeGetICMProfile (HDC hdc, LPDWORD pBufSize, Extbyte * pszFilename)
)
{
  return GetICMProfileW (hdc, pBufSize, (LPWSTR) pszFilename);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetICMProfile
#define SetICMProfile error_use_qxeSetICMProfile_or_SetICMProfileA_and_SetICMProfileW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeSetICMProfile (HDC hdc, Extbyte * lpFileName)
)
{
  return SetICMProfileW (hdc, (LPWSTR) lpFileName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EnumICMProfiles
#define EnumICMProfiles error_use_qxeEnumICMProfiles_or_EnumICMProfilesA_and_EnumICMProfilesW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
int qxeEnumICMProfiles (HDC hdc, ICMENUMPROCW lpProc, LPARAM lParam)
)
{
  return EnumICMProfilesW (hdc, lpProc, lParam);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef UpdateICMRegKey
#define UpdateICMRegKey error_Deprecated__not_used_by_our_codebase
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef wglUseFontBitmaps
#define wglUseFontBitmaps error_causes_link_error
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef wglUseFontOutlines
#define wglUseFontOutlines error_causes_link_error
#endif /* defined (HAVE_MS_WINDOWS) */


/* Processing file MMSYSTEM.H */


/*----------------------------------------------------------------------*/
/*                      Processing file MMSYSTEM.H                      */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef sndPlaySound
#define sndPlaySound error_use_qxesndPlaySound_or_sndPlaySoundA_and_sndPlaySoundW
#endif
DECLARE_INLINE_HEADER (
BOOL qxesndPlaySound (const Extbyte * pszSound, UINT fuSound)
)
{
  return sndPlaySoundW ((LPCWSTR) pszSound, fuSound);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PlaySound
#define PlaySound error_use_qxePlaySound_or_PlaySoundA_and_PlaySoundW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePlaySound (const Extbyte * pszSound, HMODULE hmod, DWORD fdwSound)
)
{
  return PlaySoundW ((LPCWSTR) pszSound, hmod, fdwSound);
}

#undef waveOutGetDevCaps
#define waveOutGetDevCaps error_split_sized_LPWAVEOUTCAPS

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef waveOutGetErrorText
#define waveOutGetErrorText error_use_qxewaveOutGetErrorText_or_waveOutGetErrorTextA_and_waveOutGetErrorTextW
#endif
DECLARE_INLINE_HEADER (
MMRESULT qxewaveOutGetErrorText (MMRESULT mmrError, Extbyte * pszText, UINT cchText)
)
{
  return waveOutGetErrorTextW (mmrError, (LPWSTR) pszText, cchText);
}

#undef waveInGetDevCaps
#define waveInGetDevCaps error_split_sized_LPWAVEINCAPS

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef waveInGetErrorText
#define waveInGetErrorText error_use_qxewaveInGetErrorText_or_waveInGetErrorTextA_and_waveInGetErrorTextW
#endif
DECLARE_INLINE_HEADER (
MMRESULT qxewaveInGetErrorText (MMRESULT mmrError, Extbyte * pszText, UINT cchText)
)
{
  return waveInGetErrorTextW (mmrError, (LPWSTR) pszText, cchText);
}

#undef midiOutGetDevCaps
#define midiOutGetDevCaps error_split_sized_LPMIDIOUTCAPS

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef midiOutGetErrorText
#define midiOutGetErrorText error_use_qxemidiOutGetErrorText_or_midiOutGetErrorTextA_and_midiOutGetErrorTextW
#endif
DECLARE_INLINE_HEADER (
MMRESULT qxemidiOutGetErrorText (MMRESULT mmrError, Extbyte * pszText, UINT cchText)
)
{
  return midiOutGetErrorTextW (mmrError, (LPWSTR) pszText, cchText);
}

#undef midiInGetDevCaps
#define midiInGetDevCaps error_split_sized_LPMIDIOUTCAPS

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef midiInGetErrorText
#define midiInGetErrorText error_use_qxemidiInGetErrorText_or_midiInGetErrorTextA_and_midiInGetErrorTextW
#endif
DECLARE_INLINE_HEADER (
MMRESULT qxemidiInGetErrorText (MMRESULT mmrError, Extbyte * pszText, UINT cchText)
)
{
  return midiInGetErrorTextW (mmrError, (LPWSTR) pszText, cchText);
}

#undef auxGetDevCaps
#define auxGetDevCaps error_split_sized_LPAUXCAPS

#undef mixerGetDevCaps
#define mixerGetDevCaps error_split_sized_LPMIXERCAPS

#undef mixerGetLineInfo
#define mixerGetLineInfo error_split_sized_LPMIXERLINE

#undef mixerGetLineControls
#define mixerGetLineControls error_split_sized_LPMIXERCONTROL

#undef mixerGetControlDetails
#define mixerGetControlDetails error_split_sized_LPMIXERCONTROL_in_LPMIXERLINECONTROLS_in_LPMIXERCONTROLDETAILS

#undef joyGetDevCaps
#define joyGetDevCaps error_split_sized_LPJOYCAPS

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mmioStringToFOURCC
#define mmioStringToFOURCC error_use_qxemmioStringToFOURCC_or_mmioStringToFOURCCA_and_mmioStringToFOURCCW
#endif
DECLARE_INLINE_HEADER (
FOURCC qxemmioStringToFOURCC (const Extbyte * sz, UINT uFlags)
)
{
  return mmioStringToFOURCCW ((LPCWSTR) sz, uFlags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mmioInstallIOProc
#define mmioInstallIOProc error_use_qxemmioInstallIOProc_or_mmioInstallIOProcA_and_mmioInstallIOProcW
#endif
DECLARE_INLINE_HEADER (
LPMMIOPROC qxemmioInstallIOProc (FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags)
)
{
  return mmioInstallIOProcW (fccIOProc, pIOProc, dwFlags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mmioOpen
#define mmioOpen error_use_qxemmioOpen_or_mmioOpenA_and_mmioOpenW
#endif
DECLARE_INLINE_HEADER (
HMMIO qxemmioOpen (Extbyte * pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen)
)
{
  return mmioOpenW ((LPWSTR) pszFileName, pmmioinfo, fdwOpen);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mmioRename
#define mmioRename error_use_qxemmioRename_or_mmioRenameA_and_mmioRenameW
#endif
DECLARE_INLINE_HEADER (
MMRESULT qxemmioRename (const Extbyte * pszFileName, const Extbyte * pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename)
)
{
  return mmioRenameW ((LPCWSTR) pszFileName, (LPCWSTR) pszNewFileName, pmmioinfo, fdwRename);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mciSendCommand
#define mciSendCommand error_use_qxemciSendCommand_or_mciSendCommandA_and_mciSendCommandW
#endif
DECLARE_INLINE_HEADER (
MCIERROR qxemciSendCommand (MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
)
{
  return mciSendCommandW (mciId, uMsg, dwParam1, dwParam2);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mciSendString
#define mciSendString error_use_qxemciSendString_or_mciSendStringA_and_mciSendStringW
#endif
DECLARE_INLINE_HEADER (
MCIERROR qxemciSendString (const Extbyte * lpstrCommand, Extbyte * lpstrReturnString, UINT uReturnLength, HWND hwndCallback)
)
{
  return mciSendStringW ((LPCWSTR) lpstrCommand, (LPWSTR) lpstrReturnString, uReturnLength, hwndCallback);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mciGetDeviceID
#define mciGetDeviceID error_use_qxemciGetDeviceID_or_mciGetDeviceIDA_and_mciGetDeviceIDW
#endif
DECLARE_INLINE_HEADER (
MCIDEVICEID qxemciGetDeviceID (const Extbyte * pszDevice)
)
{
  return mciGetDeviceIDW ((LPCWSTR) pszDevice);
}

#if !defined (MINGW)
#undef mciGetDeviceIDFromElementID
#define mciGetDeviceIDFromElementID error_missing_from_Win98se_version_of_ADVAPI32_dll
#endif /* !defined (MINGW) */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef mciGetErrorString
#define mciGetErrorString error_use_qxemciGetErrorString_or_mciGetErrorStringA_and_mciGetErrorStringW
#endif
DECLARE_INLINE_HEADER (
BOOL qxemciGetErrorString (MCIERROR mcierr, Extbyte * pszText, UINT cchText)
)
{
  return mciGetErrorStringW (mcierr, (LPWSTR) pszText, cchText);
}


/* Processing file ACLAPI.h */


/*----------------------------------------------------------------------*/
/*                       Processing file ACLAPI.h                       */
/*----------------------------------------------------------------------*/

#undef GetEffectiveRightsFromAcl
#define GetEffectiveRightsFromAcl error_Function_needs_review_to_determine_how_to_handle_it

#undef GetAuditedPermissionsFromAcl
#define GetAuditedPermissionsFromAcl error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildSecurityDescriptor
#define BuildSecurityDescriptor error_Function_needs_review_to_determine_how_to_handle_it

#undef LookupSecurityDescriptorParts
#define LookupSecurityDescriptorParts error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildExplicitAccessWithName
#define BuildExplicitAccessWithName error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildImpersonateExplicitAccessWithName
#define BuildImpersonateExplicitAccessWithName error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildTrusteeWithName
#define BuildTrusteeWithName error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildImpersonateTrustee
#define BuildImpersonateTrustee error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildTrusteeWithSid
#define BuildTrusteeWithSid error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildTrusteeWithObjectsAndSid
#define BuildTrusteeWithObjectsAndSid error_Function_needs_review_to_determine_how_to_handle_it

#undef BuildTrusteeWithObjectsAndName
#define BuildTrusteeWithObjectsAndName error_Function_needs_review_to_determine_how_to_handle_it

#undef GetTrusteeName
#define GetTrusteeName error_Function_needs_review_to_determine_how_to_handle_it

#undef GetTrusteeType
#define GetTrusteeType error_Function_needs_review_to_determine_how_to_handle_it

#undef GetTrusteeForm
#define GetTrusteeForm error_Function_needs_review_to_determine_how_to_handle_it

#undef GetMultipleTrusteeOperation
#define GetMultipleTrusteeOperation error_Function_needs_review_to_determine_how_to_handle_it

#undef GetMultipleTrustee
#define GetMultipleTrustee error_Function_needs_review_to_determine_how_to_handle_it

#undef SetEntriesInAcl
#define SetEntriesInAcl error_Function_needs_review_to_determine_how_to_handle_it

#undef GetExplicitEntriesFromAcl
#define GetExplicitEntriesFromAcl error_Function_needs_review_to_determine_how_to_handle_it

/* Skipping GetNamedSecurityInfo because Cygwin declaration makes first param const */

#undef SetNamedSecurityInfo
#define SetNamedSecurityInfo error_Function_needs_review_to_determine_how_to_handle_it


/* Processing file LIBLOADERAPI.H */


/*----------------------------------------------------------------------*/
/*                    Processing file LIBLOADERAPI.H                    */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FindResourceEx
#define FindResourceEx error_use_qxeFindResourceEx_or_FindResourceExA_and_FindResourceExW
#endif
DECLARE_INLINE_HEADER (
HRSRC qxeFindResourceEx (HMODULE hModule, const Extbyte * lpType, const Extbyte * lpName, WORD wLanguage)
)
{
  return FindResourceExW (hModule, (LPCWSTR) lpType, (LPCWSTR) lpName, wLanguage);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadLibraryEx
#define LoadLibraryEx error_use_qxeLoadLibraryEx_or_LoadLibraryExA_and_LoadLibraryExW
#endif
DECLARE_INLINE_HEADER (
HMODULE qxeLoadLibraryEx (const Extbyte * lpLibFileName, HANDLE hFile, DWORD dwFlags)
)
{
  return LoadLibraryExW ((LPCWSTR) lpLibFileName, hFile, dwFlags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetModuleHandle
#define GetModuleHandle error_use_qxeGetModuleHandle_or_GetModuleHandleA_and_GetModuleHandleW
#endif
DECLARE_INLINE_HEADER (
HMODULE qxeGetModuleHandle (const Extbyte * lpModuleName)
)
{
  return GetModuleHandleW ((LPCWSTR) lpModuleName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadLibrary
#define LoadLibrary error_use_qxeLoadLibrary_or_LoadLibraryA_and_LoadLibraryW
#endif
DECLARE_INLINE_HEADER (
HMODULE qxeLoadLibrary (const Extbyte * lpLibFileName)
)
{
  return LoadLibraryW ((LPCWSTR) lpLibFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetModuleFileName
#define GetModuleFileName error_use_qxeGetModuleFileName_or_GetModuleFileNameA_and_GetModuleFileNameW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetModuleFileName (HMODULE hModule, Extbyte * lpFilename, DWORD nSize)
)
{
  return GetModuleFileNameW (hModule, (LPWSTR) lpFilename, nSize);
}


/* Processing file sysinfoapi.h */


/*----------------------------------------------------------------------*/
/*                     Processing file sysinfoapi.h                     */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetWindowsDirectory
#define GetWindowsDirectory error_use_qxeGetWindowsDirectory_or_GetWindowsDirectoryA_and_GetWindowsDirectoryW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetWindowsDirectory (Extbyte * lpBuffer, UINT uSize)
)
{
  return GetWindowsDirectoryW ((LPWSTR) lpBuffer, uSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetSystemDirectory
#define GetSystemDirectory error_use_qxeGetSystemDirectory_or_GetSystemDirectoryA_and_GetSystemDirectoryW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetSystemDirectory (Extbyte * lpBuffer, UINT uSize)
)
{
  return GetSystemDirectoryW ((LPWSTR) lpBuffer, uSize);
}


/* Processing file memoryapi.h */


/*----------------------------------------------------------------------*/
/*                     Processing file memoryapi.h                      */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateFileMapping
#define CreateFileMapping error_use_qxeCreateFileMapping_or_CreateFileMappingA_and_CreateFileMappingW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateFileMapping (HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, const Extbyte * lpName)
)
{
  return CreateFileMappingW (hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, (LPCWSTR) lpName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenFileMapping
#define OpenFileMapping error_use_qxeOpenFileMapping_or_OpenFileMappingA_and_OpenFileMappingW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeOpenFileMapping (DWORD dwDesiredAccess, BOOL bInheritHandle, const Extbyte * lpName)
)
{
  return OpenFileMappingW (dwDesiredAccess, bInheritHandle, (LPCWSTR) lpName);
}


/* Processing file IMM.H */


/*----------------------------------------------------------------------*/
/*                        Processing file IMM.H                         */
/*----------------------------------------------------------------------*/

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmInstallIME
#define ImmInstallIME error_use_qxeImmInstallIME_or_ImmInstallIMEA_and_ImmInstallIMEW
#endif
DECLARE_INLINE_HEADER (
HKL qxeImmInstallIME (const Extbyte * lpszIMEFileName, const Extbyte * lpszLayoutText)
)
{
  return ImmInstallIMEW ((LPCWSTR) lpszIMEFileName, (LPCWSTR) lpszLayoutText);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetDescription
#define ImmGetDescription error_use_qxeImmGetDescription_or_ImmGetDescriptionA_and_ImmGetDescriptionW
#endif
DECLARE_INLINE_HEADER (
UINT qxeImmGetDescription (HKL arg1, Extbyte * arg2, UINT uBufLen)
)
{
  return ImmGetDescriptionW (arg1, (LPWSTR) arg2, uBufLen);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetIMEFileName
#define ImmGetIMEFileName error_use_qxeImmGetIMEFileName_or_ImmGetIMEFileNameA_and_ImmGetIMEFileNameW
#endif
DECLARE_INLINE_HEADER (
UINT qxeImmGetIMEFileName (HKL arg1, Extbyte * arg2, UINT uBufLen)
)
{
  return ImmGetIMEFileNameW (arg1, (LPWSTR) arg2, uBufLen);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetCompositionString
#define ImmGetCompositionString error_use_qxeImmGetCompositionString_or_ImmGetCompositionStringA_and_ImmGetCompositionStringW
#endif
DECLARE_INLINE_HEADER (
LONG qxeImmGetCompositionString (HIMC arg1, DWORD arg2, LPVOID arg3, DWORD arg4)
)
{
  return ImmGetCompositionStringW (arg1, arg2, arg3, arg4);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
/* Skipping ImmSetCompositionString because different prototypes in VC6 and VC7 */
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetCandidateListCount
#define ImmGetCandidateListCount error_use_qxeImmGetCandidateListCount_or_ImmGetCandidateListCountA_and_ImmGetCandidateListCountW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeImmGetCandidateListCount (HIMC arg1, LPDWORD lpdwListCount)
)
{
  return ImmGetCandidateListCountW (arg1, lpdwListCount);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetCandidateList
#define ImmGetCandidateList error_use_qxeImmGetCandidateList_or_ImmGetCandidateListA_and_ImmGetCandidateListW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeImmGetCandidateList (HIMC arg1, DWORD deIndex, LPCANDIDATELIST arg3, DWORD dwBufLen)
)
{
  return ImmGetCandidateListW (arg1, deIndex, arg3, dwBufLen);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetGuideLine
#define ImmGetGuideLine error_use_qxeImmGetGuideLine_or_ImmGetGuideLineA_and_ImmGetGuideLineW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeImmGetGuideLine (HIMC arg1, DWORD dwIndex, Extbyte * arg3, DWORD dwBufLen)
)
{
  return ImmGetGuideLineW (arg1, dwIndex, (LPWSTR) arg3, dwBufLen);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetCompositionFont
#define ImmGetCompositionFont error_use_qxeImmGetCompositionFont_or_ImmGetCompositionFontA_and_ImmGetCompositionFontW
#endif
/* NOTE: split-sized LOGFONT */
DECLARE_INLINE_HEADER (
BOOL qxeImmGetCompositionFont (HIMC arg1, LPLOGFONTW arg2)
)
{
  return ImmGetCompositionFontW (arg1, arg2);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmSetCompositionFont
#define ImmSetCompositionFont error_use_qxeImmSetCompositionFont_or_ImmSetCompositionFontA_and_ImmSetCompositionFontW
#endif
/* NOTE: split-sized LOGFONT */
DECLARE_INLINE_HEADER (
BOOL qxeImmSetCompositionFont (HIMC arg1, LPLOGFONTW arg2)
)
{
  return ImmSetCompositionFontW (arg1, arg2);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmConfigureIME
#define ImmConfigureIME error_use_qxeImmConfigureIME_or_ImmConfigureIMEA_and_ImmConfigureIMEW
#endif
/* NOTE: // split-simple REGISTERWORD */
DECLARE_INLINE_HEADER (
BOOL qxeImmConfigureIME (HKL arg1, HWND arg2, DWORD arg3, LPVOID arg4)
)
{
  return ImmConfigureIMEW (arg1, arg2, arg3, arg4);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmEscape
#define ImmEscape error_use_qxeImmEscape_or_ImmEscapeA_and_ImmEscapeW
#endif
/* NOTE: // strings of various sorts */
DECLARE_INLINE_HEADER (
LRESULT qxeImmEscape (HKL arg1, HIMC arg2, UINT arg3, LPVOID arg4)
)
{
  return ImmEscapeW (arg1, arg2, arg3, arg4);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmGetConversionList
#define ImmGetConversionList error_use_qxeImmGetConversionList_or_ImmGetConversionListA_and_ImmGetConversionListW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeImmGetConversionList (HKL arg1, HIMC arg2, const Extbyte * arg3, LPCANDIDATELIST arg4, DWORD dwBufLen, UINT uFlag)
)
{
  return ImmGetConversionListW (arg1, arg2, (LPCWSTR) arg3, arg4, dwBufLen, uFlag);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmIsUIMessage
#define ImmIsUIMessage error_use_qxeImmIsUIMessage_or_ImmIsUIMessageA_and_ImmIsUIMessageW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeImmIsUIMessage (HWND arg1, UINT arg2, WPARAM arg3, LPARAM arg4)
)
{
  return ImmIsUIMessageW (arg1, arg2, arg3, arg4);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmRegisterWord
#define ImmRegisterWord error_use_qxeImmRegisterWord_or_ImmRegisterWordA_and_ImmRegisterWordW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeImmRegisterWord (HKL arg1, const Extbyte * lpszReading, DWORD arg3, const Extbyte * lpszRegister)
)
{
  return ImmRegisterWordW (arg1, (LPCWSTR) lpszReading, arg3, (LPCWSTR) lpszRegister);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmUnregisterWord
#define ImmUnregisterWord error_use_qxeImmUnregisterWord_or_ImmUnregisterWordA_and_ImmUnregisterWordW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeImmUnregisterWord (HKL arg1, const Extbyte * lpszReading, DWORD arg3, const Extbyte * lpszUnregister)
)
{
  return ImmUnregisterWordW (arg1, (LPCWSTR) lpszReading, arg3, (LPCWSTR) lpszUnregister);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef ImmGetRegisterWordStyle
#define ImmGetRegisterWordStyle error_split_sized_STYLEBUF
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ImmEnumRegisterWord
#define ImmEnumRegisterWord error_use_qxeImmEnumRegisterWord_or_ImmEnumRegisterWordA_and_ImmEnumRegisterWordW
#endif
DECLARE_INLINE_HEADER (
UINT qxeImmEnumRegisterWord (HKL arg1, REGISTERWORDENUMPROCW arg2, const Extbyte * lpszReading, DWORD arg4, const Extbyte * lpszRegister, LPVOID arg6)
)
{
  return ImmEnumRegisterWordW (arg1, arg2, (LPCWSTR) lpszReading, arg4, (LPCWSTR) lpszRegister, arg6);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef ImmGetImeMenuItems
#define ImmGetImeMenuItems error_split_sized_IMEMENUITEMINFO
#endif /* defined (HAVE_MS_WINDOWS) */


/* Processing file WINUSER.H */


/*----------------------------------------------------------------------*/
/*                      Processing file WINUSER.H                       */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef wvsprintf
#define wvsprintf error_use_qxewvsprintf_or_wvsprintfA_and_wvsprintfW
#endif
DECLARE_INLINE_HEADER (
int qxewvsprintf (Extbyte * arg1, const Extbyte * arg2, va_list arglist)
)
{
  return wvsprintfW ((LPWSTR) arg1, (LPCWSTR) arg2, arglist);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadKeyboardLayout
#define LoadKeyboardLayout error_use_qxeLoadKeyboardLayout_or_LoadKeyboardLayoutA_and_LoadKeyboardLayoutW
#endif
DECLARE_INLINE_HEADER (
HKL qxeLoadKeyboardLayout (const Extbyte * pwszKLID, UINT Flags)
)
{
  return LoadKeyboardLayoutW ((LPCWSTR) pwszKLID, Flags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetKeyboardLayoutName
#define GetKeyboardLayoutName error_use_qxeGetKeyboardLayoutName_or_GetKeyboardLayoutNameA_and_GetKeyboardLayoutNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetKeyboardLayoutName (Extbyte * pwszKLID)
)
{
  return GetKeyboardLayoutNameW ((LPWSTR) pwszKLID);
}

#undef CreateDesktop
#define CreateDesktop error_split_sized_LPDEVMODE

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenDesktop
#define OpenDesktop error_use_qxeOpenDesktop_or_OpenDesktopA_and_OpenDesktopW
#endif
DECLARE_INLINE_HEADER (
HDESK qxeOpenDesktop (const Extbyte * lpszDesktop, DWORD dwFlags, BOOL fInherit, ACCESS_MASK dwDesiredAccess)
)
{
  return OpenDesktopW ((LPCWSTR) lpszDesktop, dwFlags, fInherit, dwDesiredAccess);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EnumDesktops
#define EnumDesktops error_use_qxeEnumDesktops_or_EnumDesktopsA_and_EnumDesktopsW
#endif
/* NOTE: // callback fun differs only in string pointer type */
DECLARE_INLINE_HEADER (
BOOL qxeEnumDesktops (HWINSTA hwinsta, DESKTOPENUMPROCW lpEnumFunc, LPARAM lParam)
)
{
  return EnumDesktopsW (hwinsta, lpEnumFunc, lParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateWindowStation
#define CreateWindowStation error_use_qxeCreateWindowStation_or_CreateWindowStationA_and_CreateWindowStationW
#endif
/* NOTE: error arg 1, VS6 prototype, missing const.
   NOTE: Prototype manually overridden.
         Header file claims:
           WINUSERAPI HWINSTA WINAPI CreateWindowStation(LPCWSTR lpwinsta,DWORD dwFlags,ACCESS_MASK dwDesiredAccess,LPSECURITY_ATTRIBUTES lpsa)
         Overridden with:
           HWINSTA CreateWindowStation(LPWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES)
         Differences in return-type qualifiers, e.g. WINAPI, are not important.
 */
DECLARE_INLINE_HEADER (
HWINSTA qxeCreateWindowStation (Extbyte * arg1, DWORD arg2, DWORD arg3, LPSECURITY_ATTRIBUTES arg4)
)
{
  return CreateWindowStationW ((LPWSTR) arg1, arg2, arg3, arg4);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenWindowStation
#define OpenWindowStation error_use_qxeOpenWindowStation_or_OpenWindowStationA_and_OpenWindowStationW
#endif
DECLARE_INLINE_HEADER (
HWINSTA qxeOpenWindowStation (const Extbyte * lpszWinSta, BOOL fInherit, ACCESS_MASK dwDesiredAccess)
)
{
  return OpenWindowStationW ((LPCWSTR) lpszWinSta, fInherit, dwDesiredAccess);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EnumWindowStations
#define EnumWindowStations error_use_qxeEnumWindowStations_or_EnumWindowStationsA_and_EnumWindowStationsW
#endif
/* NOTE: // callback fun differs only in string pointer type */
DECLARE_INLINE_HEADER (
BOOL qxeEnumWindowStations (WINSTAENUMPROCW lpEnumFunc, LPARAM lParam)
)
{
  return EnumWindowStationsW (lpEnumFunc, lParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetUserObjectInformation
#define GetUserObjectInformation error_use_qxeGetUserObjectInformation_or_GetUserObjectInformationA_and_GetUserObjectInformationW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetUserObjectInformation (HANDLE hObj, int nIndex, PVOID pvInfo, DWORD nLength, LPDWORD lpnLengthNeeded)
)
{
  return GetUserObjectInformationW (hObj, nIndex, pvInfo, nLength, lpnLengthNeeded);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetUserObjectInformation
#define SetUserObjectInformation error_use_qxeSetUserObjectInformation_or_SetUserObjectInformationA_and_SetUserObjectInformationW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetUserObjectInformation (HANDLE hObj, int nIndex, PVOID pvInfo, DWORD nLength)
)
{
  return SetUserObjectInformationW (hObj, nIndex, pvInfo, nLength);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegisterWindowMessage
#define RegisterWindowMessage error_use_qxeRegisterWindowMessage_or_RegisterWindowMessageA_and_RegisterWindowMessageW
#endif
DECLARE_INLINE_HEADER (
UINT qxeRegisterWindowMessage (const Extbyte * lpString)
)
{
  return RegisterWindowMessageW ((LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetMessage
#define GetMessage error_use_qxeGetMessage_or_GetMessageA_and_GetMessageW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetMessage (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
)
{
  return GetMessageW (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DispatchMessage
#define DispatchMessage error_use_qxeDispatchMessage_or_DispatchMessageA_and_DispatchMessageW
#endif
DECLARE_INLINE_HEADER (
LRESULT qxeDispatchMessage (const MSG * lpMsg)
)
{
  return DispatchMessageW (lpMsg);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PeekMessage
#define PeekMessage error_use_qxePeekMessage_or_PeekMessageA_and_PeekMessageW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePeekMessage (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
)
{
  return PeekMessageW (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SendMessage
#define SendMessage error_use_qxeSendMessage_or_SendMessageA_and_SendMessageW
#endif
DECLARE_INLINE_HEADER (
LRESULT qxeSendMessage (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
)
{
  return SendMessageW (hWnd, Msg, wParam, lParam);
}

#undef SendMessageTimeout
#define SendMessageTimeout error_VS6_has_erroneous_seventh_parameter_DWORD_PTR_instead_of_PDWORD_PTR

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SendNotifyMessage
#define SendNotifyMessage error_use_qxeSendNotifyMessage_or_SendNotifyMessageA_and_SendNotifyMessageW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSendNotifyMessage (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
)
{
  return SendNotifyMessageW (hWnd, Msg, wParam, lParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SendMessageCallback
#define SendMessageCallback error_use_qxeSendMessageCallback_or_SendMessageCallbackA_and_SendMessageCallbackW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSendMessageCallback (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData)
)
{
  return SendMessageCallbackW (hWnd, Msg, wParam, lParam, lpResultCallBack, dwData);
}

#undef BroadcastSystemMessageEx
#define BroadcastSystemMessageEx error_Function_needs_review_to_determine_how_to_handle_it

#undef BroadcastSystemMessage
#define BroadcastSystemMessage error_win95_version_not_split__NT_4_0__only

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PostMessage
#define PostMessage error_use_qxePostMessage_or_PostMessageA_and_PostMessageW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePostMessage (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
)
{
  return PostMessageW (hWnd, Msg, wParam, lParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PostThreadMessage
#define PostThreadMessage error_use_qxePostThreadMessage_or_PostThreadMessageA_and_PostThreadMessageW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePostThreadMessage (DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
)
{
  return PostThreadMessageW (idThread, Msg, wParam, lParam);
}

/* Skipping DefWindowProc because return value is conditionalized on _MAC, messes up parser */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegisterClass
#define RegisterClass error_use_qxeRegisterClass_or_RegisterClassA_and_RegisterClassW
#endif
DECLARE_INLINE_HEADER (
ATOM qxeRegisterClass (const WNDCLASSW * lpWndClass)
)
{
  return RegisterClassW (lpWndClass);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef UnregisterClass
#define UnregisterClass error_use_qxeUnregisterClass_or_UnregisterClassA_and_UnregisterClassW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeUnregisterClass (const Extbyte * lpClassName, HINSTANCE hInstance)
)
{
  return UnregisterClassW ((LPCWSTR) lpClassName, hInstance);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetClassInfo
#define GetClassInfo error_use_qxeGetClassInfo_or_GetClassInfoA_and_GetClassInfoW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetClassInfo (HINSTANCE hInstance, const Extbyte * lpClassName, LPWNDCLASSW lpWndClass)
)
{
  return GetClassInfoW (hInstance, (LPCWSTR) lpClassName, lpWndClass);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegisterClassEx
#define RegisterClassEx error_use_qxeRegisterClassEx_or_RegisterClassExA_and_RegisterClassExW
#endif
DECLARE_INLINE_HEADER (
ATOM qxeRegisterClassEx (const WNDCLASSEXW * arg1)
)
{
  return RegisterClassExW (arg1);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetClassInfoEx
#define GetClassInfoEx error_use_qxeGetClassInfoEx_or_GetClassInfoExA_and_GetClassInfoExW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeGetClassInfoEx (HINSTANCE hInstance, const Extbyte * lpszClass, LPWNDCLASSEXW lpwcx)
)
{
  return GetClassInfoExW (hInstance, (LPCWSTR) lpszClass, lpwcx);
}

#undef CallWindowProc
#define CallWindowProc error_two_versions__STRICT_and_non_STRICT

#undef RegisterDeviceNotification
#define RegisterDeviceNotification error_NT_5_0__only

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateWindowEx
#define CreateWindowEx error_use_qxeCreateWindowEx_or_CreateWindowExA_and_CreateWindowExW
#endif
DECLARE_INLINE_HEADER (
HWND qxeCreateWindowEx (DWORD dwExStyle, const Extbyte * lpClassName, const Extbyte * lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
)
{
  return CreateWindowExW (dwExStyle, (LPCWSTR) lpClassName, (LPCWSTR) lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateDialogParam
#define CreateDialogParam error_use_qxeCreateDialogParam_or_CreateDialogParamA_and_CreateDialogParamW
#endif
DECLARE_INLINE_HEADER (
HWND qxeCreateDialogParam (HINSTANCE hInstance, const Extbyte * lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
)
{
  return CreateDialogParamW (hInstance, (LPCWSTR) lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateDialogIndirectParam
#define CreateDialogIndirectParam error_use_qxeCreateDialogIndirectParam_or_CreateDialogIndirectParamA_and_CreateDialogIndirectParamW
#endif
/* NOTE: error in Cygwin prototype (no split) but fixable with typedef */
DECLARE_INLINE_HEADER (
HWND qxeCreateDialogIndirectParam (HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
)
{
  return CreateDialogIndirectParamW (hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DialogBoxParam
#define DialogBoxParam error_use_qxeDialogBoxParam_or_DialogBoxParamA_and_DialogBoxParamW
#endif
DECLARE_INLINE_HEADER (
INT_PTR qxeDialogBoxParam (HINSTANCE hInstance, const Extbyte * lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
)
{
  return DialogBoxParamW (hInstance, (LPCWSTR) lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DialogBoxIndirectParam
#define DialogBoxIndirectParam error_use_qxeDialogBoxIndirectParam_or_DialogBoxIndirectParamA_and_DialogBoxIndirectParamW
#endif
/* NOTE: error in Cygwin prototype (no split) but fixable with typedef */
DECLARE_INLINE_HEADER (
INT_PTR qxeDialogBoxIndirectParam (HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
)
{
  return DialogBoxIndirectParamW (hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetDlgItemText
#define SetDlgItemText error_use_qxeSetDlgItemText_or_SetDlgItemTextA_and_SetDlgItemTextW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetDlgItemText (HWND hDlg, int nIDDlgItem, const Extbyte * lpString)
)
{
  return SetDlgItemTextW (hDlg, nIDDlgItem, (LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetDlgItemText
#define GetDlgItemText error_use_qxeGetDlgItemText_or_GetDlgItemTextA_and_GetDlgItemTextW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetDlgItemText (HWND hDlg, int nIDDlgItem, Extbyte * lpString, int cchMax)
)
{
  return GetDlgItemTextW (hDlg, nIDDlgItem, (LPWSTR) lpString, cchMax);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SendDlgItemMessage
#define SendDlgItemMessage error_use_qxeSendDlgItemMessage_or_SendDlgItemMessageA_and_SendDlgItemMessageW
#endif
DECLARE_INLINE_HEADER (
LRESULT qxeSendDlgItemMessage (HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
)
{
  return SendDlgItemMessageW (hDlg, nIDDlgItem, Msg, wParam, lParam);
}

#undef DefDlgProc
#define DefDlgProc error_return_value_is_conditionalized_on__MAC__messes_up_parser

#if !defined (CYGWIN_HEADERS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CallMsgFilter
#define CallMsgFilter error_use_qxeCallMsgFilter_or_CallMsgFilterA_and_CallMsgFilterW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCallMsgFilter (LPMSG lpMsg, int nCode)
)
{
  return CallMsgFilterW (lpMsg, nCode);
}
#endif /* !defined (CYGWIN_HEADERS) */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegisterClipboardFormat
#define RegisterClipboardFormat error_use_qxeRegisterClipboardFormat_or_RegisterClipboardFormatA_and_RegisterClipboardFormatW
#endif
DECLARE_INLINE_HEADER (
UINT qxeRegisterClipboardFormat (const Extbyte * lpszFormat)
)
{
  return RegisterClipboardFormatW ((LPCWSTR) lpszFormat);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetClipboardFormatName
#define GetClipboardFormatName error_use_qxeGetClipboardFormatName_or_GetClipboardFormatNameA_and_GetClipboardFormatNameW
#endif
DECLARE_INLINE_HEADER (
int qxeGetClipboardFormatName (UINT format, Extbyte * lpszFormatName, int cchMaxCount)
)
{
  return GetClipboardFormatNameW (format, (LPWSTR) lpszFormatName, cchMaxCount);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharToOem
#define CharToOem error_use_qxeCharToOem_or_CharToOemA_and_CharToOemW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCharToOem (const Extbyte * lpszSrc, LPSTR lpszDst)
)
{
  return CharToOemW ((LPCWSTR) lpszSrc, lpszDst);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OemToChar
#define OemToChar error_use_qxeOemToChar_or_OemToCharA_and_OemToCharW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeOemToChar (LPCSTR lpszSrc, Extbyte * lpszDst)
)
{
  return OemToCharW (lpszSrc, (LPWSTR) lpszDst);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharToOemBuff
#define CharToOemBuff error_use_qxeCharToOemBuff_or_CharToOemBuffA_and_CharToOemBuffW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCharToOemBuff (const Extbyte * lpszSrc, LPSTR lpszDst, DWORD cchDstLength)
)
{
  return CharToOemBuffW ((LPCWSTR) lpszSrc, lpszDst, cchDstLength);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OemToCharBuff
#define OemToCharBuff error_use_qxeOemToCharBuff_or_OemToCharBuffA_and_OemToCharBuffW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeOemToCharBuff (LPCSTR lpszSrc, Extbyte * lpszDst, DWORD cchDstLength)
)
{
  return OemToCharBuffW (lpszSrc, (LPWSTR) lpszDst, cchDstLength);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharUpper
#define CharUpper error_use_qxeCharUpper_or_CharUpperA_and_CharUpperW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxeCharUpper (Extbyte * lpsz)
)
{
  return (Extbyte *) CharUpperW ((LPWSTR) lpsz);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharUpperBuff
#define CharUpperBuff error_use_qxeCharUpperBuff_or_CharUpperBuffA_and_CharUpperBuffW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeCharUpperBuff (Extbyte * lpsz, DWORD cchLength)
)
{
  return CharUpperBuffW ((LPWSTR) lpsz, cchLength);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharLower
#define CharLower error_use_qxeCharLower_or_CharLowerA_and_CharLowerW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxeCharLower (Extbyte * lpsz)
)
{
  return (Extbyte *) CharLowerW ((LPWSTR) lpsz);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharLowerBuff
#define CharLowerBuff error_use_qxeCharLowerBuff_or_CharLowerBuffA_and_CharLowerBuffW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeCharLowerBuff (Extbyte * lpsz, DWORD cchLength)
)
{
  return CharLowerBuffW ((LPWSTR) lpsz, cchLength);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharNext
#define CharNext error_use_qxeCharNext_or_CharNextA_and_CharNextW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxeCharNext (const Extbyte * lpsz)
)
{
  return (Extbyte *) CharNextW ((LPCWSTR) lpsz);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CharPrev
#define CharPrev error_use_qxeCharPrev_or_CharPrevA_and_CharPrevW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxeCharPrev (const Extbyte * lpszStart, const Extbyte * lpszCurrent)
)
{
  return (Extbyte *) CharPrevW ((LPCWSTR) lpszStart, (LPCWSTR) lpszCurrent);
}

#undef IsCharAlpha
#define IsCharAlpha error_split_CHAR

#undef IsCharAlphaNumeric
#define IsCharAlphaNumeric error_split_CHAR

#undef IsCharUpper
#define IsCharUpper error_split_CHAR

#undef IsCharLower
#define IsCharLower error_split_CHAR

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetKeyNameText
#define GetKeyNameText error_use_qxeGetKeyNameText_or_GetKeyNameTextA_and_GetKeyNameTextW
#endif
DECLARE_INLINE_HEADER (
int qxeGetKeyNameText (LONG lParam, Extbyte * lpString, int cchSize)
)
{
  return GetKeyNameTextW (lParam, (LPWSTR) lpString, cchSize);
}

/* Skipping VkKeyScan because split CHAR */

#undef VkKeyScanEx
#define VkKeyScanEx error_split_CHAR__NT_4_0__only

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef MapVirtualKey
#define MapVirtualKey error_use_qxeMapVirtualKey_or_MapVirtualKeyA_and_MapVirtualKeyW
#endif
DECLARE_INLINE_HEADER (
UINT qxeMapVirtualKey (UINT uCode, UINT uMapType)
)
{
  return MapVirtualKeyW (uCode, uMapType);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef MapVirtualKeyEx
#define MapVirtualKeyEx error_use_qxeMapVirtualKeyEx_or_MapVirtualKeyExA_and_MapVirtualKeyExW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
UINT qxeMapVirtualKeyEx (UINT uCode, UINT uMapType, HKL dwhkl)
)
{
  return MapVirtualKeyExW (uCode, uMapType, dwhkl);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadAccelerators
#define LoadAccelerators error_use_qxeLoadAccelerators_or_LoadAcceleratorsA_and_LoadAcceleratorsW
#endif
DECLARE_INLINE_HEADER (
HACCEL qxeLoadAccelerators (HINSTANCE hInstance, const Extbyte * lpTableName)
)
{
  return LoadAcceleratorsW (hInstance, (LPCWSTR) lpTableName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateAcceleratorTable
#define CreateAcceleratorTable error_use_qxeCreateAcceleratorTable_or_CreateAcceleratorTableA_and_CreateAcceleratorTableW
#endif
DECLARE_INLINE_HEADER (
HACCEL qxeCreateAcceleratorTable (LPACCEL paccel, int cAccel)
)
{
  return CreateAcceleratorTableW (paccel, cAccel);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CopyAcceleratorTable
#define CopyAcceleratorTable error_use_qxeCopyAcceleratorTable_or_CopyAcceleratorTableA_and_CopyAcceleratorTableW
#endif
DECLARE_INLINE_HEADER (
int qxeCopyAcceleratorTable (HACCEL hAccelSrc, LPACCEL lpAccelDst, int cAccelEntries)
)
{
  return CopyAcceleratorTableW (hAccelSrc, lpAccelDst, cAccelEntries);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef TranslateAccelerator
#define TranslateAccelerator error_use_qxeTranslateAccelerator_or_TranslateAcceleratorA_and_TranslateAcceleratorW
#endif
DECLARE_INLINE_HEADER (
int qxeTranslateAccelerator (HWND hWnd, HACCEL hAccTable, LPMSG lpMsg)
)
{
  return TranslateAcceleratorW (hWnd, hAccTable, lpMsg);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadMenu
#define LoadMenu error_use_qxeLoadMenu_or_LoadMenuA_and_LoadMenuW
#endif
DECLARE_INLINE_HEADER (
HMENU qxeLoadMenu (HINSTANCE hInstance, const Extbyte * lpMenuName)
)
{
  return LoadMenuW (hInstance, (LPCWSTR) lpMenuName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadMenuIndirect
#define LoadMenuIndirect error_use_qxeLoadMenuIndirect_or_LoadMenuIndirectA_and_LoadMenuIndirectW
#endif
DECLARE_INLINE_HEADER (
HMENU qxeLoadMenuIndirect (const MENUTEMPLATEW * lpMenuTemplate)
)
{
  return LoadMenuIndirectW (lpMenuTemplate);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ChangeMenu
#define ChangeMenu error_use_qxeChangeMenu_or_ChangeMenuA_and_ChangeMenuW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeChangeMenu (HMENU hMenu, UINT cmd, const Extbyte * lpszNewItem, UINT cmdInsert, UINT flags)
)
{
  return ChangeMenuW (hMenu, cmd, (LPCWSTR) lpszNewItem, cmdInsert, flags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetMenuString
#define GetMenuString error_use_qxeGetMenuString_or_GetMenuStringA_and_GetMenuStringW
#endif
DECLARE_INLINE_HEADER (
int qxeGetMenuString (HMENU hMenu, UINT uIDItem, Extbyte * lpString, int cchMax, UINT flags)
)
{
  return GetMenuStringW (hMenu, uIDItem, (LPWSTR) lpString, cchMax, flags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef InsertMenu
#define InsertMenu error_use_qxeInsertMenu_or_InsertMenuA_and_InsertMenuW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeInsertMenu (HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, const Extbyte * lpNewItem)
)
{
  return InsertMenuW (hMenu, uPosition, uFlags, uIDNewItem, (LPCWSTR) lpNewItem);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef AppendMenu
#define AppendMenu error_use_qxeAppendMenu_or_AppendMenuA_and_AppendMenuW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeAppendMenu (HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, const Extbyte * lpNewItem)
)
{
  return AppendMenuW (hMenu, uFlags, uIDNewItem, (LPCWSTR) lpNewItem);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ModifyMenu
#define ModifyMenu error_use_qxeModifyMenu_or_ModifyMenuA_and_ModifyMenuW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeModifyMenu (HMENU hMnu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, const Extbyte * lpNewItem)
)
{
  return ModifyMenuW (hMnu, uPosition, uFlags, uIDNewItem, (LPCWSTR) lpNewItem);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef InsertMenuItem
#define InsertMenuItem error_use_qxeInsertMenuItem_or_InsertMenuItemA_and_InsertMenuItemW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeInsertMenuItem (HMENU hmenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmi)
)
{
  return InsertMenuItemW (hmenu, item, fByPosition, lpmi);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetMenuItemInfo
#define GetMenuItemInfo error_use_qxeGetMenuItemInfo_or_GetMenuItemInfoA_and_GetMenuItemInfoW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeGetMenuItemInfo (HMENU hmenu, UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii)
)
{
  return GetMenuItemInfoW (hmenu, item, fByPosition, lpmii);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetMenuItemInfo
#define SetMenuItemInfo error_use_qxeSetMenuItemInfo_or_SetMenuItemInfoA_and_SetMenuItemInfoW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeSetMenuItemInfo (HMENU hmenu, UINT item, BOOL fByPositon, LPCMENUITEMINFOW lpmii)
)
{
  return SetMenuItemInfoW (hmenu, item, fByPositon, lpmii);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DrawText
#define DrawText error_use_qxeDrawText_or_DrawTextA_and_DrawTextW
#endif
DECLARE_INLINE_HEADER (
int qxeDrawText (HDC hdc, const Extbyte * lpchText, int cchText, LPRECT lprc, UINT format)
)
{
  return DrawTextW (hdc, (LPCWSTR) lpchText, cchText, lprc, format);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DrawTextEx
#define DrawTextEx error_use_qxeDrawTextEx_or_DrawTextExA_and_DrawTextExW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
int qxeDrawTextEx (HDC hdc, Extbyte * lpchText, int cchText, LPRECT lprc, UINT format, LPDRAWTEXTPARAMS lpdtp)
)
{
  return DrawTextExW (hdc, (LPWSTR) lpchText, cchText, lprc, format, lpdtp);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GrayString
#define GrayString error_use_qxeGrayString_or_GrayStringA_and_GrayStringW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGrayString (HDC hDC, HBRUSH hBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData, int nCount, int X, int Y, int nWidth, int nHeight)
)
{
  return GrayStringW (hDC, hBrush, lpOutputFunc, lpData, nCount, X, Y, nWidth, nHeight);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DrawState
#define DrawState error_use_qxeDrawState_or_DrawStateA_and_DrawStateW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeDrawState (HDC hdc, HBRUSH hbrFore, DRAWSTATEPROC qfnCallBack, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags)
)
{
  return DrawStateW (hdc, hbrFore, qfnCallBack, lData, wData, x, y, cx, cy, uFlags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef TabbedTextOut
#define TabbedTextOut error_use_qxeTabbedTextOut_or_TabbedTextOutA_and_TabbedTextOutW
#endif
DECLARE_INLINE_HEADER (
LONG qxeTabbedTextOut (HDC hdc, int x, int y, const Extbyte * lpString, int chCount, int nTabPositions, const INT * lpnTabStopPositions, int nTabOrigin)
)
{
  return TabbedTextOutW (hdc, x, y, (LPCWSTR) lpString, chCount, nTabPositions, lpnTabStopPositions, nTabOrigin);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTabbedTextExtent
#define GetTabbedTextExtent error_use_qxeGetTabbedTextExtent_or_GetTabbedTextExtentA_and_GetTabbedTextExtentW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetTabbedTextExtent (HDC hdc, const Extbyte * lpString, int chCount, int nTabPositions, const INT * lpnTabStopPositions)
)
{
  return GetTabbedTextExtentW (hdc, (LPCWSTR) lpString, chCount, nTabPositions, lpnTabStopPositions);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetProp
#define SetProp error_use_qxeSetProp_or_SetPropA_and_SetPropW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetProp (HWND hWnd, const Extbyte * lpString, HANDLE hData)
)
{
  return SetPropW (hWnd, (LPCWSTR) lpString, hData);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetProp
#define GetProp error_use_qxeGetProp_or_GetPropA_and_GetPropW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeGetProp (HWND hWnd, const Extbyte * lpString)
)
{
  return GetPropW (hWnd, (LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RemoveProp
#define RemoveProp error_use_qxeRemoveProp_or_RemovePropA_and_RemovePropW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeRemoveProp (HWND hWnd, const Extbyte * lpString)
)
{
  return RemovePropW (hWnd, (LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EnumPropsEx
#define EnumPropsEx error_use_qxeEnumPropsEx_or_EnumPropsExA_and_EnumPropsExW
#endif
/* NOTE: // callback fun differs only in string pointer type */
DECLARE_INLINE_HEADER (
int qxeEnumPropsEx (HWND hWnd, PROPENUMPROCEXW lpEnumFunc, LPARAM lParam)
)
{
  return EnumPropsExW (hWnd, lpEnumFunc, lParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EnumProps
#define EnumProps error_use_qxeEnumProps_or_EnumPropsA_and_EnumPropsW
#endif
/* NOTE: // callback fun differs only in string pointer type */
DECLARE_INLINE_HEADER (
int qxeEnumProps (HWND hWnd, PROPENUMPROCW lpEnumFunc)
)
{
  return EnumPropsW (hWnd, lpEnumFunc);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetWindowText
#define SetWindowText error_use_qxeSetWindowText_or_SetWindowTextA_and_SetWindowTextW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetWindowText (HWND hWnd, const Extbyte * lpString)
)
{
  return SetWindowTextW (hWnd, (LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetWindowText
#define GetWindowText error_use_qxeGetWindowText_or_GetWindowTextA_and_GetWindowTextW
#endif
DECLARE_INLINE_HEADER (
int qxeGetWindowText (HWND hWnd, Extbyte * lpString, int nMaxCount)
)
{
  return GetWindowTextW (hWnd, (LPWSTR) lpString, nMaxCount);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetWindowTextLength
#define GetWindowTextLength error_use_qxeGetWindowTextLength_or_GetWindowTextLengthA_and_GetWindowTextLengthW
#endif
DECLARE_INLINE_HEADER (
int qxeGetWindowTextLength (HWND hWnd)
)
{
  return GetWindowTextLengthW (hWnd);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef MessageBox
#define MessageBox error_use_qxeMessageBox_or_MessageBoxA_and_MessageBoxW
#endif
DECLARE_INLINE_HEADER (
int qxeMessageBox (HWND hWnd, const Extbyte * lpText, const Extbyte * lpCaption, UINT uType)
)
{
  return MessageBoxW (hWnd, (LPCWSTR) lpText, (LPCWSTR) lpCaption, uType);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef MessageBoxEx
#define MessageBoxEx error_use_qxeMessageBoxEx_or_MessageBoxExA_and_MessageBoxExW
#endif
DECLARE_INLINE_HEADER (
int qxeMessageBoxEx (HWND hWnd, const Extbyte * lpText, const Extbyte * lpCaption, UINT uType, WORD wLanguageId)
)
{
  return MessageBoxExW (hWnd, (LPCWSTR) lpText, (LPCWSTR) lpCaption, uType, wLanguageId);
}

#undef MessageBoxIndirect
#define MessageBoxIndirect error_Cygwin_has_split_MSGBOXPARAMS__instead_of_LPMSGBOXPARAMS

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetWindowLong
#define GetWindowLong error_use_qxeGetWindowLong_or_GetWindowLongA_and_GetWindowLongW
#endif
DECLARE_INLINE_HEADER (
LONG qxeGetWindowLong (HWND hWnd, int nIndex)
)
{
  return GetWindowLongW (hWnd, nIndex);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetWindowLong
#define SetWindowLong error_use_qxeSetWindowLong_or_SetWindowLongA_and_SetWindowLongW
#endif
DECLARE_INLINE_HEADER (
LONG qxeSetWindowLong (HWND hWnd, int nIndex, LONG dwNewLong)
)
{
  return SetWindowLongW (hWnd, nIndex, dwNewLong);
}

/* Skipping GetWindowLongPtr because macro vs. function depending on arch, prefer function */

/* Skipping SetWindowLongPtr because macro vs. function depending on arch, prefer function */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetClassLong
#define GetClassLong error_use_qxeGetClassLong_or_GetClassLongA_and_GetClassLongW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetClassLong (HWND hWnd, int nIndex)
)
{
  return GetClassLongW (hWnd, nIndex);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetClassLong
#define SetClassLong error_use_qxeSetClassLong_or_SetClassLongA_and_SetClassLongW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeSetClassLong (HWND hWnd, int nIndex, LONG dwNewLong)
)
{
  return SetClassLongW (hWnd, nIndex, dwNewLong);
}

/* Skipping GetClassLongPtr because macro vs. function depending on arch, prefer function */

/* Skipping SetClassLongPtr because macro vs. function depending on arch, prefer function */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FindWindow
#define FindWindow error_use_qxeFindWindow_or_FindWindowA_and_FindWindowW
#endif
DECLARE_INLINE_HEADER (
HWND qxeFindWindow (const Extbyte * lpClassName, const Extbyte * lpWindowName)
)
{
  return FindWindowW ((LPCWSTR) lpClassName, (LPCWSTR) lpWindowName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FindWindowEx
#define FindWindowEx error_use_qxeFindWindowEx_or_FindWindowExA_and_FindWindowExW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
HWND qxeFindWindowEx (HWND hWndParent, HWND hWndChildAfter, const Extbyte * lpszClass, const Extbyte * lpszWindow)
)
{
  return FindWindowExW (hWndParent, hWndChildAfter, (LPCWSTR) lpszClass, (LPCWSTR) lpszWindow);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetClassName
#define GetClassName error_use_qxeGetClassName_or_GetClassNameA_and_GetClassNameW
#endif
DECLARE_INLINE_HEADER (
int qxeGetClassName (HWND hWnd, Extbyte * lpClassName, int nMaxCount)
)
{
  return GetClassNameW (hWnd, (LPWSTR) lpClassName, nMaxCount);
}

#undef SetWindowsHook
#define SetWindowsHook error_obsolete__two_versions__STRICT_and_non_STRICT

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetWindowsHookEx
#define SetWindowsHookEx error_use_qxeSetWindowsHookEx_or_SetWindowsHookExA_and_SetWindowsHookExW
#endif
DECLARE_INLINE_HEADER (
HHOOK qxeSetWindowsHookEx (int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId)
)
{
  return SetWindowsHookExW (idHook, lpfn, hmod, dwThreadId);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadBitmap
#define LoadBitmap error_use_qxeLoadBitmap_or_LoadBitmapA_and_LoadBitmapW
#endif
DECLARE_INLINE_HEADER (
HBITMAP qxeLoadBitmap (HINSTANCE hInstance, const Extbyte * lpBitmapName)
)
{
  return LoadBitmapW (hInstance, (LPCWSTR) lpBitmapName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadCursor
#define LoadCursor error_use_qxeLoadCursor_or_LoadCursorA_and_LoadCursorW
#endif
DECLARE_INLINE_HEADER (
HCURSOR qxeLoadCursor (HINSTANCE hInstance, const Extbyte * lpCursorName)
)
{
  return LoadCursorW (hInstance, (LPCWSTR) lpCursorName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadCursorFromFile
#define LoadCursorFromFile error_use_qxeLoadCursorFromFile_or_LoadCursorFromFileA_and_LoadCursorFromFileW
#endif
DECLARE_INLINE_HEADER (
HCURSOR qxeLoadCursorFromFile (const Extbyte * lpFileName)
)
{
  return LoadCursorFromFileW ((LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadIcon
#define LoadIcon error_use_qxeLoadIcon_or_LoadIconA_and_LoadIconW
#endif
DECLARE_INLINE_HEADER (
HICON qxeLoadIcon (HINSTANCE hInstance, const Extbyte * lpIconName)
)
{
  return LoadIconW (hInstance, (LPCWSTR) lpIconName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LoadImage
#define LoadImage error_use_qxeLoadImage_or_LoadImageA_and_LoadImageW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
HANDLE qxeLoadImage (HINSTANCE hInst, const Extbyte * name, UINT type, int cx, int cy, UINT fuLoad)
)
{
  return LoadImageW (hInst, (LPCWSTR) name, type, cx, cy, fuLoad);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef IsDialogMessage
#define IsDialogMessage error_use_qxeIsDialogMessage_or_IsDialogMessageA_and_IsDialogMessageW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeIsDialogMessage (HWND hDlg, LPMSG lpMsg)
)
{
  return IsDialogMessageW (hDlg, lpMsg);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DlgDirList
#define DlgDirList error_use_qxeDlgDirList_or_DlgDirListA_and_DlgDirListW
#endif
DECLARE_INLINE_HEADER (
int qxeDlgDirList (HWND hDlg, Extbyte * lpPathSpec, int nIDListBox, int nIDStaticPath, UINT uFileType)
)
{
  return DlgDirListW (hDlg, (LPWSTR) lpPathSpec, nIDListBox, nIDStaticPath, uFileType);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DlgDirSelectEx
#define DlgDirSelectEx error_use_qxeDlgDirSelectEx_or_DlgDirSelectExA_and_DlgDirSelectExW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeDlgDirSelectEx (HWND hwndDlg, Extbyte * lpString, int chCount, int idListBox)
)
{
  return DlgDirSelectExW (hwndDlg, (LPWSTR) lpString, chCount, idListBox);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DlgDirListComboBox
#define DlgDirListComboBox error_use_qxeDlgDirListComboBox_or_DlgDirListComboBoxA_and_DlgDirListComboBoxW
#endif
DECLARE_INLINE_HEADER (
int qxeDlgDirListComboBox (HWND hDlg, Extbyte * lpPathSpec, int nIDComboBox, int nIDStaticPath, UINT uFiletype)
)
{
  return DlgDirListComboBoxW (hDlg, (LPWSTR) lpPathSpec, nIDComboBox, nIDStaticPath, uFiletype);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DlgDirSelectComboBoxEx
#define DlgDirSelectComboBoxEx error_use_qxeDlgDirSelectComboBoxEx_or_DlgDirSelectComboBoxExA_and_DlgDirSelectComboBoxExW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeDlgDirSelectComboBoxEx (HWND hwndDlg, Extbyte * lpString, int cchOut, int idComboBox)
)
{
  return DlgDirSelectComboBoxExW (hwndDlg, (LPWSTR) lpString, cchOut, idComboBox);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DefFrameProc
#define DefFrameProc error_use_qxeDefFrameProc_or_DefFrameProcA_and_DefFrameProcW
#endif
DECLARE_INLINE_HEADER (
LRESULT qxeDefFrameProc (HWND hWnd, HWND hWndMDIClient, UINT uMsg, WPARAM wParam, LPARAM lParam)
)
{
  return DefFrameProcW (hWnd, hWndMDIClient, uMsg, wParam, lParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DefMDIChildProc
#define DefMDIChildProc error_use_qxeDefMDIChildProc_or_DefMDIChildProcA_and_DefMDIChildProcW
#endif
DECLARE_INLINE_HEADER (
LRESULT qxeDefMDIChildProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
)
{
  return DefMDIChildProcW (hWnd, uMsg, wParam, lParam);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateMDIWindow
#define CreateMDIWindow error_use_qxeCreateMDIWindow_or_CreateMDIWindowA_and_CreateMDIWindowW
#endif
/* NOTE: error arg 1, VS6 prototype, missing const.
   NOTE: Prototype manually overridden.
         Header file claims:
           WINUSERAPI HWND WINAPI CreateMDIWindow(LPCWSTR lpClassName,LPCWSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HINSTANCE hInstance,LPARAM lParam)
         Overridden with:
           HWND CreateMDIWindow(LPWSTR,LPWSTR,DWORD,int,int,int,int,HWND,HINSTANCE,LPARAM)
         Differences in return-type qualifiers, e.g. WINAPI, are not important.
 */
DECLARE_INLINE_HEADER (
HWND qxeCreateMDIWindow (Extbyte * arg1, Extbyte * arg2, DWORD arg3, int arg4, int arg5, int arg6, int arg7, HWND arg8, HINSTANCE arg9, LPARAM arg10)
)
{
  return CreateMDIWindowW ((LPWSTR) arg1, (LPWSTR) arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WinHelp
#define WinHelp error_use_qxeWinHelp_or_WinHelpA_and_WinHelpW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWinHelp (HWND hWndMain, const Extbyte * lpszHelp, UINT uCommand, ULONG_PTR dwData)
)
{
  return WinHelpW (hWndMain, (LPCWSTR) lpszHelp, uCommand, dwData);
}

#undef ChangeDisplaySettings
#define ChangeDisplaySettings error_split_sized_LPDEVMODE

#undef ChangeDisplaySettingsEx
#define ChangeDisplaySettingsEx error_split_sized_LPDEVMODE__NT_5_0_Win98__only

#undef EnumDisplaySettings
#define EnumDisplaySettings error_split_sized_LPDEVMODE

#undef EnumDisplaySettingsEx
#define EnumDisplaySettingsEx error_Function_needs_review_to_determine_how_to_handle_it

#undef EnumDisplayDevices
#define EnumDisplayDevices error_split_sized_PDISPLAY_DEVICE__NT_5_0__only__no_Win98

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SystemParametersInfo
#define SystemParametersInfo error_use_qxeSystemParametersInfo_or_SystemParametersInfoA_and_SystemParametersInfoW
#endif
/* NOTE: probs w/ICONMETRICS, NONCLIENTMETRICS */
DECLARE_INLINE_HEADER (
BOOL qxeSystemParametersInfo (UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
)
{
  return SystemParametersInfoW (uiAction, uiParam, pvParam, fWinIni);
}

#undef GetMonitorInfo
#define GetMonitorInfo error_NT_5_0_Win98__only

#undef GetWindowModuleFileName
#define GetWindowModuleFileName error_NT_5_0__only

#undef RealGetWindowClass
#define RealGetWindowClass error_NT_5_0__only

#undef GetAltTabInfo
#define GetAltTabInfo error_NT_5_0__only

#undef GetRawInputDeviceInfo
#define GetRawInputDeviceInfo error_Function_needs_review_to_determine_how_to_handle_it


/* Processing file DDEML.H */


/*----------------------------------------------------------------------*/
/*                       Processing file DDEML.H                        */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DdeInitialize
#define DdeInitialize error_use_qxeDdeInitialize_or_DdeInitializeA_and_DdeInitializeW
#endif
DECLARE_INLINE_HEADER (
UINT qxeDdeInitialize (LPDWORD pidInst, PFNCALLBACK pfnCallback, DWORD afCmd, DWORD ulRes)
)
{
  return DdeInitializeW (pidInst, pfnCallback, afCmd, ulRes);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DdeCreateStringHandle
#define DdeCreateStringHandle error_use_qxeDdeCreateStringHandle_or_DdeCreateStringHandleA_and_DdeCreateStringHandleW
#endif
/* NOTE: former error in Cygwin prototype, but no more (Cygwin 1.7, 1-30-10) */
DECLARE_INLINE_HEADER (
HSZ qxeDdeCreateStringHandle (DWORD idInst, const Extbyte * psz, int iCodePage)
)
{
  return DdeCreateStringHandleW (idInst, (LPCWSTR) psz, iCodePage);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DdeQueryString
#define DdeQueryString error_use_qxeDdeQueryString_or_DdeQueryStringA_and_DdeQueryStringW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeDdeQueryString (DWORD idInst, HSZ hsz, Extbyte * psz, DWORD cchMax, int iCodePage)
)
{
  return DdeQueryStringW (idInst, hsz, (LPWSTR) psz, cchMax, iCodePage);
}


/* Processing file debugapi.h */


/*----------------------------------------------------------------------*/
/*                      Processing file debugapi.h                      */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OutputDebugString
#define OutputDebugString error_use_qxeOutputDebugString_or_OutputDebugStringA_and_OutputDebugStringW
#endif
DECLARE_INLINE_HEADER (
VOID qxeOutputDebugString (const Extbyte * lpOutputString)
)
{
  OutputDebugStringW ((LPCWSTR) lpOutputString);
}


/* Processing file namedpipeapi.h */


/*----------------------------------------------------------------------*/
/*                    Processing file namedpipeapi.h                    */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateNamedPipe
#define CreateNamedPipe error_use_qxeCreateNamedPipe_or_CreateNamedPipeA_and_CreateNamedPipeW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateNamedPipe (const Extbyte * lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
)
{
  return CreateNamedPipeW ((LPCWSTR) lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WaitNamedPipe
#define WaitNamedPipe error_use_qxeWaitNamedPipe_or_WaitNamedPipeA_and_WaitNamedPipeW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWaitNamedPipe (const Extbyte * lpNamedPipeName, DWORD nTimeOut)
)
{
  return WaitNamedPipeW ((LPCWSTR) lpNamedPipeName, nTimeOut);
}


/* Processing file WINBASE.H */


/*----------------------------------------------------------------------*/
/*                      Processing file WINBASE.H                       */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetBinaryType
#define GetBinaryType error_use_qxeGetBinaryType_or_GetBinaryTypeA_and_GetBinaryTypeW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetBinaryType (const Extbyte * lpApplicationName, LPDWORD lpBinaryType)
)
{
  return GetBinaryTypeW ((LPCWSTR) lpApplicationName, lpBinaryType);
}

#undef SetFileShortName
#define SetFileShortName error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FormatMessage
#define FormatMessage error_use_qxeFormatMessage_or_FormatMessageA_and_FormatMessageW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeFormatMessage (DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, Extbyte * lpBuffer, DWORD nSize, va_list * Arguments)
)
{
  return FormatMessageW (dwFlags, lpSource, dwMessageId, dwLanguageId, (LPWSTR) lpBuffer, nSize, Arguments);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateMailslot
#define CreateMailslot error_use_qxeCreateMailslot_or_CreateMailslotA_and_CreateMailslotW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateMailslot (const Extbyte * lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
)
{
  return CreateMailslotW ((LPCWSTR) lpName, nMaxMessageSize, lReadTimeout, lpSecurityAttributes);
}

#if !defined (CYGWIN_HEADERS)
#undef EncryptFile
#define EncryptFile error_Win2K__only
#endif /* !defined (CYGWIN_HEADERS) */

#if !defined (CYGWIN_HEADERS)
#undef DecryptFile
#define DecryptFile error_Win2K__only
#endif /* !defined (CYGWIN_HEADERS) */

#undef FileEncryptionStatus
#define FileEncryptionStatus error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef lstrcmp
#define lstrcmp error_use_qxelstrcmp_or_lstrcmpA_and_lstrcmpW
#endif
DECLARE_INLINE_HEADER (
int qxelstrcmp (const Extbyte * lpString1, const Extbyte * lpString2)
)
{
  return lstrcmpW ((LPCWSTR) lpString1, (LPCWSTR) lpString2);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef lstrcmpi
#define lstrcmpi error_use_qxelstrcmpi_or_lstrcmpiA_and_lstrcmpiW
#endif
DECLARE_INLINE_HEADER (
int qxelstrcmpi (const Extbyte * lpString1, const Extbyte * lpString2)
)
{
  return lstrcmpiW ((LPCWSTR) lpString1, (LPCWSTR) lpString2);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef lstrcpyn
#define lstrcpyn error_use_qxelstrcpyn_or_lstrcpynA_and_lstrcpynW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxelstrcpyn (Extbyte * lpString1, const Extbyte * lpString2, int iMaxLength)
)
{
  return (Extbyte *) lstrcpynW ((LPWSTR) lpString1, (LPCWSTR) lpString2, iMaxLength);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef lstrcpy
#define lstrcpy error_use_qxelstrcpy_or_lstrcpyA_and_lstrcpyW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxelstrcpy (Extbyte * lpString1, const Extbyte * lpString2)
)
{
  return (Extbyte *) lstrcpyW ((LPWSTR) lpString1, (LPCWSTR) lpString2);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef lstrcat
#define lstrcat error_use_qxelstrcat_or_lstrcatA_and_lstrcatW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxelstrcat (Extbyte * lpString1, const Extbyte * lpString2)
)
{
  return (Extbyte *) lstrcatW ((LPWSTR) lpString1, (LPCWSTR) lpString2);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef lstrlen
#define lstrlen error_use_qxelstrlen_or_lstrlenA_and_lstrlenW
#endif
DECLARE_INLINE_HEADER (
int qxelstrlen (const Extbyte * lpString)
)
{
  return lstrlenW ((LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateSemaphore
#define CreateSemaphore error_use_qxeCreateSemaphore_or_CreateSemaphoreA_and_CreateSemaphoreW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateSemaphore (LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, const Extbyte * lpName)
)
{
  return CreateSemaphoreW (lpSemaphoreAttributes, lInitialCount, lMaximumCount, (LPCWSTR) lpName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateWaitableTimer
#define CreateWaitableTimer error_use_qxeCreateWaitableTimer_or_CreateWaitableTimerA_and_CreateWaitableTimerW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateWaitableTimer (LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, const Extbyte * lpTimerName)
)
{
  return CreateWaitableTimerW (lpTimerAttributes, bManualReset, (LPCWSTR) lpTimerName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FatalAppExit
#define FatalAppExit error_use_qxeFatalAppExit_or_FatalAppExitA_and_FatalAppExitW
#endif
DECLARE_INLINE_HEADER (
VOID qxeFatalAppExit (UINT uAction, const Extbyte * lpMessageText)
)
{
  FatalAppExitW (uAction, (LPCWSTR) lpMessageText);
}

/* Skipping EnumResourceTypes because different prototypes in VC6 and VC7 */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef BeginUpdateResource
#define BeginUpdateResource error_use_qxeBeginUpdateResource_or_BeginUpdateResourceA_and_BeginUpdateResourceW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeBeginUpdateResource (const Extbyte * pFileName, BOOL bDeleteExistingResources)
)
{
  return BeginUpdateResourceW ((LPCWSTR) pFileName, bDeleteExistingResources);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef UpdateResource
#define UpdateResource error_use_qxeUpdateResource_or_UpdateResourceA_and_UpdateResourceW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeUpdateResource (HANDLE hUpdate, const Extbyte * lpType, const Extbyte * lpName, WORD wLanguage, LPVOID lpData, DWORD cb)
)
{
  return UpdateResourceW (hUpdate, (LPCWSTR) lpType, (LPCWSTR) lpName, wLanguage, lpData, cb);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef EndUpdateResource
#define EndUpdateResource error_use_qxeEndUpdateResource_or_EndUpdateResourceA_and_EndUpdateResourceW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeEndUpdateResource (HANDLE hUpdate, BOOL fDiscard)
)
{
  return EndUpdateResourceW (hUpdate, fDiscard);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GlobalAddAtom
#define GlobalAddAtom error_use_qxeGlobalAddAtom_or_GlobalAddAtomA_and_GlobalAddAtomW
#endif
DECLARE_INLINE_HEADER (
ATOM qxeGlobalAddAtom (const Extbyte * lpString)
)
{
  return GlobalAddAtomW ((LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GlobalFindAtom
#define GlobalFindAtom error_use_qxeGlobalFindAtom_or_GlobalFindAtomA_and_GlobalFindAtomW
#endif
DECLARE_INLINE_HEADER (
ATOM qxeGlobalFindAtom (const Extbyte * lpString)
)
{
  return GlobalFindAtomW ((LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GlobalGetAtomName
#define GlobalGetAtomName error_use_qxeGlobalGetAtomName_or_GlobalGetAtomNameA_and_GlobalGetAtomNameW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGlobalGetAtomName (ATOM nAtom, Extbyte * lpBuffer, int nSize)
)
{
  return GlobalGetAtomNameW (nAtom, (LPWSTR) lpBuffer, nSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef AddAtom
#define AddAtom error_use_qxeAddAtom_or_AddAtomA_and_AddAtomW
#endif
DECLARE_INLINE_HEADER (
ATOM qxeAddAtom (const Extbyte * lpString)
)
{
  return AddAtomW ((LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FindAtom
#define FindAtom error_use_qxeFindAtom_or_FindAtomA_and_FindAtomW
#endif
DECLARE_INLINE_HEADER (
ATOM qxeFindAtom (const Extbyte * lpString)
)
{
  return FindAtomW ((LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetAtomName
#define GetAtomName error_use_qxeGetAtomName_or_GetAtomNameA_and_GetAtomNameW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetAtomName (ATOM nAtom, Extbyte * lpBuffer, int nSize)
)
{
  return GetAtomNameW (nAtom, (LPWSTR) lpBuffer, nSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetProfileInt
#define GetProfileInt error_use_qxeGetProfileInt_or_GetProfileIntA_and_GetProfileIntW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetProfileInt (const Extbyte * lpAppName, const Extbyte * lpKeyName, INT nDefault)
)
{
  return GetProfileIntW ((LPCWSTR) lpAppName, (LPCWSTR) lpKeyName, nDefault);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetProfileString
#define GetProfileString error_use_qxeGetProfileString_or_GetProfileStringA_and_GetProfileStringW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetProfileString (const Extbyte * lpAppName, const Extbyte * lpKeyName, const Extbyte * lpDefault, Extbyte * lpReturnedString, DWORD nSize)
)
{
  return GetProfileStringW ((LPCWSTR) lpAppName, (LPCWSTR) lpKeyName, (LPCWSTR) lpDefault, (LPWSTR) lpReturnedString, nSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WriteProfileString
#define WriteProfileString error_use_qxeWriteProfileString_or_WriteProfileStringA_and_WriteProfileStringW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWriteProfileString (const Extbyte * lpAppName, const Extbyte * lpKeyName, const Extbyte * lpString)
)
{
  return WriteProfileStringW ((LPCWSTR) lpAppName, (LPCWSTR) lpKeyName, (LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetProfileSection
#define GetProfileSection error_use_qxeGetProfileSection_or_GetProfileSectionA_and_GetProfileSectionW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetProfileSection (const Extbyte * lpAppName, Extbyte * lpReturnedString, DWORD nSize)
)
{
  return GetProfileSectionW ((LPCWSTR) lpAppName, (LPWSTR) lpReturnedString, nSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WriteProfileSection
#define WriteProfileSection error_use_qxeWriteProfileSection_or_WriteProfileSectionA_and_WriteProfileSectionW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWriteProfileSection (const Extbyte * lpAppName, const Extbyte * lpString)
)
{
  return WriteProfileSectionW ((LPCWSTR) lpAppName, (LPCWSTR) lpString);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetPrivateProfileInt
#define GetPrivateProfileInt error_use_qxeGetPrivateProfileInt_or_GetPrivateProfileIntA_and_GetPrivateProfileIntW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetPrivateProfileInt (const Extbyte * lpAppName, const Extbyte * lpKeyName, INT nDefault, const Extbyte * lpFileName)
)
{
  return GetPrivateProfileIntW ((LPCWSTR) lpAppName, (LPCWSTR) lpKeyName, nDefault, (LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetPrivateProfileString
#define GetPrivateProfileString error_use_qxeGetPrivateProfileString_or_GetPrivateProfileStringA_and_GetPrivateProfileStringW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetPrivateProfileString (const Extbyte * lpAppName, const Extbyte * lpKeyName, const Extbyte * lpDefault, Extbyte * lpReturnedString, DWORD nSize, const Extbyte * lpFileName)
)
{
  return GetPrivateProfileStringW ((LPCWSTR) lpAppName, (LPCWSTR) lpKeyName, (LPCWSTR) lpDefault, (LPWSTR) lpReturnedString, nSize, (LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WritePrivateProfileString
#define WritePrivateProfileString error_use_qxeWritePrivateProfileString_or_WritePrivateProfileStringA_and_WritePrivateProfileStringW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWritePrivateProfileString (const Extbyte * lpAppName, const Extbyte * lpKeyName, const Extbyte * lpString, const Extbyte * lpFileName)
)
{
  return WritePrivateProfileStringW ((LPCWSTR) lpAppName, (LPCWSTR) lpKeyName, (LPCWSTR) lpString, (LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetPrivateProfileSection
#define GetPrivateProfileSection error_use_qxeGetPrivateProfileSection_or_GetPrivateProfileSectionA_and_GetPrivateProfileSectionW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetPrivateProfileSection (const Extbyte * lpAppName, Extbyte * lpReturnedString, DWORD nSize, const Extbyte * lpFileName)
)
{
  return GetPrivateProfileSectionW ((LPCWSTR) lpAppName, (LPWSTR) lpReturnedString, nSize, (LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WritePrivateProfileSection
#define WritePrivateProfileSection error_use_qxeWritePrivateProfileSection_or_WritePrivateProfileSectionA_and_WritePrivateProfileSectionW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWritePrivateProfileSection (const Extbyte * lpAppName, const Extbyte * lpString, const Extbyte * lpFileName)
)
{
  return WritePrivateProfileSectionW ((LPCWSTR) lpAppName, (LPCWSTR) lpString, (LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetPrivateProfileSectionNames
#define GetPrivateProfileSectionNames error_use_qxeGetPrivateProfileSectionNames_or_GetPrivateProfileSectionNamesA_and_GetPrivateProfileSectionNamesW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetPrivateProfileSectionNames (Extbyte * lpszReturnBuffer, DWORD nSize, const Extbyte * lpFileName)
)
{
  return GetPrivateProfileSectionNamesW ((LPWSTR) lpszReturnBuffer, nSize, (LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetPrivateProfileStruct
#define GetPrivateProfileStruct error_use_qxeGetPrivateProfileStruct_or_GetPrivateProfileStructA_and_GetPrivateProfileStructW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetPrivateProfileStruct (const Extbyte * lpszSection, const Extbyte * lpszKey, LPVOID lpStruct, UINT uSizeStruct, const Extbyte * szFile)
)
{
  return GetPrivateProfileStructW ((LPCWSTR) lpszSection, (LPCWSTR) lpszKey, lpStruct, uSizeStruct, (LPCWSTR) szFile);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WritePrivateProfileStruct
#define WritePrivateProfileStruct error_use_qxeWritePrivateProfileStruct_or_WritePrivateProfileStructA_and_WritePrivateProfileStructW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWritePrivateProfileStruct (const Extbyte * lpszSection, const Extbyte * lpszKey, LPVOID lpStruct, UINT uSizeStruct, const Extbyte * szFile)
)
{
  return WritePrivateProfileStructW ((LPCWSTR) lpszSection, (LPCWSTR) lpszKey, lpStruct, uSizeStruct, (LPCWSTR) szFile);
}

#undef GetSystemWow64Directory
#define GetSystemWow64Directory error_Function_needs_review_to_determine_how_to_handle_it

#undef SetDllDirectory
#define SetDllDirectory error_Function_needs_review_to_determine_how_to_handle_it

#undef GetDllDirectory
#define GetDllDirectory error_Function_needs_review_to_determine_how_to_handle_it

#undef GetFirmwareEnvironmentVariable
#define GetFirmwareEnvironmentVariable error_Function_needs_review_to_determine_how_to_handle_it

#undef SetFirmwareEnvironmentVariable
#define SetFirmwareEnvironmentVariable error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateDirectoryEx
#define CreateDirectoryEx error_use_qxeCreateDirectoryEx_or_CreateDirectoryExA_and_CreateDirectoryExW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCreateDirectoryEx (const Extbyte * lpTemplateDirectory, const Extbyte * lpNewDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
)
{
  return CreateDirectoryExW ((LPCWSTR) lpTemplateDirectory, (LPCWSTR) lpNewDirectory, lpSecurityAttributes);
}

#undef GetCompressedFileSize
#define GetCompressedFileSize error_

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CopyFileEx
#define CopyFileEx error_use_qxeCopyFileEx_or_CopyFileExA_and_CopyFileExW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeCopyFileEx (const Extbyte * lpExistingFileName, const Extbyte * lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags)
)
{
  return CopyFileExW ((LPCWSTR) lpExistingFileName, (LPCWSTR) lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

#undef CheckNameLegalDOS8Dot3
#define CheckNameLegalDOS8Dot3 error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CopyFile
#define CopyFile error_use_qxeCopyFile_or_CopyFileA_and_CopyFileW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCopyFile (const Extbyte * lpExistingFileName, const Extbyte * lpNewFileName, BOOL bFailIfExists)
)
{
  return CopyFileW ((LPCWSTR) lpExistingFileName, (LPCWSTR) lpNewFileName, bFailIfExists);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef MoveFile
#define MoveFile error_use_qxeMoveFile_or_MoveFileA_and_MoveFileW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeMoveFile (const Extbyte * lpExistingFileName, const Extbyte * lpNewFileName)
)
{
  return MoveFileW ((LPCWSTR) lpExistingFileName, (LPCWSTR) lpNewFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef MoveFileEx
#define MoveFileEx error_use_qxeMoveFileEx_or_MoveFileExA_and_MoveFileExW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeMoveFileEx (const Extbyte * lpExistingFileName, const Extbyte * lpNewFileName, DWORD dwFlags)
)
{
  return MoveFileExW ((LPCWSTR) lpExistingFileName, (LPCWSTR) lpNewFileName, dwFlags);
}

#undef MoveFileWithProgress
#define MoveFileWithProgress error_NT_5_0__only

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CallNamedPipe
#define CallNamedPipe error_use_qxeCallNamedPipe_or_CallNamedPipeA_and_CallNamedPipeW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCallNamedPipe (const Extbyte * lpNamedPipeName, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesRead, DWORD nTimeOut)
)
{
  return CallNamedPipeW ((LPCWSTR) lpNamedPipeName, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesRead, nTimeOut);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetNamedPipeHandleState
#define GetNamedPipeHandleState error_use_qxeGetNamedPipeHandleState_or_GetNamedPipeHandleStateA_and_GetNamedPipeHandleStateW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetNamedPipeHandleState (HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances, LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout, Extbyte * lpUserName, DWORD nMaxUserNameSize)
)
{
  return GetNamedPipeHandleStateW (hNamedPipe, lpState, lpCurInstances, lpMaxCollectionCount, lpCollectDataTimeout, (LPWSTR) lpUserName, nMaxUserNameSize);
}

#undef ReplaceFile
#define ReplaceFile error_Function_needs_review_to_determine_how_to_handle_it

#undef CreateHardLink
#define CreateHardLink error_NT_5_0__only

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ClearEventLog
#define ClearEventLog error_use_qxeClearEventLog_or_ClearEventLogA_and_ClearEventLogW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeClearEventLog (HANDLE hEventLog, const Extbyte * lpBackupFileName)
)
{
  return ClearEventLogW (hEventLog, (LPCWSTR) lpBackupFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef BackupEventLog
#define BackupEventLog error_use_qxeBackupEventLog_or_BackupEventLogA_and_BackupEventLogW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeBackupEventLog (HANDLE hEventLog, const Extbyte * lpBackupFileName)
)
{
  return BackupEventLogW (hEventLog, (LPCWSTR) lpBackupFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenEventLog
#define OpenEventLog error_use_qxeOpenEventLog_or_OpenEventLogA_and_OpenEventLogW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeOpenEventLog (const Extbyte * lpUNCServerName, const Extbyte * lpSourceName)
)
{
  return OpenEventLogW ((LPCWSTR) lpUNCServerName, (LPCWSTR) lpSourceName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegisterEventSource
#define RegisterEventSource error_use_qxeRegisterEventSource_or_RegisterEventSourceA_and_RegisterEventSourceW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeRegisterEventSource (const Extbyte * lpUNCServerName, const Extbyte * lpSourceName)
)
{
  return RegisterEventSourceW ((LPCWSTR) lpUNCServerName, (LPCWSTR) lpSourceName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenBackupEventLog
#define OpenBackupEventLog error_use_qxeOpenBackupEventLog_or_OpenBackupEventLogA_and_OpenBackupEventLogW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeOpenBackupEventLog (const Extbyte * lpUNCServerName, const Extbyte * lpFileName)
)
{
  return OpenBackupEventLogW ((LPCWSTR) lpUNCServerName, (LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ReadEventLog
#define ReadEventLog error_use_qxeReadEventLog_or_ReadEventLogA_and_ReadEventLogW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeReadEventLog (HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD * pnBytesRead, DWORD * pnMinNumberOfBytesNeeded)
)
{
  return ReadEventLogW (hEventLog, dwReadFlags, dwRecordOffset, lpBuffer, nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ReportEvent
#define ReportEvent error_use_qxeReportEvent_or_ReportEventA_and_ReportEventW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeReportEvent (HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, const Extbyte * * lpStrings, LPVOID lpRawData)
)
{
  return ReportEventW (hEventLog, wType, wCategory, dwEventID, lpUserSid, wNumStrings, dwDataSize, (LPCWSTR *) lpStrings, lpRawData);
}

#undef ReadDirectoryChanges
#define ReadDirectoryChanges error_Unicode_only

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef IsBadStringPtr
#define IsBadStringPtr error_use_qxeIsBadStringPtr_or_IsBadStringPtrA_and_IsBadStringPtrW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeIsBadStringPtr (const Extbyte * lpsz, UINT_PTR ucchMax)
)
{
  return IsBadStringPtrW ((LPCWSTR) lpsz, ucchMax);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef BuildCommDCB
#define BuildCommDCB error_use_qxeBuildCommDCB_or_BuildCommDCBA_and_BuildCommDCBW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeBuildCommDCB (const Extbyte * lpDef, LPDCB lpDCB)
)
{
  return BuildCommDCBW ((LPCWSTR) lpDef, lpDCB);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef BuildCommDCBAndTimeouts
#define BuildCommDCBAndTimeouts error_use_qxeBuildCommDCBAndTimeouts_or_BuildCommDCBAndTimeoutsA_and_BuildCommDCBAndTimeoutsW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeBuildCommDCBAndTimeouts (const Extbyte * lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
)
{
  return BuildCommDCBAndTimeoutsW ((LPCWSTR) lpDef, lpDCB, lpCommTimeouts);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CommConfigDialog
#define CommConfigDialog error_use_qxeCommConfigDialog_or_CommConfigDialogA_and_CommConfigDialogW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCommConfigDialog (const Extbyte * lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
)
{
  return CommConfigDialogW ((LPCWSTR) lpszName, hWnd, lpCC);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetDefaultCommConfig
#define GetDefaultCommConfig error_use_qxeGetDefaultCommConfig_or_GetDefaultCommConfigA_and_GetDefaultCommConfigW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetDefaultCommConfig (const Extbyte * lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
)
{
  return GetDefaultCommConfigW ((LPCWSTR) lpszName, lpCC, lpdwSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetDefaultCommConfig
#define SetDefaultCommConfig error_use_qxeSetDefaultCommConfig_or_SetDefaultCommConfigA_and_SetDefaultCommConfigW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetDefaultCommConfig (const Extbyte * lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
)
{
  return SetDefaultCommConfigW ((LPCWSTR) lpszName, lpCC, dwSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetComputerName
#define SetComputerName error_use_qxeSetComputerName_or_SetComputerNameA_and_SetComputerNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetComputerName (const Extbyte * lpComputerName)
)
{
  return SetComputerNameW ((LPCWSTR) lpComputerName);
}

#undef DnsHostnameToComputerName
#define DnsHostnameToComputerName error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LogonUser
#define LogonUser error_use_qxeLogonUser_or_LogonUserA_and_LogonUserW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeLogonUser (const Extbyte * lpszUsername, const Extbyte * lpszDomain, const Extbyte * lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken)
)
{
  return LogonUserW ((LPCWSTR) lpszUsername, (LPCWSTR) lpszDomain, (LPCWSTR) lpszPassword, dwLogonType, dwLogonProvider, phToken);
}

#undef CreateProcessWithLogon
#define CreateProcessWithLogon error_Function_needs_review_to_determine_how_to_handle_it

#undef GetCurrentHwProfile
#define GetCurrentHwProfile error_split_sized_LPHW_PROFILE_INFO__NT_4_0__only

#undef VerifyVersionInfo
#define VerifyVersionInfo error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetUserName
#define GetUserName error_use_qxeGetUserName_or_GetUserNameA_and_GetUserNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetUserName (Extbyte * lpBuffer, LPDWORD pcbBuffer)
)
{
  return GetUserNameW ((LPWSTR) lpBuffer, pcbBuffer);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LookupAccountName
#define LookupAccountName error_use_qxeLookupAccountName_or_LookupAccountNameA_and_LookupAccountNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeLookupAccountName (const Extbyte * lpSystemName, const Extbyte * lpAccountName, PSID Sid, LPDWORD cbSid, Extbyte * ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse)
)
{
  return LookupAccountNameW ((LPCWSTR) lpSystemName, (LPCWSTR) lpAccountName, Sid, cbSid, (LPWSTR) ReferencedDomainName, cchReferencedDomainName, peUse);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LookupAccountSid
#define LookupAccountSid error_use_qxeLookupAccountSid_or_LookupAccountSidA_and_LookupAccountSidW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeLookupAccountSid (const Extbyte * lpSystemName, PSID Sid, Extbyte * Name, LPDWORD cchName, Extbyte * ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse)
)
{
  return LookupAccountSidW ((LPCWSTR) lpSystemName, Sid, (LPWSTR) Name, cchName, (LPWSTR) ReferencedDomainName, cchReferencedDomainName, peUse);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LookupPrivilegeDisplayName
#define LookupPrivilegeDisplayName error_use_qxeLookupPrivilegeDisplayName_or_LookupPrivilegeDisplayNameA_and_LookupPrivilegeDisplayNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeLookupPrivilegeDisplayName (const Extbyte * lpSystemName, const Extbyte * lpName, Extbyte * lpDisplayName, LPDWORD cchDisplayName, LPDWORD lpLanguageId)
)
{
  return LookupPrivilegeDisplayNameW ((LPCWSTR) lpSystemName, (LPCWSTR) lpName, (LPWSTR) lpDisplayName, cchDisplayName, lpLanguageId);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LookupPrivilegeName
#define LookupPrivilegeName error_use_qxeLookupPrivilegeName_or_LookupPrivilegeNameA_and_LookupPrivilegeNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeLookupPrivilegeName (const Extbyte * lpSystemName, PLUID lpLuid, Extbyte * lpName, LPDWORD cchName)
)
{
  return LookupPrivilegeNameW ((LPCWSTR) lpSystemName, lpLuid, (LPWSTR) lpName, cchName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef LookupPrivilegeValue
#define LookupPrivilegeValue error_use_qxeLookupPrivilegeValue_or_LookupPrivilegeValueA_and_LookupPrivilegeValueW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeLookupPrivilegeValue (const Extbyte * lpSystemName, const Extbyte * lpName, PLUID lpLuid)
)
{
  return LookupPrivilegeValueW ((LPCWSTR) lpSystemName, (LPCWSTR) lpName, lpLuid);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetVolumeLabel
#define SetVolumeLabel error_use_qxeSetVolumeLabel_or_SetVolumeLabelA_and_SetVolumeLabelW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetVolumeLabel (const Extbyte * lpRootPathName, const Extbyte * lpVolumeName)
)
{
  return SetVolumeLabelW ((LPCWSTR) lpRootPathName, (LPCWSTR) lpVolumeName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetComputerName
#define GetComputerName error_use_qxeGetComputerName_or_GetComputerNameA_and_GetComputerNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetComputerName (Extbyte * lpBuffer, LPDWORD nSize)
)
{
  return GetComputerNameW ((LPWSTR) lpBuffer, nSize);
}

#undef CreateJobObject
#define CreateJobObject error_NT_5_0__only

#undef OpenJobObject
#define OpenJobObject error_NT_5_0__only

#undef FindFirstVolumeMountPoint
#define FindFirstVolumeMountPoint error_Function_needs_review_to_determine_how_to_handle_it

#undef FindNextVolumeMountPoint
#define FindNextVolumeMountPoint error_Function_needs_review_to_determine_how_to_handle_it

#undef SetVolumeMountPoint
#define SetVolumeMountPoint error_Function_needs_review_to_determine_how_to_handle_it

#undef CreateActCtx
#define CreateActCtx error_Function_needs_review_to_determine_how_to_handle_it

#undef FindActCtxSectionString
#define FindActCtxSectionString error_Function_needs_review_to_determine_how_to_handle_it

#undef QueryActCtx
#define QueryActCtx error_Function_needs_review_to_determine_how_to_handle_it


/* Processing file SHELLAPI.H */


/*----------------------------------------------------------------------*/
/*                      Processing file SHELLAPI.H                      */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DragQueryFile
#define DragQueryFile error_use_qxeDragQueryFile_or_DragQueryFileA_and_DragQueryFileW
#endif
DECLARE_INLINE_HEADER (
UINT qxeDragQueryFile (HDROP hDrop, UINT iFile, Extbyte * lpszFile, UINT cch)
)
{
  return DragQueryFileW (hDrop, iFile, (LPWSTR) lpszFile, cch);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ShellExecute
#define ShellExecute error_use_qxeShellExecute_or_ShellExecuteA_and_ShellExecuteW
#endif
DECLARE_INLINE_HEADER (
HINSTANCE qxeShellExecute (HWND hwnd, const Extbyte * lpOperation, const Extbyte * lpFile, const Extbyte * lpParameters, const Extbyte * lpDirectory, INT nShowCmd)
)
{
  return ShellExecuteW (hwnd, (LPCWSTR) lpOperation, (LPCWSTR) lpFile, (LPCWSTR) lpParameters, (LPCWSTR) lpDirectory, nShowCmd);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FindExecutable
#define FindExecutable error_use_qxeFindExecutable_or_FindExecutableA_and_FindExecutableW
#endif
DECLARE_INLINE_HEADER (
HINSTANCE qxeFindExecutable (const Extbyte * lpFile, const Extbyte * lpDirectory, Extbyte * lpResult)
)
{
  return FindExecutableW ((LPCWSTR) lpFile, (LPCWSTR) lpDirectory, (LPWSTR) lpResult);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ShellAbout
#define ShellAbout error_use_qxeShellAbout_or_ShellAboutA_and_ShellAboutW
#endif
DECLARE_INLINE_HEADER (
INT qxeShellAbout (HWND hWnd, const Extbyte * szApp, const Extbyte * szOtherStuff, HICON hIcon)
)
{
  return ShellAboutW (hWnd, (LPCWSTR) szApp, (LPCWSTR) szOtherStuff, hIcon);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ExtractAssociatedIcon
#define ExtractAssociatedIcon error_use_qxeExtractAssociatedIcon_or_ExtractAssociatedIconA_and_ExtractAssociatedIconW
#endif
/* NOTE: error arg2, Cygwin prototype, extra const.
   NOTE: Prototype manually overridden.
         Header file claims:
           SHSTDAPI_(HICON) ExtractAssociatedIcon(HINSTANCE hInst, LPWSTR pszIconPath, WORD *piIcon)
         Overridden with:
           HICON ExtractAssociatedIcon(HINSTANCE, LPWSTR, LPWORD)
         Differences in return-type qualifiers, e.g. WINAPI, are not important.
 */
DECLARE_INLINE_HEADER (
HICON qxeExtractAssociatedIcon (HINSTANCE arg1, Extbyte * arg2, LPWORD arg3)
)
{
  return ExtractAssociatedIconW (arg1, (LPWSTR) arg2, arg3);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ExtractIcon
#define ExtractIcon error_use_qxeExtractIcon_or_ExtractIconA_and_ExtractIconW
#endif
DECLARE_INLINE_HEADER (
HICON qxeExtractIcon (HINSTANCE hInst, const Extbyte * pszExeFileName, UINT nIconIndex)
)
{
  return ExtractIconW (hInst, (LPCWSTR) pszExeFileName, nIconIndex);
}

#if !defined (CYGWIN_HEADERS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DoEnvironmentSubst
#define DoEnvironmentSubst error_use_qxeDoEnvironmentSubst_or_DoEnvironmentSubstA_and_DoEnvironmentSubstW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
DWORD qxeDoEnvironmentSubst (Extbyte * pszSrc, UINT cchSrc)
)
{
  return DoEnvironmentSubstW ((LPWSTR) pszSrc, cchSrc);
}
#endif /* !defined (CYGWIN_HEADERS) */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ExtractIconEx
#define ExtractIconEx error_use_qxeExtractIconEx_or_ExtractIconExA_and_ExtractIconExW
#endif
/* NOTE: NT 4.0+ only, former error in Cygwin prototype but no more (Cygwin 1.7, 1-30-10) */
DECLARE_INLINE_HEADER (
UINT qxeExtractIconEx (const Extbyte * lpszFile, int nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons)
)
{
  return ExtractIconExW ((LPCWSTR) lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SHFileOperation
#define SHFileOperation error_use_qxeSHFileOperation_or_SHFileOperationA_and_SHFileOperationW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
int qxeSHFileOperation (LPSHFILEOPSTRUCTW lpFileOp)
)
{
  return SHFileOperationW (lpFileOp);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ShellExecuteEx
#define ShellExecuteEx error_use_qxeShellExecuteEx_or_ShellExecuteExA_and_ShellExecuteExW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeShellExecuteEx (SHELLEXECUTEINFOW * pExecInfo)
)
{
  return ShellExecuteExW (pExecInfo);
}

#if !defined (CYGWIN_HEADERS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SHQueryRecycleBin
#define SHQueryRecycleBin error_use_qxeSHQueryRecycleBin_or_SHQueryRecycleBinA_and_SHQueryRecycleBinW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
HRESULT qxeSHQueryRecycleBin (const Extbyte * pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
)
{
  return SHQueryRecycleBinW ((LPCWSTR) pszRootPath, pSHQueryRBInfo);
}
#endif /* !defined (CYGWIN_HEADERS) */

#if !defined (CYGWIN_HEADERS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SHEmptyRecycleBin
#define SHEmptyRecycleBin error_use_qxeSHEmptyRecycleBin_or_SHEmptyRecycleBinA_and_SHEmptyRecycleBinW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
HRESULT qxeSHEmptyRecycleBin (HWND hwnd, const Extbyte * pszRootPath, DWORD dwFlags)
)
{
  return SHEmptyRecycleBinW (hwnd, (LPCWSTR) pszRootPath, dwFlags);
}
#endif /* !defined (CYGWIN_HEADERS) */

#undef Shell_NotifyIcon
#define Shell_NotifyIcon error_split_sized_NOTIFYICONDATA__NT_4_0__only

/* Skipping SHGetFileInfo because split-sized SHFILEINFO, NT 4.0+ only */

#if !defined (CYGWIN_HEADERS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SHGetNewLinkInfo
#define SHGetNewLinkInfo error_use_qxeSHGetNewLinkInfo_or_SHGetNewLinkInfoA_and_SHGetNewLinkInfoW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeSHGetNewLinkInfo (const Extbyte * pszLinkTo, const Extbyte * pszDir, Extbyte * pszName, BOOL * pfMustCopy, UINT uFlags)
)
{
  return SHGetNewLinkInfoW ((LPCWSTR) pszLinkTo, (LPCWSTR) pszDir, (LPWSTR) pszName, pfMustCopy, uFlags);
}
#endif /* !defined (CYGWIN_HEADERS) */

#if !defined (CYGWIN_HEADERS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SHInvokePrinterCommand
#define SHInvokePrinterCommand error_use_qxeSHInvokePrinterCommand_or_SHInvokePrinterCommandA_and_SHInvokePrinterCommandW
#endif
/* NOTE: NT 4.0+ only */
DECLARE_INLINE_HEADER (
BOOL qxeSHInvokePrinterCommand (HWND hwnd, UINT uAction, const Extbyte * lpBuf1, const Extbyte * lpBuf2, BOOL fModal)
)
{
  return SHInvokePrinterCommandW (hwnd, uAction, (LPCWSTR) lpBuf1, (LPCWSTR) lpBuf2, fModal);
}
#endif /* !defined (CYGWIN_HEADERS) */


/* Processing file WINNETWK.H */


/*----------------------------------------------------------------------*/
/*                      Processing file WINNETWK.H                      */
/*----------------------------------------------------------------------*/

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetAddConnection
#define WNetAddConnection error_use_qxeWNetAddConnection_or_WNetAddConnectionA_and_WNetAddConnectionW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetAddConnection (const Extbyte * lpRemoteName, const Extbyte * lpPassword, const Extbyte * lpLocalName)
)
{
  return WNetAddConnectionW ((LPCWSTR) lpRemoteName, (LPCWSTR) lpPassword, (LPCWSTR) lpLocalName);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetAddConnection2
#define WNetAddConnection2 error_use_qxeWNetAddConnection2_or_WNetAddConnection2A_and_WNetAddConnection2W
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetAddConnection2 (LPNETRESOURCEW lpNetResource, const Extbyte * lpPassword, const Extbyte * lpUserName, DWORD dwFlags)
)
{
  return WNetAddConnection2W (lpNetResource, (LPCWSTR) lpPassword, (LPCWSTR) lpUserName, dwFlags);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetAddConnection3
#define WNetAddConnection3 error_use_qxeWNetAddConnection3_or_WNetAddConnection3A_and_WNetAddConnection3W
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetAddConnection3 (HWND hwndOwner, LPNETRESOURCEW lpNetResource, const Extbyte * lpPassword, const Extbyte * lpUserName, DWORD dwFlags)
)
{
  return WNetAddConnection3W (hwndOwner, lpNetResource, (LPCWSTR) lpPassword, (LPCWSTR) lpUserName, dwFlags);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetCancelConnection
#define WNetCancelConnection error_use_qxeWNetCancelConnection_or_WNetCancelConnectionA_and_WNetCancelConnectionW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetCancelConnection (const Extbyte * lpName, BOOL fForce)
)
{
  return WNetCancelConnectionW ((LPCWSTR) lpName, fForce);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetCancelConnection2
#define WNetCancelConnection2 error_use_qxeWNetCancelConnection2_or_WNetCancelConnection2A_and_WNetCancelConnection2W
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetCancelConnection2 (const Extbyte * lpName, DWORD dwFlags, BOOL fForce)
)
{
  return WNetCancelConnection2W ((LPCWSTR) lpName, dwFlags, fForce);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetGetConnection
#define WNetGetConnection error_use_qxeWNetGetConnection_or_WNetGetConnectionA_and_WNetGetConnectionW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetGetConnection (const Extbyte * lpLocalName, Extbyte * lpRemoteName, LPDWORD lpnLength)
)
{
  return WNetGetConnectionW ((LPCWSTR) lpLocalName, (LPWSTR) lpRemoteName, lpnLength);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetUseConnection
#define WNetUseConnection error_use_qxeWNetUseConnection_or_WNetUseConnectionA_and_WNetUseConnectionW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetUseConnection (HWND hwndOwner, LPNETRESOURCEW lpNetResource, const Extbyte * lpPassword, const Extbyte * lpUserID, DWORD dwFlags, Extbyte * lpAccessName, LPDWORD lpBufferSize, LPDWORD lpResult)
)
{
  return WNetUseConnectionW (hwndOwner, lpNetResource, (LPCWSTR) lpPassword, (LPCWSTR) lpUserID, dwFlags, (LPWSTR) lpAccessName, lpBufferSize, lpResult);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetConnectionDialog1
#define WNetConnectionDialog1 error_use_qxeWNetConnectionDialog1_or_WNetConnectionDialog1A_and_WNetConnectionDialog1W
#endif
/* NOTE: contains split-simple LPNETRESOURCE */
DECLARE_INLINE_HEADER (
DWORD qxeWNetConnectionDialog1 (LPCONNECTDLGSTRUCTW lpConnDlgStruct)
)
{
  return WNetConnectionDialog1W (lpConnDlgStruct);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetDisconnectDialog1
#define WNetDisconnectDialog1 error_use_qxeWNetDisconnectDialog1_or_WNetDisconnectDialog1A_and_WNetDisconnectDialog1W
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetDisconnectDialog1 (LPDISCDLGSTRUCTW lpConnDlgStruct)
)
{
  return WNetDisconnectDialog1W (lpConnDlgStruct);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetOpenEnum
#define WNetOpenEnum error_use_qxeWNetOpenEnum_or_WNetOpenEnumA_and_WNetOpenEnumW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetOpenEnum (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEW lpNetResource, LPHANDLE lphEnum)
)
{
  return WNetOpenEnumW (dwScope, dwType, dwUsage, lpNetResource, lphEnum);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetEnumResource
#define WNetEnumResource error_use_qxeWNetEnumResource_or_WNetEnumResourceA_and_WNetEnumResourceW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetEnumResource (HANDLE hEnum, LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize)
)
{
  return WNetEnumResourceW (hEnum, lpcCount, lpBuffer, lpBufferSize);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef WNetGetResourceParent
#define WNetGetResourceParent error_Function_needs_review_to_determine_how_to_handle_it
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef WNetGetResourceInformation
#define WNetGetResourceInformation error_Function_needs_review_to_determine_how_to_handle_it
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetGetUniversalName
#define WNetGetUniversalName error_use_qxeWNetGetUniversalName_or_WNetGetUniversalNameA_and_WNetGetUniversalNameW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetGetUniversalName (const Extbyte * lpLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpBufferSize)
)
{
  return WNetGetUniversalNameW ((LPCWSTR) lpLocalPath, dwInfoLevel, lpBuffer, lpBufferSize);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetGetUser
#define WNetGetUser error_use_qxeWNetGetUser_or_WNetGetUserA_and_WNetGetUserW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetGetUser (const Extbyte * lpName, Extbyte * lpUserName, LPDWORD lpnLength)
)
{
  return WNetGetUserW ((LPCWSTR) lpName, (LPWSTR) lpUserName, lpnLength);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetGetProviderName
#define WNetGetProviderName error_use_qxeWNetGetProviderName_or_WNetGetProviderNameA_and_WNetGetProviderNameW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetGetProviderName (DWORD dwNetType, Extbyte * lpProviderName, LPDWORD lpBufferSize)
)
{
  return WNetGetProviderNameW (dwNetType, (LPWSTR) lpProviderName, lpBufferSize);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetGetNetworkInformation
#define WNetGetNetworkInformation error_use_qxeWNetGetNetworkInformation_or_WNetGetNetworkInformationA_and_WNetGetNetworkInformationW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetGetNetworkInformation (const Extbyte * lpProvider, LPNETINFOSTRUCT lpNetInfoStruct)
)
{
  return WNetGetNetworkInformationW ((LPCWSTR) lpProvider, lpNetInfoStruct);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WNetGetLastError
#define WNetGetLastError error_use_qxeWNetGetLastError_or_WNetGetLastErrorA_and_WNetGetLastErrorW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeWNetGetLastError (LPDWORD lpError, Extbyte * lpErrorBuf, DWORD nErrorBufSize, Extbyte * lpNameBuf, DWORD nNameBufSize)
)
{
  return WNetGetLastErrorW (lpError, (LPWSTR) lpErrorBuf, nErrorBufSize, (LPWSTR) lpNameBuf, nNameBufSize);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef MultinetGetConnectionPerformance
#define MultinetGetConnectionPerformance error_use_qxeMultinetGetConnectionPerformance_or_MultinetGetConnectionPerformanceA_and_MultinetGetConnectionPerformanceW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeMultinetGetConnectionPerformance (LPNETRESOURCEW lpNetResource, LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct)
)
{
  return MultinetGetConnectionPerformanceW (lpNetResource, lpNetConnectInfoStruct);
}
#endif /* defined (HAVE_MS_WINDOWS) */


/* Processing file securitybaseapi.h */


/*----------------------------------------------------------------------*/
/*                  Processing file securitybaseapi.h                   */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef AccessCheckAndAuditAlarm
#define AccessCheckAndAuditAlarm error_use_qxeAccessCheckAndAuditAlarm_or_AccessCheckAndAuditAlarmA_and_AccessCheckAndAuditAlarmW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeAccessCheckAndAuditAlarm (const Extbyte * SubsystemName, LPVOID HandleId, Extbyte * ObjectTypeName, Extbyte * ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose)
)
{
  return AccessCheckAndAuditAlarmW ((LPCWSTR) SubsystemName, HandleId, (LPWSTR) ObjectTypeName, (LPWSTR) ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetFileSecurity
#define GetFileSecurity error_use_qxeGetFileSecurity_or_GetFileSecurityA_and_GetFileSecurityW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetFileSecurity (const Extbyte * lpFileName, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded)
)
{
  return GetFileSecurityW ((LPCWSTR) lpFileName, RequestedInformation, pSecurityDescriptor, nLength, lpnLengthNeeded);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ObjectCloseAuditAlarm
#define ObjectCloseAuditAlarm error_use_qxeObjectCloseAuditAlarm_or_ObjectCloseAuditAlarmA_and_ObjectCloseAuditAlarmW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeObjectCloseAuditAlarm (const Extbyte * SubsystemName, LPVOID HandleId, BOOL GenerateOnClose)
)
{
  return ObjectCloseAuditAlarmW ((LPCWSTR) SubsystemName, HandleId, GenerateOnClose);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ObjectDeleteAuditAlarm
#define ObjectDeleteAuditAlarm error_use_qxeObjectDeleteAuditAlarm_or_ObjectDeleteAuditAlarmA_and_ObjectDeleteAuditAlarmW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeObjectDeleteAuditAlarm (const Extbyte * SubsystemName, LPVOID HandleId, BOOL GenerateOnClose)
)
{
  return ObjectDeleteAuditAlarmW ((LPCWSTR) SubsystemName, HandleId, GenerateOnClose);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ObjectOpenAuditAlarm
#define ObjectOpenAuditAlarm error_use_qxeObjectOpenAuditAlarm_or_ObjectOpenAuditAlarmA_and_ObjectOpenAuditAlarmW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeObjectOpenAuditAlarm (const Extbyte * SubsystemName, LPVOID HandleId, Extbyte * ObjectTypeName, Extbyte * ObjectName, PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess, DWORD GrantedAccess, PPRIVILEGE_SET Privileges, BOOL ObjectCreation, BOOL AccessGranted, LPBOOL GenerateOnClose)
)
{
  return ObjectOpenAuditAlarmW ((LPCWSTR) SubsystemName, HandleId, (LPWSTR) ObjectTypeName, (LPWSTR) ObjectName, pSecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ObjectPrivilegeAuditAlarm
#define ObjectPrivilegeAuditAlarm error_use_qxeObjectPrivilegeAuditAlarm_or_ObjectPrivilegeAuditAlarmA_and_ObjectPrivilegeAuditAlarmW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeObjectPrivilegeAuditAlarm (const Extbyte * SubsystemName, LPVOID HandleId, HANDLE ClientToken, DWORD DesiredAccess, PPRIVILEGE_SET Privileges, BOOL AccessGranted)
)
{
  return ObjectPrivilegeAuditAlarmW ((LPCWSTR) SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PrivilegedServiceAuditAlarm
#define PrivilegedServiceAuditAlarm error_use_qxePrivilegedServiceAuditAlarm_or_PrivilegedServiceAuditAlarmA_and_PrivilegedServiceAuditAlarmW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePrivilegedServiceAuditAlarm (const Extbyte * SubsystemName, const Extbyte * ServiceName, HANDLE ClientToken, PPRIVILEGE_SET Privileges, BOOL AccessGranted)
)
{
  return PrivilegedServiceAuditAlarmW ((LPCWSTR) SubsystemName, (LPCWSTR) ServiceName, ClientToken, Privileges, AccessGranted);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetFileSecurity
#define SetFileSecurity error_use_qxeSetFileSecurity_or_SetFileSecurityA_and_SetFileSecurityW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetFileSecurity (const Extbyte * lpFileName, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor)
)
{
  return SetFileSecurityW ((LPCWSTR) lpFileName, SecurityInformation, pSecurityDescriptor);
}


/* Processing file FILEAPI.H */


/*----------------------------------------------------------------------*/
/*                      Processing file FILEAPI.H                       */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetFileAttributes
#define GetFileAttributes error_use_qxeGetFileAttributes_or_GetFileAttributesA_and_GetFileAttributesW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetFileAttributes (const Extbyte * lpFileName)
)
{
  return GetFileAttributesW ((LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DefineDosDevice
#define DefineDosDevice error_use_qxeDefineDosDevice_or_DefineDosDeviceA_and_DefineDosDeviceW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeDefineDosDevice (DWORD dwFlags, const Extbyte * lpDeviceName, const Extbyte * lpTargetPath)
)
{
  return DefineDosDeviceW (dwFlags, (LPCWSTR) lpDeviceName, (LPCWSTR) lpTargetPath);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FindFirstChangeNotification
#define FindFirstChangeNotification error_use_qxeFindFirstChangeNotification_or_FindFirstChangeNotificationA_and_FindFirstChangeNotificationW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeFindFirstChangeNotification (const Extbyte * lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter)
)
{
  return FindFirstChangeNotificationW ((LPCWSTR) lpPathName, bWatchSubtree, dwNotifyFilter);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateFile
#define CreateFile error_use_qxeCreateFile_or_CreateFileA_and_CreateFileW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateFile (const Extbyte * lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
)
{
  return CreateFileW ((LPCWSTR) lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetDiskFreeSpace
#define GetDiskFreeSpace error_use_qxeGetDiskFreeSpace_or_GetDiskFreeSpaceA_and_GetDiskFreeSpaceW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetDiskFreeSpace (const Extbyte * lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters)
)
{
  return GetDiskFreeSpaceW ((LPCWSTR) lpRootPathName, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetDriveType
#define GetDriveType error_use_qxeGetDriveType_or_GetDriveTypeA_and_GetDriveTypeW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetDriveType (const Extbyte * lpRootPathName)
)
{
  return GetDriveTypeW ((LPCWSTR) lpRootPathName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetFullPathName
#define GetFullPathName error_use_qxeGetFullPathName_or_GetFullPathNameA_and_GetFullPathNameW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetFullPathName (const Extbyte * lpFileName, DWORD nBufferLength, Extbyte * lpBuffer, Extbyte * * lpFilePart)
)
{
  return GetFullPathNameW ((LPCWSTR) lpFileName, nBufferLength, (LPWSTR) lpBuffer, (LPWSTR *) lpFilePart);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetLogicalDriveStrings
#define GetLogicalDriveStrings error_use_qxeGetLogicalDriveStrings_or_GetLogicalDriveStringsA_and_GetLogicalDriveStringsW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetLogicalDriveStrings (DWORD nBufferLength, Extbyte * lpBuffer)
)
{
  return GetLogicalDriveStringsW (nBufferLength, (LPWSTR) lpBuffer);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetShortPathName
#define GetShortPathName error_use_qxeGetShortPathName_or_GetShortPathNameA_and_GetShortPathNameW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetShortPathName (const Extbyte * lpszLongPath, Extbyte * lpszShortPath, DWORD cchBuffer)
)
{
  return GetShortPathNameW ((LPCWSTR) lpszLongPath, (LPWSTR) lpszShortPath, cchBuffer);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef QueryDosDevice
#define QueryDosDevice error_use_qxeQueryDosDevice_or_QueryDosDeviceA_and_QueryDosDeviceW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeQueryDosDevice (const Extbyte * lpDeviceName, Extbyte * lpTargetPath, DWORD ucchMax)
)
{
  return QueryDosDeviceW ((LPCWSTR) lpDeviceName, (LPWSTR) lpTargetPath, ucchMax);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTempFileName
#define GetTempFileName error_use_qxeGetTempFileName_or_GetTempFileNameA_and_GetTempFileNameW
#endif
DECLARE_INLINE_HEADER (
UINT qxeGetTempFileName (const Extbyte * lpPathName, const Extbyte * lpPrefixString, UINT uUnique, Extbyte * lpTempFileName)
)
{
  return GetTempFileNameW ((LPCWSTR) lpPathName, (LPCWSTR) lpPrefixString, uUnique, (LPWSTR) lpTempFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetVolumeInformation
#define GetVolumeInformation error_use_qxeGetVolumeInformation_or_GetVolumeInformationA_and_GetVolumeInformationW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetVolumeInformation (const Extbyte * lpRootPathName, Extbyte * lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, Extbyte * lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
)
{
  return GetVolumeInformationW ((LPCWSTR) lpRootPathName, (LPWSTR) lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber, lpMaximumComponentLength, lpFileSystemFlags, (LPWSTR) lpFileSystemNameBuffer, nFileSystemNameSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateDirectory
#define CreateDirectory error_use_qxeCreateDirectory_or_CreateDirectoryA_and_CreateDirectoryW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCreateDirectory (const Extbyte * lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
)
{
  return CreateDirectoryW ((LPCWSTR) lpPathName, lpSecurityAttributes);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef DeleteFile
#define DeleteFile error_use_qxeDeleteFile_or_DeleteFileA_and_DeleteFileW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeDeleteFile (const Extbyte * lpFileName)
)
{
  return DeleteFileW ((LPCWSTR) lpFileName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetDiskFreeSpaceEx
#define GetDiskFreeSpaceEx error_use_qxeGetDiskFreeSpaceEx_or_GetDiskFreeSpaceExA_and_GetDiskFreeSpaceExW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetDiskFreeSpaceEx (const Extbyte * lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes)
)
{
  return GetDiskFreeSpaceExW ((LPCWSTR) lpDirectoryName, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetFileAttributesEx
#define GetFileAttributesEx error_use_qxeGetFileAttributesEx_or_GetFileAttributesExA_and_GetFileAttributesExW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetFileAttributesEx (const Extbyte * lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
)
{
  return GetFileAttributesExW ((LPCWSTR) lpFileName, fInfoLevelId, lpFileInformation);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RemoveDirectory
#define RemoveDirectory error_use_qxeRemoveDirectory_or_RemoveDirectoryA_and_RemoveDirectoryW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeRemoveDirectory (const Extbyte * lpPathName)
)
{
  return RemoveDirectoryW ((LPCWSTR) lpPathName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetFileAttributes
#define SetFileAttributes error_use_qxeSetFileAttributes_or_SetFileAttributesA_and_SetFileAttributesW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetFileAttributes (const Extbyte * lpFileName, DWORD dwFileAttributes)
)
{
  return SetFileAttributesW ((LPCWSTR) lpFileName, dwFileAttributes);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetTempPath
#define GetTempPath error_use_qxeGetTempPath_or_GetTempPathA_and_GetTempPathW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetTempPath (DWORD nBufferLength, Extbyte * lpBuffer)
)
{
  return GetTempPathW (nBufferLength, (LPWSTR) lpBuffer);
}


/* Processing file WINREG.H */


/*----------------------------------------------------------------------*/
/*                       Processing file WINREG.H                       */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegConnectRegistry
#define RegConnectRegistry error_use_qxeRegConnectRegistry_or_RegConnectRegistryA_and_RegConnectRegistryW
#endif
/* NOTE: former error in Cygwin prototype, but no more (Cygwin 1.7, 1-30-10) */
DECLARE_INLINE_HEADER (
LONG qxeRegConnectRegistry (const Extbyte * lpMachineName, HKEY hKey, PHKEY phkResult)
)
{
  return RegConnectRegistryW ((LPCWSTR) lpMachineName, hKey, phkResult);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegCreateKey
#define RegCreateKey error_use_qxeRegCreateKey_or_RegCreateKeyA_and_RegCreateKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegCreateKey (HKEY hKey, const Extbyte * lpSubKey, PHKEY phkResult)
)
{
  return RegCreateKeyW (hKey, (LPCWSTR) lpSubKey, phkResult);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegCreateKeyEx
#define RegCreateKeyEx error_use_qxeRegCreateKeyEx_or_RegCreateKeyExA_and_RegCreateKeyExW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegCreateKeyEx (HKEY hKey, const Extbyte * lpSubKey, DWORD Reserved, Extbyte * lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
)
{
  return RegCreateKeyExW (hKey, (LPCWSTR) lpSubKey, Reserved, (LPWSTR) lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegDeleteKey
#define RegDeleteKey error_use_qxeRegDeleteKey_or_RegDeleteKeyA_and_RegDeleteKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegDeleteKey (HKEY hKey, const Extbyte * lpSubKey)
)
{
  return RegDeleteKeyW (hKey, (LPCWSTR) lpSubKey);
}

#undef RegDeleteKeyEx
#define RegDeleteKeyEx error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegDeleteValue
#define RegDeleteValue error_use_qxeRegDeleteValue_or_RegDeleteValueA_and_RegDeleteValueW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegDeleteValue (HKEY hKey, const Extbyte * lpValueName)
)
{
  return RegDeleteValueW (hKey, (LPCWSTR) lpValueName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegEnumKey
#define RegEnumKey error_use_qxeRegEnumKey_or_RegEnumKeyA_and_RegEnumKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegEnumKey (HKEY hKey, DWORD dwIndex, Extbyte * lpName, DWORD cchName)
)
{
  return RegEnumKeyW (hKey, dwIndex, (LPWSTR) lpName, cchName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegEnumKeyEx
#define RegEnumKeyEx error_use_qxeRegEnumKeyEx_or_RegEnumKeyExA_and_RegEnumKeyExW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegEnumKeyEx (HKEY hKey, DWORD dwIndex, Extbyte * lpName, LPDWORD lpcchName, LPDWORD lpReserved, Extbyte * lpClass, LPDWORD lpcchClass, PFILETIME lpftLastWriteTime)
)
{
  return RegEnumKeyExW (hKey, dwIndex, (LPWSTR) lpName, lpcchName, lpReserved, (LPWSTR) lpClass, lpcchClass, lpftLastWriteTime);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegEnumValue
#define RegEnumValue error_use_qxeRegEnumValue_or_RegEnumValueA_and_RegEnumValueW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegEnumValue (HKEY hKey, DWORD dwIndex, Extbyte * lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
)
{
  return RegEnumValueW (hKey, dwIndex, (LPWSTR) lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegLoadKey
#define RegLoadKey error_use_qxeRegLoadKey_or_RegLoadKeyA_and_RegLoadKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegLoadKey (HKEY hKey, const Extbyte * lpSubKey, const Extbyte * lpFile)
)
{
  return RegLoadKeyW (hKey, (LPCWSTR) lpSubKey, (LPCWSTR) lpFile);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegOpenKey
#define RegOpenKey error_use_qxeRegOpenKey_or_RegOpenKeyA_and_RegOpenKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegOpenKey (HKEY hKey, const Extbyte * lpSubKey, PHKEY phkResult)
)
{
  return RegOpenKeyW (hKey, (LPCWSTR) lpSubKey, phkResult);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegOpenKeyEx
#define RegOpenKeyEx error_use_qxeRegOpenKeyEx_or_RegOpenKeyExA_and_RegOpenKeyExW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegOpenKeyEx (HKEY hKey, const Extbyte * lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
)
{
  return RegOpenKeyExW (hKey, (LPCWSTR) lpSubKey, ulOptions, samDesired, phkResult);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegQueryInfoKey
#define RegQueryInfoKey error_use_qxeRegQueryInfoKey_or_RegQueryInfoKeyA_and_RegQueryInfoKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegQueryInfoKey (HKEY hKey, Extbyte * lpClass, LPDWORD lpcchClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime)
)
{
  return RegQueryInfoKeyW (hKey, (LPWSTR) lpClass, lpcchClass, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegQueryValue
#define RegQueryValue error_use_qxeRegQueryValue_or_RegQueryValueA_and_RegQueryValueW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegQueryValue (HKEY hKey, const Extbyte * lpSubKey, Extbyte * lpData, PLONG lpcbData)
)
{
  return RegQueryValueW (hKey, (LPCWSTR) lpSubKey, (LPWSTR) lpData, lpcbData);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegQueryMultipleValues
#define RegQueryMultipleValues error_use_qxeRegQueryMultipleValues_or_RegQueryMultipleValuesA_and_RegQueryMultipleValuesW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegQueryMultipleValues (HKEY hKey, PVALENTW val_list, DWORD num_vals, Extbyte * lpValueBuf, LPDWORD ldwTotsize)
)
{
  return RegQueryMultipleValuesW (hKey, val_list, num_vals, (LPWSTR) lpValueBuf, ldwTotsize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegQueryValueEx
#define RegQueryValueEx error_use_qxeRegQueryValueEx_or_RegQueryValueExA_and_RegQueryValueExW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegQueryValueEx (HKEY hKey, const Extbyte * lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
)
{
  return RegQueryValueExW (hKey, (LPCWSTR) lpValueName, lpReserved, lpType, lpData, lpcbData);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegReplaceKey
#define RegReplaceKey error_use_qxeRegReplaceKey_or_RegReplaceKeyA_and_RegReplaceKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegReplaceKey (HKEY hKey, const Extbyte * lpSubKey, const Extbyte * lpNewFile, const Extbyte * lpOldFile)
)
{
  return RegReplaceKeyW (hKey, (LPCWSTR) lpSubKey, (LPCWSTR) lpNewFile, (LPCWSTR) lpOldFile);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegRestoreKey
#define RegRestoreKey error_use_qxeRegRestoreKey_or_RegRestoreKeyA_and_RegRestoreKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegRestoreKey (HKEY hKey, const Extbyte * lpFile, DWORD dwFlags)
)
{
  return RegRestoreKeyW (hKey, (LPCWSTR) lpFile, dwFlags);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegSaveKey
#define RegSaveKey error_use_qxeRegSaveKey_or_RegSaveKeyA_and_RegSaveKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegSaveKey (HKEY hKey, const Extbyte * lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
)
{
  return RegSaveKeyW (hKey, (LPCWSTR) lpFile, lpSecurityAttributes);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegSetValue
#define RegSetValue error_use_qxeRegSetValue_or_RegSetValueA_and_RegSetValueW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegSetValue (HKEY hKey, const Extbyte * lpSubKey, DWORD dwType, const Extbyte * lpData, DWORD cbData)
)
{
  return RegSetValueW (hKey, (LPCWSTR) lpSubKey, dwType, (LPCWSTR) lpData, cbData);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegSetValueEx
#define RegSetValueEx error_use_qxeRegSetValueEx_or_RegSetValueExA_and_RegSetValueExW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegSetValueEx (HKEY hKey, const Extbyte * lpValueName, DWORD Reserved, DWORD dwType, const BYTE * lpData, DWORD cbData)
)
{
  return RegSetValueExW (hKey, (LPCWSTR) lpValueName, Reserved, dwType, lpData, cbData);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef RegUnLoadKey
#define RegUnLoadKey error_use_qxeRegUnLoadKey_or_RegUnLoadKeyA_and_RegUnLoadKeyW
#endif
DECLARE_INLINE_HEADER (
LONG qxeRegUnLoadKey (HKEY hKey, const Extbyte * lpSubKey)
)
{
  return RegUnLoadKeyW (hKey, (LPCWSTR) lpSubKey);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef InitiateSystemShutdown
#define InitiateSystemShutdown error_use_qxeInitiateSystemShutdown_or_InitiateSystemShutdownA_and_InitiateSystemShutdownW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeInitiateSystemShutdown (Extbyte * lpMachineName, Extbyte * lpMessage, DWORD dwTimeout, BOOL bForceAppsClosed, BOOL bRebootAfterShutdown)
)
{
  return InitiateSystemShutdownW ((LPWSTR) lpMachineName, (LPWSTR) lpMessage, dwTimeout, bForceAppsClosed, bRebootAfterShutdown);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef AbortSystemShutdown
#define AbortSystemShutdown error_use_qxeAbortSystemShutdown_or_AbortSystemShutdownA_and_AbortSystemShutdownW
#endif
/* NOTE: error arg 1, Cygwin prototype, extra const.
   NOTE: Prototype manually overridden.
         Header file claims:
           WINADVAPI WINBOOL WINAPI AbortSystemShutdown(LPWSTR lpMachineName)
         Overridden with:
           BOOL AbortSystemShutdown(LPWSTR)
         Differences in return-type qualifiers, e.g. WINAPI, are not important.
 */
DECLARE_INLINE_HEADER (
BOOL qxeAbortSystemShutdown (Extbyte * arg1)
)
{
  return AbortSystemShutdownW ((LPWSTR) arg1);
}


/* Processing file synchapi.h */


/*----------------------------------------------------------------------*/
/*                      Processing file synchapi.h                      */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenMutex
#define OpenMutex error_use_qxeOpenMutex_or_OpenMutexA_and_OpenMutexW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeOpenMutex (DWORD dwDesiredAccess, BOOL bInheritHandle, const Extbyte * lpName)
)
{
  return OpenMutexW (dwDesiredAccess, bInheritHandle, (LPCWSTR) lpName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenEvent
#define OpenEvent error_use_qxeOpenEvent_or_OpenEventA_and_OpenEventW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeOpenEvent (DWORD dwDesiredAccess, BOOL bInheritHandle, const Extbyte * lpName)
)
{
  return OpenEventW (dwDesiredAccess, bInheritHandle, (LPCWSTR) lpName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenSemaphore
#define OpenSemaphore error_use_qxeOpenSemaphore_or_OpenSemaphoreA_and_OpenSemaphoreW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeOpenSemaphore (DWORD dwDesiredAccess, BOOL bInheritHandle, const Extbyte * lpName)
)
{
  return OpenSemaphoreW (dwDesiredAccess, bInheritHandle, (LPCWSTR) lpName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateMutex
#define CreateMutex error_use_qxeCreateMutex_or_CreateMutexA_and_CreateMutexW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateMutex (LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, const Extbyte * lpName)
)
{
  return CreateMutexW (lpMutexAttributes, bInitialOwner, (LPCWSTR) lpName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateEvent
#define CreateEvent error_use_qxeCreateEvent_or_CreateEventA_and_CreateEventW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeCreateEvent (LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, const Extbyte * lpName)
)
{
  return CreateEventW (lpEventAttributes, bManualReset, bInitialState, (LPCWSTR) lpName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef OpenWaitableTimer
#define OpenWaitableTimer error_use_qxeOpenWaitableTimer_or_OpenWaitableTimerA_and_OpenWaitableTimerW
#endif
DECLARE_INLINE_HEADER (
HANDLE qxeOpenWaitableTimer (DWORD dwDesiredAccess, BOOL bInheritHandle, const Extbyte * lpTimerName)
)
{
  return OpenWaitableTimerW (dwDesiredAccess, bInheritHandle, (LPCWSTR) lpTimerName);
}


/* Processing file WINNLS.H */


/*----------------------------------------------------------------------*/
/*                       Processing file WINNLS.H                       */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetLocaleInfo
#define GetLocaleInfo error_use_qxeGetLocaleInfo_or_GetLocaleInfoA_and_GetLocaleInfoW
#endif
DECLARE_INLINE_HEADER (
int qxeGetLocaleInfo (LCID Locale, LCTYPE LCType, Extbyte * lpLCData, int cchData)
)
{
  return GetLocaleInfoW (Locale, LCType, (LPWSTR) lpLCData, cchData);
}

#undef LCMapString
#define LCMapString error_not_used__not_examined_yet

#undef GetNumberFormat
#define GetNumberFormat error_not_used__not_examined_yet

#undef GetCurrencyFormat
#define GetCurrencyFormat error_not_used__not_examined_yet

#undef EnumCalendarInfo
#define EnumCalendarInfo error_not_used__not_examined_yet

#undef EnumCalendarInfoEx
#define EnumCalendarInfoEx error_not_used__not_examined_yet

#undef EnumTimeFormats
#define EnumTimeFormats error_not_used__not_examined_yet

#undef EnumDateFormats
#define EnumDateFormats error_not_used__not_examined_yet

#undef EnumDateFormatsEx
#define EnumDateFormatsEx error_not_used__not_examined_yet

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetLocaleInfo
#define SetLocaleInfo error_use_qxeSetLocaleInfo_or_SetLocaleInfoA_and_SetLocaleInfoW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetLocaleInfo (LCID Locale, LCTYPE LCType, const Extbyte * lpLCData)
)
{
  return SetLocaleInfoW (Locale, LCType, (LPCWSTR) lpLCData);
}

#undef GetCalendarInfo
#define GetCalendarInfo error_Function_needs_review_to_determine_how_to_handle_it

#undef SetCalendarInfo
#define SetCalendarInfo error_Function_needs_review_to_determine_how_to_handle_it

#undef GetGeoInfo
#define GetGeoInfo error_Function_needs_review_to_determine_how_to_handle_it

#undef GetCPInfoEx
#define GetCPInfoEx error_not_used__not_examined_yet

#undef EnumSystemLocales
#define EnumSystemLocales error_not_used__not_examined_yet

#undef EnumSystemLanguageGroups
#define EnumSystemLanguageGroups error_Function_needs_review_to_determine_how_to_handle_it

#undef EnumLanguageGroupLocales
#define EnumLanguageGroupLocales error_Function_needs_review_to_determine_how_to_handle_it

#undef EnumUILanguages
#define EnumUILanguages error_Function_needs_review_to_determine_how_to_handle_it

#undef EnumSystemCodePages
#define EnumSystemCodePages error_not_used__not_examined_yet


/* Processing file SHLOBJ.H */


/*----------------------------------------------------------------------*/
/*                       Processing file SHLOBJ.H                       */
/*----------------------------------------------------------------------*/

#undef SHGetFolderPath
#define SHGetFolderPath error_Function_needs_review_to_determine_how_to_handle_it

#undef SHGetIconOverlayIndex
#define SHGetIconOverlayIndex error_Function_needs_review_to_determine_how_to_handle_it

/* Skipping SHGetPathFromIDList because Cygwin qualifiers confuses parser */

#undef SHCreateDirectoryEx
#define SHCreateDirectoryEx error_Function_needs_review_to_determine_how_to_handle_it

/* Skipping SHGetSpecialFolderPath because error in Cygwin prototype, missing from Cygwin libraries */

#undef SHGetFolderPathAndSubDir
#define SHGetFolderPathAndSubDir error_Function_needs_review_to_determine_how_to_handle_it

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SHBrowseForFolder
#define SHBrowseForFolder error_use_qxeSHBrowseForFolder_or_SHBrowseForFolderA_and_SHBrowseForFolderW
#endif
DECLARE_INLINE_HEADER (
LPITEMIDLIST qxeSHBrowseForFolder (LPBROWSEINFOW lpbi)
)
{
  return SHBrowseForFolderW (lpbi);
}


/* Processing file COMMDLG.H */


/*----------------------------------------------------------------------*/
/*                      Processing file COMMDLG.H                       */
/*----------------------------------------------------------------------*/

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetOpenFileName
#define GetOpenFileName error_use_qxeGetOpenFileName_or_GetOpenFileNameA_and_GetOpenFileNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetOpenFileName (LPOPENFILENAMEW arg1)
)
{
  return GetOpenFileNameW (arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetSaveFileName
#define GetSaveFileName error_use_qxeGetSaveFileName_or_GetSaveFileNameA_and_GetSaveFileNameW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeGetSaveFileName (LPOPENFILENAMEW arg1)
)
{
  return GetSaveFileNameW (arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetFileTitle
#define GetFileTitle error_use_qxeGetFileTitle_or_GetFileTitleA_and_GetFileTitleW
#endif
DECLARE_INLINE_HEADER (
short qxeGetFileTitle (const Extbyte * arg1, Extbyte * arg2, WORD arg3)
)
{
  return GetFileTitleW ((LPCWSTR) arg1, (LPWSTR) arg2, arg3);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ChooseColor
#define ChooseColor error_use_qxeChooseColor_or_ChooseColorA_and_ChooseColorW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeChooseColor (LPCHOOSECOLORW arg1)
)
{
  return ChooseColorW (arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FindText
#define FindText error_use_qxeFindText_or_FindTextA_and_FindTextW
#endif
DECLARE_INLINE_HEADER (
HWND qxeFindText (LPFINDREPLACEW arg1)
)
{
  return FindTextW (arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ReplaceText
#define ReplaceText error_use_qxeReplaceText_or_ReplaceTextA_and_ReplaceTextW
#endif
DECLARE_INLINE_HEADER (
HWND qxeReplaceText (LPFINDREPLACEW arg1)
)
{
  return ReplaceTextW (arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef ChooseFont
#define ChooseFont error_split_sized_LPLOGFONT_in_LPCHOOSEFONT
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PrintDlg
#define PrintDlg error_use_qxePrintDlg_or_PrintDlgA_and_PrintDlgW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePrintDlg (LPPRINTDLGW arg1)
)
{
  return PrintDlgW (arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#undef PrintDlgEx
#define PrintDlgEx error_Function_needs_review_to_determine_how_to_handle_it
#endif /* defined (HAVE_MS_WINDOWS) */

#if defined (HAVE_MS_WINDOWS)
#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PageSetupDlg
#define PageSetupDlg error_use_qxePageSetupDlg_or_PageSetupDlgA_and_PageSetupDlgW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePageSetupDlg (LPPAGESETUPDLGW arg1)
)
{
  return PageSetupDlgW (arg1);
}
#endif /* defined (HAVE_MS_WINDOWS) */


/* Processing file WINCON.H */


/*----------------------------------------------------------------------*/
/*                       Processing file WINCON.H                       */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef PeekConsoleInput
#define PeekConsoleInput error_use_qxePeekConsoleInput_or_PeekConsoleInputA_and_PeekConsoleInputW
#endif
DECLARE_INLINE_HEADER (
BOOL qxePeekConsoleInput (HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
)
{
  return PeekConsoleInputW (hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ReadConsoleInput
#define ReadConsoleInput error_use_qxeReadConsoleInput_or_ReadConsoleInputA_and_ReadConsoleInputW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeReadConsoleInput (HANDLE hConsoleInput, PINPUT_RECORD lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsRead)
)
{
  return ReadConsoleInputW (hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WriteConsoleInput
#define WriteConsoleInput error_use_qxeWriteConsoleInput_or_WriteConsoleInputA_and_WriteConsoleInputW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWriteConsoleInput (HANDLE hConsoleInput, const INPUT_RECORD * lpBuffer, DWORD nLength, LPDWORD lpNumberOfEventsWritten)
)
{
  return WriteConsoleInputW (hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ReadConsoleOutput
#define ReadConsoleOutput error_use_qxeReadConsoleOutput_or_ReadConsoleOutputA_and_ReadConsoleOutputW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeReadConsoleOutput (HANDLE hConsoleOutput, PCHAR_INFO lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpReadRegion)
)
{
  return ReadConsoleOutputW (hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpReadRegion);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WriteConsoleOutput
#define WriteConsoleOutput error_use_qxeWriteConsoleOutput_or_WriteConsoleOutputA_and_WriteConsoleOutputW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWriteConsoleOutput (HANDLE hConsoleOutput, const CHAR_INFO * lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpWriteRegion)
)
{
  return WriteConsoleOutputW (hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ReadConsoleOutputCharacter
#define ReadConsoleOutputCharacter error_use_qxeReadConsoleOutputCharacter_or_ReadConsoleOutputCharacterA_and_ReadConsoleOutputCharacterW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeReadConsoleOutputCharacter (HANDLE hConsoleOutput, Extbyte * lpCharacter, DWORD nLength, COORD dwReadCoord, LPDWORD lpNumberOfCharsRead)
)
{
  return ReadConsoleOutputCharacterW (hConsoleOutput, (LPWSTR) lpCharacter, nLength, dwReadCoord, lpNumberOfCharsRead);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WriteConsoleOutputCharacter
#define WriteConsoleOutputCharacter error_use_qxeWriteConsoleOutputCharacter_or_WriteConsoleOutputCharacterA_and_WriteConsoleOutputCharacterW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWriteConsoleOutputCharacter (HANDLE hConsoleOutput, const Extbyte * lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
)
{
  return WriteConsoleOutputCharacterW (hConsoleOutput, (LPCWSTR) lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten);
}

#undef FillConsoleOutputCharacter
#define FillConsoleOutputCharacter error_split_CHAR

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ScrollConsoleScreenBuffer
#define ScrollConsoleScreenBuffer error_use_qxeScrollConsoleScreenBuffer_or_ScrollConsoleScreenBufferA_and_ScrollConsoleScreenBufferW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeScrollConsoleScreenBuffer (HANDLE hConsoleOutput, const SMALL_RECT * lpScrollRectangle, const SMALL_RECT * lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO * lpFill)
)
{
  return ScrollConsoleScreenBufferW (hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetConsoleTitle
#define GetConsoleTitle error_use_qxeGetConsoleTitle_or_GetConsoleTitleA_and_GetConsoleTitleW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetConsoleTitle (Extbyte * lpConsoleTitle, DWORD nSize)
)
{
  return GetConsoleTitleW ((LPWSTR) lpConsoleTitle, nSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetConsoleTitle
#define SetConsoleTitle error_use_qxeSetConsoleTitle_or_SetConsoleTitleA_and_SetConsoleTitleW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetConsoleTitle (const Extbyte * lpConsoleTitle)
)
{
  return SetConsoleTitleW ((LPCWSTR) lpConsoleTitle);
}

/* Skipping ReadConsole because last argument PCONSOLE_READCONSOLE_CONTROL pInputControl in recent VisualStudio, LPVOID in W32API */

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef WriteConsole
#define WriteConsole error_use_qxeWriteConsole_or_WriteConsoleA_and_WriteConsoleW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeWriteConsole (HANDLE hConsoleOutput, const VOID * lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
)
{
  return WriteConsoleW (hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
}


/* Processing file processthreadsapi.h */


/*----------------------------------------------------------------------*/
/*                 Processing file processthreadsapi.h                  */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetStartupInfo
#define GetStartupInfo error_use_qxeGetStartupInfo_or_GetStartupInfoA_and_GetStartupInfoW
#endif
DECLARE_INLINE_HEADER (
VOID qxeGetStartupInfo (LPSTARTUPINFOW lpStartupInfo)
)
{
  GetStartupInfoW (lpStartupInfo);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateProcessAsUser
#define CreateProcessAsUser error_use_qxeCreateProcessAsUser_or_CreateProcessAsUserA_and_CreateProcessAsUserW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCreateProcessAsUser (HANDLE hToken, const Extbyte * lpApplicationName, Extbyte * lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, const Extbyte * lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
)
{
  return CreateProcessAsUserW (hToken, (LPCWSTR) lpApplicationName, (LPWSTR) lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, (LPCWSTR) lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef CreateProcess
#define CreateProcess error_use_qxeCreateProcess_or_CreateProcessA_and_CreateProcessW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeCreateProcess (const Extbyte * lpApplicationName, Extbyte * lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, const Extbyte * lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
)
{
  return CreateProcessW ((LPCWSTR) lpApplicationName, (LPWSTR) lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, (LPCWSTR) lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}


/* Processing file processenv.h */


/*----------------------------------------------------------------------*/
/*                     Processing file processenv.h                     */
/*----------------------------------------------------------------------*/

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCommandLine
#define GetCommandLine error_use_qxeGetCommandLine_or_GetCommandLineA_and_GetCommandLineW
#endif
DECLARE_INLINE_HEADER (
Extbyte * qxeGetCommandLine (void)
)
{
  return (Extbyte *) GetCommandLineW ();
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetCurrentDirectory
#define SetCurrentDirectory error_use_qxeSetCurrentDirectory_or_SetCurrentDirectoryA_and_SetCurrentDirectoryW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetCurrentDirectory (const Extbyte * lpPathName)
)
{
  return SetCurrentDirectoryW ((LPCWSTR) lpPathName);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetCurrentDirectory
#define GetCurrentDirectory error_use_qxeGetCurrentDirectory_or_GetCurrentDirectoryA_and_GetCurrentDirectoryW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetCurrentDirectory (DWORD nBufferLength, Extbyte * lpBuffer)
)
{
  return GetCurrentDirectoryW (nBufferLength, (LPWSTR) lpBuffer);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SearchPath
#define SearchPath error_use_qxeSearchPath_or_SearchPathA_and_SearchPathW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeSearchPath (const Extbyte * lpPath, const Extbyte * lpFileName, const Extbyte * lpExtension, DWORD nBufferLength, Extbyte * lpBuffer, Extbyte * * lpFilePart)
)
{
  return SearchPathW ((LPCWSTR) lpPath, (LPCWSTR) lpFileName, (LPCWSTR) lpExtension, nBufferLength, (LPWSTR) lpBuffer, (LPWSTR *) lpFilePart);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef ExpandEnvironmentStrings
#define ExpandEnvironmentStrings error_use_qxeExpandEnvironmentStrings_or_ExpandEnvironmentStringsA_and_ExpandEnvironmentStringsW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeExpandEnvironmentStrings (const Extbyte * lpSrc, Extbyte * lpDst, DWORD nSize)
)
{
  return ExpandEnvironmentStringsW ((LPCWSTR) lpSrc, (LPWSTR) lpDst, nSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef FreeEnvironmentStrings
#define FreeEnvironmentStrings error_use_qxeFreeEnvironmentStrings_or_FreeEnvironmentStringsA_and_FreeEnvironmentStringsW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeFreeEnvironmentStrings (LPWCH penv)
)
{
  return FreeEnvironmentStringsW (penv);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef GetEnvironmentVariable
#define GetEnvironmentVariable error_use_qxeGetEnvironmentVariable_or_GetEnvironmentVariableA_and_GetEnvironmentVariableW
#endif
DECLARE_INLINE_HEADER (
DWORD qxeGetEnvironmentVariable (const Extbyte * lpName, Extbyte * lpBuffer, DWORD nSize)
)
{
  return GetEnvironmentVariableW ((LPCWSTR) lpName, (LPWSTR) lpBuffer, nSize);
}

#ifdef ERROR_WHEN_NONINTERCEPTED_FUNS_USED
#undef SetEnvironmentVariable
#define SetEnvironmentVariable error_use_qxeSetEnvironmentVariable_or_SetEnvironmentVariableA_and_SetEnvironmentVariableW
#endif
DECLARE_INLINE_HEADER (
BOOL qxeSetEnvironmentVariable (const Extbyte * lpName, const Extbyte * lpValue)
)
{
  return SetEnvironmentVariableW ((LPCWSTR) lpName, (LPCWSTR) lpValue);
}

