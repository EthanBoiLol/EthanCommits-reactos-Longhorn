@ stdcall ChangeWindowMessageFilter(long long)
@ stdcall ChangeWindowMessageFilterEx(long long long ptr)
@ stdcall UnregisterPowerSettingNotification(ptr) 
@ stdcall SetWindowCompositionAttribute(ptr ptr)
@ stdcall CalculatePopupWindowPosition(ptr long long ptr ptr)
@ stdcall DwmGetDxSharedSurface(ptr ptr ptr ptr ptr ptr)
@ stdcall GetWindowCompositionAttribute(ptr ptr)
@ stdcall GhostWindowFromHungWindow(ptr)
@ stdcall HungWindowFromGhostWindow(ptr) 


@   stdcall -version=0x600+ InternalGetWindowIcon(ptr long)
@   stdcall -version=0x600+ IsProcessDPIAware()
@   stdcall -version=0x600+ IsThreadDesktopComposited()
@   stdcall -version=0x601+ IsTopLevelWindow(ptr)
@   stdcall -version=0x601+ IsTouchWindow(ptr long)
@   stdcall -version=0x600+ IsWindowRedirectedForPrint(ptr)
@ stdcall -version=0x600+ LogicalToPhysicalPoint(ptr ptr)
@   stdcall -stub -version=0x600+ OpenThreadDesktop(long long long long)
@   stdcall -stub -version=0x600+ PaintMonitor()
@ stub -version=0x601+ QueryDisplayConfig
@ stub -version=0x601+ SetDisplayConfig
@   stdcall -stub -version=0x600+ RegisterErrorReportingDialog(long long)
@   stdcall -stub -version=0x600+ RegisterFrostWindow(long long)
@   stdcall -stub -version=0x600+ RegisterGhostWindow(long long)
@   stdcall -version=0x600+ DisplayConfigGetDeviceInfo(ptr)
@   stdcall -version=0x600+ RegisterTouchWindow(ptr long)
@   stdcall -version=0x600+ CloseTouchInputHandle(ptr)
@   stdcall -version=0x600+ RegisterPowerSettingNotification(ptr ptr long)
@   stdcall -version=0x600+ SetProcessDPIAware()
@ stdcall AddClipboardFormatListener(ptr)
@ stdcall RemoveClipboardFormatListener(ptr)

@ stdcall GetProcessDpiAwarenessInternal(ptr ptr)
@ stdcall SetProcessDpiAwarenessInternal(long)
@ stdcall GetDpiForMonitorInternal(ptr long ptr ptr)