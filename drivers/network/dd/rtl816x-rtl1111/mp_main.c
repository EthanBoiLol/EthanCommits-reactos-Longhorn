/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    mp_main.c

Abstract:
    This module contains NDIS miniport handlers

Revision History:

Notes:

--*/

#include "precomp.h"

#if DBG
#define _FILENUMBER     'NIAM'
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

NDIS_HANDLE     NdisMiniportDriverHandle = NULL;
NDIS_HANDLE     MiniportDriverContext = NULL;


NDIS_STATUS DriverEntry(
    IN  PDRIVER_OBJECT   DriverObject,
    IN  PUNICODE_STRING  RegistryPath
    )
/*++
Routine Description:

Arguments:

    DriverObject    -   pointer to the driver object
    RegistryPath    -   pointer to the driver registry path
     
Return Value:
    
    NDIS_STATUS - the value returned by NdisMRegisterMiniport 
    
--*/
{
    NDIS_STATUS                         Status;
    NDIS_MINIPORT_DRIVER_CHARACTERISTICS MPChar;
    
    DBGPRINT(MP_TRACE, ("====> DriverEntry\n"));
#if DBG
	//KdBreakPoint();
#endif
	KdPrint(("Built %s %s\n", __DATE__, __TIME__));

    //
    // Fill in the Miniport characteristics structure with the version numbers 
    // and the entry points for driver-supplied MiniportXxx 
    //
    NdisZeroMemory(&MPChar, sizeof(MPChar));

    MPChar.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS,
    MPChar.Header.Size = sizeof(NDIS_MINIPORT_DRIVER_CHARACTERISTICS);
    MPChar.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_1;

    MPChar.MajorNdisVersion             = MP_NDIS_MAJOR_VERSION;
    MPChar.MinorNdisVersion             = MP_NDIS_MINOR_VERSION;
    MPChar.MajorDriverVersion           = NIC_MAJOR_DRIVER_VERSION;
    MPChar.MinorDriverVersion           = NIC_MINOR_DRIVER_VERISON;

    MPChar.SetOptionsHandler = MpSetOptions;
    
    MPChar.InitializeHandlerEx          = MPInitialize;
    MPChar.HaltHandlerEx                = MPHalt;

    MPChar.UnloadHandler                = MPUnload,
    
    MPChar.PauseHandler                 = MPPause;      
    MPChar.RestartHandler               = MPRestart;    
    MPChar.OidRequestHandler            = MPOidRequest;    
    MPChar.SendNetBufferListsHandler    = MPSendNetBufferLists;  // dummy
    MPChar.ReturnNetBufferListsHandler  = MPReturnNetBufferLists;
    MPChar.CancelSendHandler            = MPCancelSendNetBufferLists;
    MPChar.DevicePnPEventNotifyHandler  = MPPnPEventNotify;
    MPChar.ShutdownHandlerEx            = MPShutdown;
    MPChar.CheckForHangHandlerEx        = MPCheckForHang;  // true
    MPChar.ResetHandlerEx               = MPReset;
    MPChar.CancelOidRequestHandler      = MPCancelOidRequest;
    
    
    DBGPRINT(MP_LOUD, ("Calling NdisMRegisterMiniportDriver...\n"));

    Status = NdisMRegisterMiniportDriver(DriverObject,
                                         RegistryPath,
                                         (PNDIS_HANDLE)MiniportDriverContext,
                                         &MPChar,
                                         &NdisMiniportDriverHandle);
    

    DBGPRINT_S(Status, ("<==== DriverEntry, Status=%x\n", Status));

    return Status;
}


NDIS_STATUS
MpSetOptions(
    IN NDIS_HANDLE  NdisMiniportDriverHandle,
    IN NDIS_HANDLE  MiniportDriverContext
    )
{
    UNREFERENCED_PARAMETER(NdisMiniportDriverHandle);
    UNREFERENCED_PARAMETER(MiniportDriverContext);
    
    DBGPRINT(MP_TRACE, ("====> MpSetOptions\n"));

    DBGPRINT(MP_TRACE, ("<==== MpSetOptions\n"));

    return (NDIS_STATUS_SUCCESS);
}


NDIS_STATUS 
MPInitialize(
    IN  NDIS_HANDLE                        MiniportAdapterHandle,
    IN  NDIS_HANDLE                        MiniportDriverContext,
    IN  PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
    )
/*++
Routine Description:

    MiniportInitialize handler

Arguments:

    MiniportAdapterHandle   The handle NDIS uses to refer to us
    MiniportDriverContext   Handle passed to NDIS when we registered the driver
    MiniportInitParameters  Initialization parameters
    
Return Value:

    NDIS_STATUS_SUCCESS unless something goes wrong

--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS  Interrupt;
    NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES   RegistrationAttributes;
    NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES        GeneralAttributes;
    NDIS_TIMER_CHARACTERISTICS                      Timer;               
    NDIS_PNP_CAPABILITIES          PowerManagementCapabilities;    
    PMP_ADAPTER     Adapter = NULL;
    PVOID           NetworkAddress;
    UINT            index;
    UINT            uiPnpCommandValue;
    ULONG           ulInfoLen;
    ULONG           InterruptVersion;
    LARGE_INTEGER   liDueTime;
    BOOLEAN         isTimerAlreadyInQueue = FALSE;
    
#if DBG
    LARGE_INTEGER   TS, TD, TE;
#endif

    DBGPRINT(MP_TRACE, ("====> MPInitialize\n"));

    UNREFERENCED_PARAMETER(MiniportDriverContext);
    
#if DBG
    NdisGetCurrentSystemTime(&TS);
#endif    

    do
    {

        //
        // Allocate MP_ADAPTER structure
        //
        Status = MpAllocAdapterBlock(&Adapter, MiniportAdapterHandle);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            break;
        }

        Adapter->AdapterHandle = MiniportAdapterHandle;

        NdisZeroMemory(&RegistrationAttributes, sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES));
        NdisZeroMemory(&GeneralAttributes, sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES));

        //
        // setting registration attributes
        //
        RegistrationAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        RegistrationAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
        RegistrationAttributes.Header.Size = sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES);

        RegistrationAttributes.MiniportAdapterContext = (NDIS_HANDLE)Adapter;
        RegistrationAttributes.AttributeFlags = NDIS_MINIPORT_ATTRIBUTES_HARDWARE_DEVICE | 
                                                NDIS_MINIPORT_ATTRIBUTES_BUS_MASTER;
        
        RegistrationAttributes.CheckForHangTimeInSeconds = 2;
        RegistrationAttributes.InterfaceType = NIC_INTERFACE_TYPE;

        Status = NdisMSetMiniportAttributes(MiniportAdapterHandle,
                                            (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&RegistrationAttributes);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        
        //
        // Read the registry parameters
        //
        Status = NICReadRegParameters(Adapter);
        
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            break;
        }
    
        //
        // Find the physical adapter
        //
        Status = MpFindAdapter(Adapter, MiniportInitParameters->AllocatedResources);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
		// During HW resources enumeration time ( CmResourceTypePort )
		// OS handed over a physical address for port start. 
		// But now it wants only a UINT
		// as the 3rd parameter to NdisMRegisterIoPortRange(..) below
		// ..@hame...
		ASSERT ( NdisGetPhysicalAddressHigh ( Adapter->IoBaseAddress ) == 0 ) ;
        
		//
        // Map bus-relative IO range to system IO space
        //
		// http://doxygen.reactos.org/dd/d5f/drivers_2network_2ndis_2ndis_2io_8c_a88f544a9e9edf72d7becfbd31025159a.html
        Status = NdisMRegisterIoPortRange(
                     (PVOID *)&Adapter->PortOffset,
                     Adapter->AdapterHandle,
                     NdisGetPhysicalAddressLow ( Adapter->IoBaseAddress ) ,
                     Adapter->IoRange);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MP_ERROR, ("NdisMRegisterioPortRange failed\n"));
    
            NdisWriteErrorLogEntry(
                Adapter->AdapterHandle,
                NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS,
                0);
        
            break;
        }
        //
        // Map bus-relative registers to virtual system-space
        // 
		// http://doxygen.reactos.org/dd/d5f/drivers_2network_2ndis_2ndis_2io_8c_a1b22d49e35b847e779e902aebc321a7a.html
        Status = NdisMMapIoSpace(
                     (PVOID *) &(Adapter->CSRAddress),
                     Adapter->AdapterHandle,
                     Adapter->MemPhysAddress,
                     Adapter->MemPhysLength );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MP_ERROR, ("NdisMMapIoSpace failed\n"));
    
            NdisWriteErrorLogEntry(
                Adapter->AdapterHandle,
                NDIS_ERROR_CODE_RESOURCE_CONFLICT,
                1,
                ERRLOG_MAP_IO_SPACE);
        
            break;
        }

        DBGPRINT(MP_INFO, ("CSRAddress="PTR_FORMAT"\n", Adapter->CSRAddress));
        
        //
        // Read additional info from NIC such as MAC address
        //
        Status = NICReadAdapterInfo(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            break;
        }

        //
        // set up generic attributes
        //

        GeneralAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        GeneralAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1;
        GeneralAttributes.Header.Size = sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES);

        GeneralAttributes.MediaType = NIC_MEDIA_TYPE;

        GeneralAttributes.MtuSize = NIC_MAX_RECV_FRAME_SIZE - NIC_HEADER_SIZE;
        GeneralAttributes.MaxXmitLinkSpeed = NIC_MEDIA_MAX_SPEED;
        GeneralAttributes.MaxRcvLinkSpeed = NIC_MEDIA_MAX_SPEED;
        GeneralAttributes.XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
        GeneralAttributes.RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
        GeneralAttributes.MediaConnectState = MediaConnectStateUnknown;
        GeneralAttributes.MediaDuplexState = MediaDuplexStateUnknown;
        GeneralAttributes.LookaheadSize = NIC_MAX_RECV_FRAME_SIZE - NIC_HEADER_SIZE;

        MPFillPoMgmtCaps (Adapter, 
                          &PowerManagementCapabilities, 
                          &Status,
                          &ulInfoLen);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            GeneralAttributes.PowerManagementCapabilities = &PowerManagementCapabilities;
        }
        else
        {
            GeneralAttributes.PowerManagementCapabilities = NULL;
        }

        //
        // do not fail the call because of failure to get PM caps
        //
        Status = NDIS_STATUS_SUCCESS;

        GeneralAttributes.MacOptions = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
                                       NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                                       NDIS_MAC_OPTION_NO_LOOPBACK;

        GeneralAttributes.SupportedPacketFilters = NDIS_PACKET_TYPE_DIRECTED |
                                                   NDIS_PACKET_TYPE_MULTICAST |
                                                   NDIS_PACKET_TYPE_ALL_MULTICAST |
                                                   NDIS_PACKET_TYPE_BROADCAST;
        
        GeneralAttributes.MaxMulticastListSize = NIC_MAX_MCAST_LIST;
        GeneralAttributes.MacAddressLength = ETH_LENGTH_OF_ADDRESS;
        NdisMoveMemory(GeneralAttributes.PermanentMacAddress,
                       Adapter->PermanentAddress,
                       ETH_LENGTH_OF_ADDRESS);

        NdisMoveMemory(GeneralAttributes.CurrentMacAddress,
                       Adapter->CurrentAddress,
                       ETH_LENGTH_OF_ADDRESS);

        
        GeneralAttributes.PhysicalMediumType = NdisPhysicalMediumUnspecified;
        GeneralAttributes.RecvScaleCapabilities = NULL;
        GeneralAttributes.AccessType = NET_IF_ACCESS_BROADCAST; // NET_IF_ACCESS_BROADCAST for a typical ethernet adapter
        GeneralAttributes.DirectionType = NET_IF_DIRECTION_SENDRECEIVE; // NET_IF_DIRECTION_SENDRECEIVE for a typical ethernet adapter
        GeneralAttributes.ConnectionType = NET_IF_CONNECTION_DEDICATED;  // NET_IF_CONNECTION_DEDICATED for a typical ethernet adapter
        GeneralAttributes.IfType = IF_TYPE_ETHERNET_CSMACD; // IF_TYPE_ETHERNET_CSMACD for a typical ethernet adapter (regardless of speed)
        GeneralAttributes.IfConnectorPresent = TRUE; // RFC 2665 TRUE if physical adapter

        GeneralAttributes.SupportedStatistics = NDIS_STATISTICS_XMIT_OK_SUPPORTED |
                                                NDIS_STATISTICS_RCV_OK_SUPPORTED |
                                                NDIS_STATISTICS_XMIT_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_RCV_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_RCV_CRC_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_RCV_NO_BUFFER_SUPPORTED |
                                                NDIS_STATISTICS_TRANSMIT_QUEUE_LENGTH_SUPPORTED |
                                                NDIS_STATISTICS_GEN_STATISTICS_SUPPORTED;
                      
        GeneralAttributes.SupportedOidList = NICSupportedOids;
        GeneralAttributes.SupportedOidListLength = sizeof(NICSupportedOids);

        Status = NdisMSetMiniportAttributes(MiniportAdapterHandle,
                                            (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&GeneralAttributes);



        
        //
        // Allocate all other memory blocks including shared memory
        //
        Status = NICAllocAdapterMemory(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            break;
        }
        //
        // Init send data structures
        //
        NICInitSend(Adapter);

        //
        // Init receive data structures
        //
        Status = NICInitRecv(Adapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        //
        // Disable interrupts here which is as soon as possible
        //
        NICDisableInterrupt(Adapter);
                     
        //
        // Register the interrupt
        //
        //
        
        //
        // the embeded NDIS interrupt structure is already zero'ed out
        // as part of the adapter structure
        //
        NdisZeroMemory(&Interrupt, sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS));
        
        Interrupt.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_INTERRUPT;
        Interrupt.Header.Revision = NDIS_MINIPORT_INTERRUPT_REVISION_1;
        Interrupt.Header.Size = sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS);

        Interrupt.InterruptHandler = MPIsr;
        Interrupt.InterruptDpcHandler = MPHandleInterrupt;
        Interrupt.DisableInterruptHandler = NULL;
        Interrupt.EnableInterruptHandler = NULL;


		// http://doxygen.reactos.org/dd/d5f/drivers_2network_2ndis_2ndis_2io_8c_ad468e735f9cad1e74314d8f365abe01c.html
        Status = NdisMRegisterInterruptEx(Adapter->AdapterHandle,
                                          Adapter,
                                          &Interrupt,
                                          &Adapter->NdisInterruptHandle
                                          );
        
                                        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MP_ERROR, ("NdisMRegisterInterrupt failed\n"));
    
            NdisWriteErrorLogEntry(
                Adapter->AdapterHandle,
                NDIS_ERROR_CODE_INTERRUPT_CONNECT,
                0);
        
            break;
        }
        
        //
        // If the driver support MSI
        //
        Adapter->InterruptType = Interrupt.InterruptType;

        if (Adapter->InterruptType == NDIS_CONNECT_MESSAGE_BASED)
        {
            Adapter->MessageInfoTable = Interrupt.MessageInfoTable;
        }
        
        //
        // If the driver supports MSI, here it should what kind of interrupt is granted. If MSI is granted,
        // the driver can check Adapter->MessageInfoTable to get MSI information
        //
        
        
        MP_SET_FLAG(Adapter, fMP_ADAPTER_INTERRUPT_IN_USE);

        //
        // Test our adapter hardware
        //
        Status = NICSelfTest(Adapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        
        //
        // Init the hardware and set up everything
        //
        Status = NICconfigureAndInitializeAdapter(Adapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        
        //
        // initial state is paused
        //
        Adapter->AdapterState = NicPaused;
        
   
        //
        // Enable the interrupt
        //
        NICEnableInterrupt(Adapter);

     } while (FALSE);

    if (Adapter && (Status != NDIS_STATUS_SUCCESS))
    {
        //
        // Undo everything if it failed
        //
        MP_DEC_REF(Adapter);
        MpFreeAdapter(Adapter);
    }

    DBGPRINT_S(Status, ("<==== MPInitialize, Status=%x\n", Status));
    
    return Status;
}


NDIS_STATUS 
MPPause(
    IN  NDIS_HANDLE                         MiniportAdapterContext,    
    IN  PNDIS_MINIPORT_PAUSE_PARAMETERS     MiniportPauseParameters
    )
/*++
 
Routine Description:
    
    MiniportPause handler
    The driver can't indicate any packet, send complete all the pending send requests. 
    and wait for all the packets returned to it.
    
Argument:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING

NOTE: A miniport can't fail the pause request

--*/
{
    PMP_ADAPTER         Adapter = (PMP_ADAPTER) MiniportAdapterContext;
    NDIS_STATUS         Status; 
    LONG                Count;

    UNREFERENCED_PARAMETER(MiniportPauseParameters);
    
    DBGPRINT(MP_TRACE, ("====> MPPause\n"));

    ASSERT(Adapter->AdapterState == NicRunning);

    NdisAcquireSpinLock(&Adapter->RcvLock);
    Adapter->AdapterState = NicPausing;
    NdisReleaseSpinLock(&Adapter->RcvLock);    

    do 
    {
        //
        // Complete all the pending sends
        // 
        NdisAcquireSpinLock(&Adapter->SendLock);
    
        MpFreeQueuedSendNetBufferLists(Adapter);
        NdisReleaseSpinLock(&Adapter->SendLock);

        NdisAcquireSpinLock(&Adapter->RcvLock);        
        MP_DEC_RCV_REF(Adapter);
        Count = MP_GET_RCV_REF(Adapter);
        if (Count ==0)
        {
            Adapter->AdapterState = NicPaused;            
            Status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            Status = NDIS_STATUS_PENDING;
        }
        NdisReleaseSpinLock(&Adapter->RcvLock);    
        
    }
    while (FALSE);
    
    DBGPRINT(MP_TRACE, ("<==== MPPause\n"));
    return Status;
}
        
    
NDIS_STATUS 
MPRestart(
    IN  NDIS_HANDLE                         MiniportAdapterContext,    
    IN  PNDIS_MINIPORT_RESTART_PARAMETERS   MiniportRestartParameters
    )
/*++
 
Routine Description:
    
    MiniportRestart handler
    The driver resumes its normal working state
    
Argument:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING  Can it return pending
    NDIS_STATUS_XXX      The driver fails to restart


--*/
{
    PMP_ADAPTER                  Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    PNDIS_RESTART_ATTRIBUTES     NdisRestartAttributes;
    PNDIS_RESTART_GENERAL_ATTRIBUTES  NdisGeneralAttributes;

    UNREFERENCED_PARAMETER(MiniportRestartParameters);

    DBGPRINT(MP_TRACE, ("====> MPRestart\n"));
    
    NdisRestartAttributes = MiniportRestartParameters->RestartAttributes;

    //
    // If NdisRestartAttributes is not NULL, then miniport can modify generic attributes and add
    // new media specific info attributes at the end. Otherwise, NDIS restarts the miniport because 
    // of other reason, miniport should not try to modify/add attributes
    //
    if (NdisRestartAttributes != NULL)
    {

        ASSERT(NdisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES);
    
        NdisGeneralAttributes = (PNDIS_RESTART_GENERAL_ATTRIBUTES)NdisRestartAttributes->Data;
    
        //
        // Check to see if we need to change any attributes, for example, the driver can change the current
        // MAC address here. Or the driver can add media specific info attributes.
        //
    }
    NdisAcquireSpinLock(&Adapter->RcvLock);
    MP_INC_RCV_REF(Adapter);
    Adapter->AdapterState = NicRunning;
    NdisReleaseSpinLock(&Adapter->RcvLock);    

    DBGPRINT(MP_TRACE, ("<==== MPRestart\n"));
    return NDIS_STATUS_SUCCESS;

}
 
  
BOOLEAN 
MPCheckForHang(
    IN  NDIS_HANDLE     MiniportAdapterContext
    )
/*++

Routine Description:
    
    MiniportCheckForHang handler
    Ndis call this handler forcing the miniport to check if it needs reset or not,
    If the miniport needs reset, then it should call its reset function
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    None.

Note: 
    CheckForHang handler is called in the context of a timer DPC. 
    take advantage of this fact when acquiring/releasing spinlocks

    NOTE: NDIS60 Miniport won't have CheckForHang handler.

--*/
{
    PMP_ADAPTER         Adapter = (PMP_ADAPTER) MiniportAdapterContext;
    NDIS_MEDIA_CONNECT_STATE CurrMediaState;
    NDIS_STATUS         Status;
    PMP_TCB             pMpTcb;
    BOOLEAN             NeedReset = TRUE;
    NDIS_REQUEST_TYPE   RequestType;
    BOOLEAN             DispatchLevel = (NDIS_CURRENT_IRQL() == DISPATCH_LEVEL);
    

    do
    {
        //
        // any nonrecoverable hardware error?
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_NON_RECOVER_ERROR))
        {
            DBGPRINT(MP_WARN, ("Non recoverable error - remove\n"));
            break;
        }
            
        //
        // hardware failure?
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_HARDWARE_ERROR))
        {
            DBGPRINT(MP_WARN, ("hardware error - reset\n"));
            break;
        }
          
        //
        // Is send stuck?                  
        //
        MP_ACQUIRE_SPIN_LOCK(&Adapter->SendLock, DispatchLevel);
        if (Adapter->nBusySend > 0)
        {
            pMpTcb = Adapter->CurrSendHead;
            pMpTcb->Count++;
			if (pMpTcb->Count > NIC_SEND_HANG_THRESHOLD)
            {
                MP_RELEASE_SPIN_LOCK(&Adapter->SendLock, DispatchLevel);
				KdBreakPoint();
                DBGPRINT(MP_WARN, ("Send stuck - reset\n"));
                break;
            }
        }
    
 		NeedReset = FALSE;

        MP_RELEASE_SPIN_LOCK(&Adapter->SendLock, DispatchLevel);
        MP_ACQUIRE_SPIN_LOCK(&Adapter->RcvLock, DispatchLevel);

        //
        // Update the RECV_BUFFER shrink count                                          
        //
        if (Adapter->CurrNumRecvBuffer > Adapter->NumRecvBuffer)
        {
            Adapter->RecvBufferShrinkCount++;          
        }

        MP_RELEASE_SPIN_LOCK(&Adapter->RcvLock, DispatchLevel);
        MP_ACQUIRE_SPIN_LOCK(&Adapter->Lock, DispatchLevel);
        CurrMediaState = NICGetMediaState(Adapter);

        if (CurrMediaState != Adapter->MediaState)
        {
            Adapter->MediaState = CurrMediaState;
            MPIndicateLinkState(Adapter);
        }
        
        MP_RELEASE_SPIN_LOCK(&Adapter->Lock, DispatchLevel);
    }
    while (FALSE);
    //
    // If need reset, ask NDIS to reset the miniport
    //
    return NeedReset;
}


VOID MPHalt(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_HALT_ACTION        HaltAction
    )

/*++

Routine Description:
    
    MiniportHalt handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    HaltAction              The reason adapter is being halted

Return Value:

    None
    
--*/
{
    LONG            Count;

    PMP_ADAPTER     Adapter = (PMP_ADAPTER) MiniportAdapterContext;

    if (HaltAction == NdisHaltDeviceSurpriseRemoved)
    {
        //
        // do something here. For example make sure halt will not rely on hardware
        // to generate an interrupt in order to complete a pending operation.
        //
    }

    ASSERT(Adapter->AdapterState == NicPaused);
        
    MP_SET_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS);
                                           
    DBGPRINT(MP_TRACE, ("====> MPHalt\n"));

    //
    // Call Shutdown handler to disable interrupts and turn the hardware off 
    // by issuing a full reset. since we are not calling our shutdown handler
    // as a result of a bugcheck, we can use NdisShutdownPowerOff as the reason for
    // shutting down the adapter.
    //
    MPShutdown(MiniportAdapterContext, NdisShutdownPowerOff);
    
    //
    // Deregister interrupt as early as possible
    //
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_INTERRUPT_IN_USE))
    {
        NdisMDeregisterInterruptEx(Adapter->NdisInterruptHandle);                           
        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_INTERRUPT_IN_USE);
    }

    //NdisCancelTimerObject (Adapter->LinkDetectionTimerHandle);
    
    //
    // Decrement the ref count which was incremented in MPInitialize
    //
    Count = MP_DEC_REF(Adapter);

    //
    // Possible non-zero ref counts mean one or more of the following conditions: 
    // 1) Pending async shared memory allocation;
    // 2) DPC's are not finished (e.g. link detection)
    //
    if (Count)
    {
        DBGPRINT(MP_WARN, ("RefCount=%d --- waiting!\n", MP_GET_REF(Adapter)));

        while (TRUE)
        {
            if (NdisWaitEvent(&Adapter->ExitEvent, 2000))
            {
                break;
            }

            DBGPRINT(MP_WARN, ("RefCount=%d --- rewaiting!\n", MP_GET_REF(Adapter)));
        }
    }        

    //
    // Reset the PHY chip.  We do this so that after a warm boot, the PHY will
    // be in a known state, with auto-negotiation enabled.
    //
    ResetPhy(Adapter);

    //
    // Free the entire adapter object, including the shared memory structures.
    //
    MpFreeAdapter(Adapter);

    DBGPRINT(MP_TRACE, ("<==== MPHalt\n"));
}

NDIS_STATUS 
MPReset(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    OUT PBOOLEAN        AddressingReset
    )
/*++

Routine Description:
    
    MiniportReset handler
    
Arguments:

    AddressingReset         To let NDIS know whether we need help from it with our reset
    MiniportAdapterContext  Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_RESET_IN_PROGRESS
    NDIS_STATUS_HARD_ERRORS

Note:
    
--*/
{
    NDIS_STATUS         Status;
    PNDIS_OID_REQUEST   PendingRequest;
    PMP_ADAPTER         Adapter = (PMP_ADAPTER) MiniportAdapterContext;

    DBGPRINT(MP_TRACE, ("====> MPReset\n"));

    *AddressingReset = TRUE;

    NdisAcquireSpinLock(&Adapter->Lock);
    NdisDprAcquireSpinLock(&Adapter->SendLock);
    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    do
    {
        ASSERT(!MP_TEST_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS));
  
        //
        // Is this adapter already doing a reset?
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS))
        {
            Status = NDIS_STATUS_RESET_IN_PROGRESS;
            MP_EXIT;
        }

        MP_SET_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);
        //
        // Abort any pending request
        //
        if (Adapter->PendingRequest != NULL)
        {
            PendingRequest = Adapter->PendingRequest;
            Adapter->PendingRequest = NULL;

            NdisDprReleaseSpinLock(&Adapter->RcvLock);
            NdisDprReleaseSpinLock(&Adapter->SendLock);
            NdisReleaseSpinLock(&Adapter->Lock);
            
            NdisMOidRequestComplete(Adapter->AdapterHandle, 
                                    PendingRequest, 
                                    NDIS_STATUS_REQUEST_ABORTED);

            NdisAcquireSpinLock(&Adapter->Lock);
            NdisDprAcquireSpinLock(&Adapter->SendLock);
            NdisDprAcquireSpinLock(&Adapter->RcvLock);
    
        } 

        MPRemoveAllWakeUpPatterns(Adapter);

        //
        // Is this adapter going to be removed
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_NON_RECOVER_ERROR))
        {
           Status = NDIS_STATUS_HARD_ERRORS;
           if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_REMOVE_IN_PROGRESS))
           {
               MP_EXIT;
           }
           //                       
           // This is an unrecoverable hardware failure. 
           // We need to tell NDIS to remove this miniport
           // 
           MP_SET_FLAG(Adapter, fMP_ADAPTER_REMOVE_IN_PROGRESS);
           MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);
           
           NdisDprReleaseSpinLock(&Adapter->RcvLock);
           NdisDprReleaseSpinLock(&Adapter->SendLock);
           NdisReleaseSpinLock(&Adapter->Lock);
           
           NdisWriteErrorLogEntry(
               Adapter->AdapterHandle,
               NDIS_ERROR_CODE_HARDWARE_FAILURE,
               1,
               ERRLOG_REMOVE_MINIPORT);
           
           NdisMRemoveMiniport(Adapter->AdapterHandle);
           
           DBGPRINT_S(Status, ("<==== MPReset, Status=%x\n", Status));
            
           return Status;
        }   
                

        //
        // Disable the interrupt and issue a reset to the NIC
        //
        NICDisableInterrupt(Adapter);
        NICIssueSelectiveReset(Adapter);


        //
        // release all the locks and then acquire back the send lock
        // we are going to clean up the send queues
        // which may involve calling Ndis APIs
        // release all the locks before grabbing the send lock to
        // avoid deadlocks
        //
        NdisDprReleaseSpinLock(&Adapter->RcvLock);
        NdisDprReleaseSpinLock(&Adapter->SendLock);
        NdisReleaseSpinLock(&Adapter->Lock);
        
        NdisAcquireSpinLock(&Adapter->SendLock);


        //
        // This is a deserialized miniport, we need to free all the send packets
        // Free the packets on SendWaitList                                                           
        //
        MpFreeQueuedSendNetBufferLists(Adapter);

        //
        // Free the packets being actively sent & stopped
        //
        MpFreeBusySendNetBufferLists(Adapter);

#if DBG
        if (MP_GET_REF(Adapter) > 1)
        {
            DBGPRINT(MP_WARN, ("RefCount=%d\n", MP_GET_REF(Adapter)));
        }
#endif

        NdisZeroMemory(Adapter->MpTcbMem, Adapter->NumTcb * sizeof(MP_TCB));

        //
        // Re-initialize the send structures
        //
        NICInitSend(Adapter);
        
        NdisReleaseSpinLock(&Adapter->SendLock);

        //
        // get all the locks again in the right order
        //
        NdisAcquireSpinLock(&Adapter->Lock);
        NdisDprAcquireSpinLock(&Adapter->SendLock);
        NdisDprAcquireSpinLock(&Adapter->RcvLock);

        //
        // Reset the RECV_BUFFER list and re-start RU         
        //
        NICResetRecv(Adapter);
        Status = NICStartRecv(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            //
            // Are we having failures in a few consecutive resets?                  
            // 
            if (Adapter->HwErrCount < NIC_HARDWARE_ERROR_THRESHOLD)
            {
                //
                // It's not over the threshold yet, let it to continue
                // 
                Adapter->HwErrCount++;
            }
            else
            {
                //
                // This is an unrecoverable hardware failure. 
                // We need to tell NDIS to remove this miniport
                // 
                MP_SET_FLAG(Adapter, fMP_ADAPTER_REMOVE_IN_PROGRESS);
                MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);
                
                NdisDprReleaseSpinLock(&Adapter->RcvLock);
                NdisDprReleaseSpinLock(&Adapter->SendLock);
                NdisReleaseSpinLock(&Adapter->Lock);
                
                NdisWriteErrorLogEntry(
                    Adapter->AdapterHandle,
                    NDIS_ERROR_CODE_HARDWARE_FAILURE,
                    1,
                    ERRLOG_REMOVE_MINIPORT);
                     
                NdisMRemoveMiniport(Adapter->AdapterHandle);
                
                DBGPRINT_S(Status, ("<==== MPReset, Status=%x\n", Status));
                return Status;
            }
            
            break;
        }
        
        Adapter->HwErrCount = 0;
        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_HARDWARE_ERROR);

        NICEnableInterrupt(Adapter);

    } 
    while (FALSE);

    MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);

    exit:

    NdisDprReleaseSpinLock(&Adapter->RcvLock);
    NdisDprReleaseSpinLock(&Adapter->SendLock);
    NdisReleaseSpinLock(&Adapter->Lock);
        
    DBGPRINT_S(Status, ("<==== MPReset, Status=%x\n", Status));
    return(Status);
}

VOID 
MPReturnNetBufferLists(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  PNET_BUFFER_LIST    NetBufferLists,
    IN  ULONG               ReturnFlags
    )
/*++

Routine Description:
    
    MiniportReturnNetBufferLists handler
    NDIS calls this handler to return the ownership of one or more NetBufferLists and 
    their embedded NetBuffers to the miniport driver.
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    NetBufferLists          A linked list of NetBufferLists that miniport driver allocated for one or more 
                            previous receive indications. The list can include NetBuferLists from different
                            previous calls to NdisMIndicateNetBufferLists.
    ReturnFlags             Flags specifying if the caller is at DISPATCH level                            

Return Value:

    None

    
--*/
{
    PMP_ADAPTER         Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    PMP_RBD             pMpRbd;
    ULONG               Count;
    PNET_BUFFER_LIST    NetBufList;
    PNET_BUFFER_LIST    NextNetBufList;
    BOOLEAN             DispatchLevel;
	ULONG_PTR			ioaddr = Adapter->PortOffset ;

    DBGPRINT(MP_TRACE, ("====> MPReturnNetBufferLists\n"));
    //
    // Later we need to check if the request control size change
    // If yes, return the NetBufferList  to pool, and reallocate 
    // one the RECV_BUFFER
    // 
    DispatchLevel = NDIS_TEST_RETURN_AT_DISPATCH_LEVEL(ReturnFlags);

    MP_ACQUIRE_SPIN_LOCK(&Adapter->RcvLock, DispatchLevel);
    
    for (NetBufList = NetBufferLists;
            NetBufList != NULL;
            NetBufList = NextNetBufList)
    {
        NextNetBufList = NET_BUFFER_LIST_NEXT_NBL(NetBufList);

        pMpRbd = MP_GET_NET_BUFFER_LIST_RECV_BUFFER(NetBufList);

#if DBG_RCV_INTERRUPT
			KdPrint(("\n-%d",pMpRbd->HwRecvBufferNumber));
#endif
      
        ASSERT(pMpRbd);

        ASSERT(MP_TEST_FLAG(pMpRbd, fMP_RBD_RECV_PEND));
        MP_CLEAR_FLAG(pMpRbd, fMP_RBD_RECV_PEND);

        // remove from pending receive list
		RemoveEntryList((PLIST_ENTRY)pMpRbd);

        //
        // Decrement the Power Mgmt Ref.
        // 
        Adapter->PoMgmt.OutstandingRecv--;

        //
        // If we have set power request pending, then complete it
        //
        if (((Adapter->PendingRequest)
                && ((Adapter->PendingRequest->RequestType == NdisRequestSetInformation)
                && (Adapter->PendingRequest->DATA.SET_INFORMATION.Oid == OID_PNP_SET_POWER)))
                && (Adapter->PoMgmt.OutstandingRecv == 0))
        {
            MpSetPowerLowComplete(Adapter);
        }
            

        if (Adapter->RecvBufferShrinkCount < NIC_RECV_BUFFER_SHRINK_THRESHOLD
                || MP_TEST_FLAG(pMpRbd, fMP_RBD_ALLOC_FROM_MEM_CHUNK))
        {

            NICReturnRBD(Adapter, pMpRbd);
			
        }
        else
        {
            //
            // Need to shrink the count of RECV_BUFFERs, if the RECV_BUFFER is not allocated during
            // initialization time, free it.
            // 
            ASSERT(Adapter->CurrNumRecvBuffer > Adapter->NumRecvBuffer);

            Adapter->RecvBufferShrinkCount = 0;
            NICFreeRecvBuffer(Adapter, pMpRbd, TRUE);
            Adapter->CurrNumRecvBuffer--;

            DBGPRINT(MP_TRACE, ("Shrink... CurrNumRecvBuffer = %d\n", Adapter->CurrNumRecvBuffer));
        }

        
        //
        // note that we get the ref count here, but check
        // to see if it is zero and signal the event -after-
        // releasing the SpinLock. otherwise, we may let the Halthandler
        // continue while we are holding a lock.
        //
        MP_DEC_RCV_REF(Adapter);
    }
    

    Count =  MP_GET_RCV_REF(Adapter);
    if ((Count == 0) && (Adapter->AdapterState == NicPausing))
    {
        //
        // If all the NetBufferLists are returned and miniport is pausing,
        // complete the pause
        // 
        
        Adapter->AdapterState = NicPaused;
        MP_RELEASE_SPIN_LOCK(&Adapter->RcvLock, DispatchLevel);
        NdisMPauseComplete(Adapter->AdapterHandle);
        MP_ACQUIRE_SPIN_LOCK(&Adapter->RcvLock, DispatchLevel);
    }
    
    MP_RELEASE_SPIN_LOCK(&Adapter->RcvLock, DispatchLevel);
    
    //
    // Only halting the miniport will set AllPacketsReturnedEvent
    //
    if (Count == 0)
    {
        NdisSetEvent(&Adapter->AllPacketsReturnedEvent);
    }

    DBGPRINT(MP_TRACE, ("<==== MPReturnNetBufferLists\n"));
}


VOID 
MPSendNetBufferLists(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  PNET_BUFFER_LIST    NetBufferList,
    IN  NDIS_PORT_NUMBER    PortNumber,
    IN  ULONG               SendFlags
    )
/*++

Routine Description:
    
    MiniportSendNetBufferList handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    NetBufferList           Pointer to a list of Net Buffer Lists.
    Dispatchlevel           Whether that caller is at DISPATCH_LEVEL or not
    
Return Value:

    None
    
--*/
{

    PMP_ADAPTER         Adapter;
    NDIS_STATUS         Status = NDIS_STATUS_PENDING;
    UINT                NetBufferCount = 0;
    PNET_BUFFER         NetBuffer;
    PNET_BUFFER_LIST    CurrNetBufferList;
    PNET_BUFFER_LIST    NextNetBufferList;
    KIRQL               OldIrql = 0;
    BOOLEAN             DispatchLevel;
    
    DBGPRINT(MP_TRACE, ("====> MPSendNetBufferLists\n"));

    Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);
    do
    {
        //
        // If the adapter is in Pausing or paused state, just fail the send.
        //
        if ( (Adapter->AdapterState == NicPausing) \
			|| (Adapter->AdapterState == NicPaused)  \
			) 
        {
            Status =  NDIS_STATUS_PAUSED;
            break;
        }

        MP_ACQUIRE_SPIN_LOCK(&Adapter->SendLock, DispatchLevel);
        OldIrql = Adapter->SendLock.OldIrql;
    
        //
        // Is this adapter ready for sending?
        // 
        if (MP_IS_NOT_READY(Adapter))
        {
            //
            // there  is link
            //
            if ( Adapter->MediaState == MediaConnectStateConnected )
            {
                //
                // Insert Net Buffer List into the queue
                // 
                for (CurrNetBufferList = NetBufferList;
                        CurrNetBufferList != NULL;
                        CurrNetBufferList = NextNetBufferList)
                {
                    NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrNetBufferList);
                    //
                    // Get how many net buffers inside the net buffer list
                    //
                    
                    NetBufferCount = 0;
                    for (NetBuffer = NET_BUFFER_LIST_FIRST_NB(CurrNetBufferList);
                            NetBuffer != NULL; 
                            NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
                    {
                        NetBufferCount++;
                    }
                    ASSERT(NetBufferCount > 0);
                    MP_GET_NET_BUFFER_LIST_REF_COUNT(CurrNetBufferList) = NetBufferCount;
                    MP_GET_NET_BUFFER_LIST_NEXT_SEND(CurrNetBufferList) = NET_BUFFER_LIST_FIRST_NB(CurrNetBufferList);
                    NET_BUFFER_LIST_STATUS(CurrNetBufferList) = NDIS_STATUS_SUCCESS;
                    InsertTailQueue(&Adapter->SendWaitQueue, 
                               MP_GET_NET_BUFFER_LIST_LINK(CurrNetBufferList));
                    Adapter->nWaitSend++;
                    DBGPRINT(MP_WARN, ("MPSendNetBufferLists: link detection - queue NetBufferList "PTR_FORMAT"\n", CurrNetBufferList));
                }
                Adapter->SendLock.OldIrql = OldIrql;
                MP_RELEASE_SPIN_LOCK(&Adapter->SendLock, DispatchLevel);
                
                break;
            }
        
            //
            // Adapter is not ready and there is not link
            //
            Status = MP_GET_STATUS_FROM_FLAGS(Adapter);
            
            Adapter->SendLock.OldIrql = OldIrql;  
            MP_RELEASE_SPIN_LOCK(&Adapter->SendLock, DispatchLevel);
            break;
        }

        //
        // Adapter is ready, send this net buffer list, in this case, we always return pending     
        //
        
		for (CurrNetBufferList = NetBufferList;
                CurrNetBufferList != NULL;
                CurrNetBufferList = NextNetBufferList)
        {
            NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrNetBufferList);
            //
            // Get how many net buffers inside the net buffer list
            //
            NetBufferCount = 0;
            for (NetBuffer = NET_BUFFER_LIST_FIRST_NB(CurrNetBufferList);
                    NetBuffer != NULL; 
                    NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
            {
                NetBufferCount++;
            }
            ASSERT(NetBufferCount > 0);
            MP_GET_NET_BUFFER_LIST_REF_COUNT(CurrNetBufferList) = NetBufferCount;
            //
            // queue is not empty or tcb is not available, or another thread is sending
            // a NetBufferList.
            //
            if (!IsQueueEmpty(&Adapter->SendWaitQueue) || 
                (!MP_TCB_RESOURCES_AVAIABLE(Adapter) ||
                 Adapter->SendingNetBufferList != NULL))
            {
                //
                // The first net buffer is the buffer to send
                //
                MP_GET_NET_BUFFER_LIST_NEXT_SEND(CurrNetBufferList) = NET_BUFFER_LIST_FIRST_NB(CurrNetBufferList);
                NET_BUFFER_LIST_STATUS(CurrNetBufferList) = NDIS_STATUS_SUCCESS;
                InsertTailQueue(&Adapter->SendWaitQueue, 
                            MP_GET_NET_BUFFER_LIST_LINK(CurrNetBufferList));
                Adapter->nWaitSend++;
            }
            else
            {
                Adapter->SendingNetBufferList = CurrNetBufferList;
                NET_BUFFER_LIST_STATUS(CurrNetBufferList) = NDIS_STATUS_SUCCESS;
                MiniportSendNetBufferList(Adapter, CurrNetBufferList, FALSE);
            }
                
        }
    
        Adapter->SendLock.OldIrql = OldIrql;
        MP_RELEASE_SPIN_LOCK(&Adapter->SendLock, DispatchLevel);
    }
    while (FALSE);

    if (Status != NDIS_STATUS_PENDING)
    {
        ULONG SendCompleteFlags = 0;
        
        for (CurrNetBufferList = NetBufferList;
                 CurrNetBufferList != NULL;
                 CurrNetBufferList = NextNetBufferList)
        {
            NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrNetBufferList);
            NET_BUFFER_LIST_STATUS(CurrNetBufferList) = Status;
        }

        if ( NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags) )
        {
            NDIS_SET_SEND_COMPLETE_FLAG(SendCompleteFlags, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
        }
                  
        NdisMSendNetBufferListsComplete(
            MP_GET_ADAPTER_HANDLE(Adapter),
            NetBufferList,
            SendCompleteFlags);   
    }
    DBGPRINT(MP_TRACE, ("<==== MPSendNetBufferLists\n"));
}

VOID MPShutdown(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_SHUTDOWN_ACTION    ShutdownAction
    )
/*++

Routine Description:
    
    MiniportShutdown handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    ShutdownAction          The reason for Shutdown

Return Value:

    None
    
--*/
{
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;

    UNREFERENCED_PARAMETER(ShutdownAction);
    
    DBGPRINT(MP_TRACE, ("====> MPShutdown\n"));

    //
    // Disable interrupt and issue a full reset
    //
    NICDisableInterrupt(Adapter);
    NICIssueFullReset(Adapter);

    DBGPRINT(MP_TRACE, ("<==== MPShutdown\n"));
}

VOID 
MPAllocateComplete(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PVOID                   VirtualAddress,
    IN  PNDIS_PHYSICAL_ADDRESS  PhysicalAddress,
    IN  ULONG                   Length,
    IN  PVOID                   Context)
/*++

Routine Description:
    
    MiniportAllocateComplete handler
    This handler is needed because we make calls to NdisMAllocateSharedMemoryAsync
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    VirtualAddress          Pointer to the allocated memory block 
    PhysicalAddress         Physical address of the memory block       
    Length                  Length of the memory block                
    Context                 Context in NdisMAllocateSharedMemoryAsyncEx              

Return Value:

    None
    
--*/
{
    ULONG           ErrorValue;
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    PMP_RBD         pMpRbd = (PMP_RBD)Context;

    DBGPRINT(MP_TRACE, ("==== MPAllocateComplete\n"));

    ASSERT(pMpRbd);
    ASSERT(MP_TEST_FLAG(pMpRbd, fMP_RBD_ALLOC_PEND));
    MP_CLEAR_FLAG(pMpRbd, fMP_RBD_ALLOC_PEND);

    NdisAcquireSpinLock(&Adapter->RcvLock);

    //
    // Is allocation successful?  
    //
    if (VirtualAddress)
    {
        ASSERT(Length == Adapter->HwRecvBufferSize);
        
        pMpRbd->OriginalHwRecvBuffer = (PHW_RECV_BUFFER)VirtualAddress;
        pMpRbd->OriginalHwRecvBufferPa = *PhysicalAddress;

        //
        // First get a HwRecvBuffer at 8 byte boundary from OriginalHwRecvBuffer
        //
        pMpRbd->HwRecvBuffer = (PHW_RECV_BUFFER)DATA_ALIGN(pMpRbd->OriginalHwRecvBuffer);
        //
        // Then shift HwRecvBuffer so that the data(after ethernet header) is at 8 bytes boundary
        //
        pMpRbd->HwRecvBuffer = (PHW_RECV_BUFFER)((PUCHAR)pMpRbd->HwRecvBuffer + HWRECV_BUFFER_SHIFT_OFFSET);
        //
        // Update physical address as well
        //
        pMpRbd->HwRecvBufferPa.QuadPart = pMpRbd->OriginalHwRecvBufferPa.QuadPart 
                                   + BYTES_SHIFT(pMpRbd->HwRecvBuffer, pMpRbd->OriginalHwRecvBuffer);
        
        ErrorValue = NICAllocRBD(Adapter, pMpRbd);
        if (ErrorValue == 0)
        {
            // Add this RECV_BUFFER to the RecvList
            Adapter->CurrNumRecvBuffer++;                      
            NICReturnRBD(Adapter, pMpRbd);

            ASSERT(Adapter->CurrNumRecvBuffer <= Adapter->MaxNumRecvBuffer);
            DBGPRINT(MP_TRACE, ("CurrNumRecvBuffer=%d\n", Adapter->CurrNumRecvBuffer));
        }
        else
        {
            NdisFreeToNPagedLookasideList(&Adapter->RecvLookaside, pMpRbd);
        }
    }
    else
    {
        NdisFreeToNPagedLookasideList(&Adapter->RecvLookaside, pMpRbd);
    }

    Adapter->bAllocNewRecvBuffer = FALSE;
    MP_DEC_REF(Adapter);

    if (MP_GET_REF(Adapter) == 0)
    {
        NdisSetEvent(&Adapter->ExitEvent);
    }

    NdisReleaseSpinLock(&Adapter->RcvLock);
}

BOOLEAN 
MPIsr(
    IN  NDIS_HANDLE     MiniportInterruptContext,
    OUT PBOOLEAN        QueueMiniportInterruptDpcHandler,
    OUT PULONG          TargetProcessors)
/*++

Routine Description:
    
    MiniportIsr handler
    
Arguments:

    MiniportInterruptContext: Pointer to the interrupt context.
        In our case, this is a pointer to our adapter structure.
        
    QueueMiniportInterruptDpcHandler: TRUE on return if MiniportHandleInterrupt 
        should be called on default CPU
        
    TargetProcessors: Pointer to a bitmap specifying 
        Target processors which should run the DPC

Return Value:

    TRUE  --- The miniport recognizes the interrupt
    FALSE   --- Otherwise
    
--*/
{
    PMP_ADAPTER  Adapter = (PMP_ADAPTER)MiniportInterruptContext;
    BOOLEAN                         InterruptRecognized = FALSE;
	ULONG_PTR		ioaddr = Adapter->PortOffset ;
    
    DBGPRINT(MP_LOUD, ("====> MPIsr\n"));
    
    do 
    {
        
        //
        // If the adapter is in low power state, then it should not 
        // recognize any interrupt
        // 
        if (Adapter->CurrentPowerState > NdisDeviceStateD0)
        {
            InterruptRecognized = FALSE;
            *QueueMiniportInterruptDpcHandler = FALSE;
            break;
        }
        //
        // We process the interrupt if it's not disabled and it's active                  
        //
        if ( !NIC_INTERRUPT_DISABLED(Adapter) && NIC_INTERRUPT_ACTIVE(Adapter))
        {
            InterruptRecognized = TRUE;

			*QueueMiniportInterruptDpcHandler = TRUE;

            //
            // Disable the interrupt (will be re-enabled in MPHandleInterrupt
            //
  			NICDisableInterrupt(Adapter);
          
        }
        else
        {
            InterruptRecognized = FALSE;
            *QueueMiniportInterruptDpcHandler = FALSE;
        }
    }
    while (FALSE);    

    DBGPRINT(MP_LOUD, ("<==== MPIsr\n"));
    
    return InterruptRecognized;
}


VOID
MPHandleInterrupt(
    IN  NDIS_HANDLE  MiniportInterruptContext,
    IN  PVOID        MiniportDpcContext,
    IN  PULONG       NdisReserved1,
    IN  PULONG       NdisReserved2
    )
/*++

Routine Description:
    
    MiniportHandleInterrupt handler
    
Arguments:

    MiniportInterruptContext:  Pointer to the interrupt context.
        In our case, this is a pointer to our adapter structure.

Return Value:

    None
    
--*/
{
    PMP_ADAPTER  Adapter = (PMP_ADAPTER)MiniportInterruptContext;
    USHORT       IntStatus;
	ULONG_PTR	ioaddr = Adapter->PortOffset ;

    UNREFERENCED_PARAMETER(MiniportDpcContext);
    UNREFERENCED_PARAMETER(NdisReserved1);
    UNREFERENCED_PARAMETER(NdisReserved2);
    //
    // Acknowledge the interrupt(s) and get the interrupt status
    //
    
	// Here and in most of the places at DPC level you will see 
	// NdisMSynchronizeWithInterruptEx(.,,.,) ** NOT ** being used.
	// Reason :: ISR cannot happen, because device interrupt is disabled.
	NIC_ACK_INTERRUPT(Adapter, IntStatus);

	MpHandleLinkInterrupt ( Adapter, IntStatus ) ;

    //    
    // Handle receive interrupt    
    //
    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    MpHandleRecvInterrupt(Adapter);

    NdisDprReleaseSpinLock(&Adapter->RcvLock);
    
    //
    // Handle send interrupt    
    //
    NdisDprAcquireSpinLock(&Adapter->SendLock);

    MpHandleSendInterrupt(Adapter);

    NdisDprReleaseSpinLock(&Adapter->SendLock);

    //
    // Start the receive unit if it had stopped
    //
    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    //
    // If the receiver is ready, then don't try to restart.
    //
    if (!NIC_IS_RECV_READY(Adapter))
		RealtekHwCommand(Adapter, SCB_RUC_START, FALSE);

    NdisDprReleaseSpinLock(&Adapter->RcvLock);

	//
    // Re-enable the interrupt (disabled in MPIsr)
    //
    NdisMSynchronizeWithInterruptEx(
        Adapter->NdisInterruptHandle,
        0,
        NICEnableInterrupt,
        Adapter);
}

VOID 
MPCancelSendNetBufferLists(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PVOID           CancelId
    )
/*++

Routine Description:
    
    MiniportCancelNetBufferLists handler - This function walks through all 
    of the queued send NetBufferLists and cancels all the NetBufferLists that
    have the correct CancelId
    
Arguments:

    MiniportAdapterContext      Pointer to our adapter
    CancelId                    All the Net Buffer Lists with this Id should be cancelled

Return Value:

    None
    
--*/
{
    PMP_ADAPTER         Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    PQUEUE_ENTRY        pEntry, pPrevEntry, pNextEntry;
    PNET_BUFFER_LIST    NetBufferList;
    PNET_BUFFER_LIST    CancelHeadNetBufferList = NULL;
    PNET_BUFFER_LIST    CancelTailNetBufferList = NULL;
    PVOID               NetBufferListId;
    
    DBGPRINT(MP_TRACE, ("====> MPCancelSendNetBufferLists\n"));

    pPrevEntry = NULL;

    NdisAcquireSpinLock(&Adapter->SendLock);

    //
    // Walk through the send wait queue and complete the sends with matching Id
    //
    do
    {

        if (IsQueueEmpty(&Adapter->SendWaitQueue))
        {
            break;
        }
        
        pEntry = GetHeadQueue(&Adapter->SendWaitQueue); 

        while (pEntry != NULL)
        {
            NetBufferList = MP_GET_NET_BUFFER_LIST_FROM_QUEUE_LINK(pEntry);
    
            NetBufferListId = NDIS_GET_NET_BUFFER_LIST_CANCEL_ID(NetBufferList);

            if ((NetBufferListId == CancelId)
                    && (NetBufferList != Adapter->SendingNetBufferList))
            {
                NET_BUFFER_LIST_STATUS(NetBufferList) = NDIS_STATUS_REQUEST_ABORTED;
                Adapter->nWaitSend--;
                //
                // This packet has the right CancelId
                //
                pNextEntry = pEntry->Next;

                if (pPrevEntry == NULL)
                {
                    Adapter->SendWaitQueue.Head = pNextEntry;
                    if (pNextEntry == NULL)
                    {
                        Adapter->SendWaitQueue.Tail = NULL;
                    }
                }
                else
                {
                    pPrevEntry->Next = pNextEntry;
                    if (pNextEntry == NULL)
                    {
                        Adapter->SendWaitQueue.Tail = pPrevEntry;
                    }
                }

                pEntry = pEntry->Next;

                //
                // Queue this NetBufferList for cancellation
                //
                if (CancelHeadNetBufferList == NULL)
                {
                    CancelHeadNetBufferList = NetBufferList;
                    CancelTailNetBufferList = NetBufferList;
                }
                else
                {
                    NET_BUFFER_LIST_NEXT_NBL(CancelTailNetBufferList) = NetBufferList;
                    CancelTailNetBufferList = NetBufferList;
                }
            }
            else
            {
                // This packet doesn't have the right CancelId
                pPrevEntry = pEntry;
                pEntry = pEntry->Next;
            }
        }
    } while (FALSE);
    
    NdisReleaseSpinLock(&Adapter->SendLock);
    
    //
    // Get the packets from SendCancelQueue and complete them if any
    //
    if (CancelHeadNetBufferList != NULL)
    {
        NET_BUFFER_LIST_NEXT_NBL(CancelTailNetBufferList) = NULL;

        NdisMSendNetBufferListsComplete(
               MP_GET_ADAPTER_HANDLE(Adapter),
               CancelHeadNetBufferList,
               0);   
    } 

    DBGPRINT(MP_TRACE, ("<==== MPCancelSendNetBufferLists\n"));
    return;

}

VOID 
MPPnPEventNotify(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent
    )
/*++

Routine Description:
    
    MiniportPnPEventNotify handler - NDIS51 and later
    
Arguments:

    MiniportAdapterContext      Pointer to our adapter
    PnPEvent                    Self-explanatory 
    InformationBuffer           Self-explanatory 
    InformationBufferLength     Self-explanatory 

Return Value:

    None
    
--*/
{
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    NDIS_DEVICE_PNP_EVENT   PnPEvent = NetDevicePnPEvent->DevicePnPEvent;
    PVOID                   InformationBuffer = NetDevicePnPEvent->InformationBuffer;
    ULONG                   InformationBufferLength = NetDevicePnPEvent->InformationBufferLength;

    //
    // Turn off the warings.
    //
    UNREFERENCED_PARAMETER(InformationBuffer);
    UNREFERENCED_PARAMETER(InformationBufferLength);
    UNREFERENCED_PARAMETER(Adapter);
    
    DBGPRINT(MP_TRACE, ("====> MPPnPEventNotify\n"));

    switch (PnPEvent)
    {
        case NdisDevicePnPEventQueryRemoved:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventQueryRemoved\n"));
            break;

        case NdisDevicePnPEventRemoved:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventRemoved\n"));
            break;       

        case NdisDevicePnPEventSurpriseRemoved:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventSurpriseRemoved\n"));
            break;

        case NdisDevicePnPEventQueryStopped:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventQueryStopped\n"));
            break;

        case NdisDevicePnPEventStopped:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventStopped\n"));
            break;      
            
        case NdisDevicePnPEventPowerProfileChanged:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventPowerProfileChanged\n"));
            break;      
            
        default:
            DBGPRINT(MP_ERROR, ("MPPnPEventNotify: unknown PnP event %x \n", PnPEvent));
            break;         
    }

    DBGPRINT(MP_TRACE, ("<==== MPPnPEventNotify\n"));

}


VOID 
MPUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )
/*++

Routine Description:
    
    The Unload handler
    This handler is registered through NdisMRegisterUnloadHandler
    
Arguments:

    DriverObject        Not used

Return Value:

    None
    
--*/
{
    //
    // Deregister Miniport driver
    //
    NdisMDeregisterMiniportDriver(NdisMiniportDriverHandle);
}


