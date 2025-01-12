@ fastcall WdEnterMonitoredSection(ptr)
@ fastcall WdExitMonitoredSection(ptr)
@ fastcall WdResetDeferredWatch(ptr)
@ fastcall WdResumeDeferredWatch(ptr ptr)
@ fastcall WdSuspendDeferredWatch(ptr)
@ stdcall WdAllocateDeferredWatchdog(ptr ptr long)
@ stdcall WdAllocateWatchdog(ptr ptr long)
@ stdcall WdAttachContext(ptr ptr)
@ stdcall WdCompleteEvent(ptr ptr)
@ stdcall WdDereferenceObject(ptr)
@ stdcall WdDetachContext(ptr)
@ stdcall WdFreeDeferredWatchdog(ptr)
@ stdcall WdFreeWatchdog(ptr)
@ stdcall WdGetDeviceObject(ptr)
@ stdcall WdGetLastEvent(ptr)
@ stdcall WdGetLowestDeviceObject(ptr)
@ stdcall WdMadeAnyProgress(ptr)
@ stdcall WdReferenceObject(ptr)
@ stdcall WdResetWatch(ptr)
@ stdcall WdResumeWatch(ptr ptr)
@ stdcall WdStartDeferredWatch(ptr ptr long)
@ stdcall WdStartWatch(ptr double ptr)
@ stdcall WdStopDeferredWatch(ptr)
@ stdcall WdStopWatch(ptr long)
@ stdcall WdSuspendWatch(ptr)

;ReactOS Specific / Vista
@ stdcall SMgrNotifySessionChange(long)
@ stdcall SMgrRegisterGdiCallout(ptr)
