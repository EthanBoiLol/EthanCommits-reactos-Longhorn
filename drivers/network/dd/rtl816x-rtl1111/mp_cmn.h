/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    mp_cmn.h

Abstract:
    Common definitions for the miniport and kd extension dll

Revision History:

Notes:

--*/

#ifndef _MP_CMN_H
#define _MP_CMN_H

// MP_TCB flags
#define fMP_TCB_IN_USE                         0x00000001
#define fMP_TCB_USE_LOCAL_BUF                  0x00000002
#define fMP_TCB_MULTICAST                      0x00000004  // a hardware workaround using multicast

#define fMP_TCB_WAIT_FOR_MAPPING               0x10000000
#define fMP_TCB_FLAGS_MAPPING                  0x30000000

// MP_RBD flags                           
#define fMP_RBD_RECV_PEND                      0x00000001
#define fMP_RBD_ALLOC_PEND                     0x00000002
#define fMP_RBD_RECV_READY                     0x00000004
#define fMP_RBD_RESOURCES                      0x00000008
#define fMP_RBD_ALLOC_FROM_MEM_CHUNK           0x00000010

// MP_ADAPTER flags               
#define fMP_ADAPTER_SCATTER_GATHER             0x00000001
#define fMP_ADAPTER_RECV_LOOKASIDE             0x00000004
#define fMP_ADAPTER_INTERRUPT_IN_USE           0x00000008
#define fMP_ADAPTER_SECONDARY                  0x00000010

#define fMP_ADAPTER_NON_RECOVER_ERROR          0x00800000

#define fMP_ADAPTER_RESET_IN_PROGRESS          0x01000000
#define fMP_ADAPTER_NO_CABLE                   0x02000000 
#define fMP_ADAPTER_HARDWARE_ERROR             0x04000000
#define fMP_ADAPTER_REMOVE_IN_PROGRESS         0x08000000
#define fMP_ADAPTER_HALT_IN_PROGRESS           0x10000000
                             
#define fMP_ADAPTER_FAIL_SEND_MASK             0x1ff00000                
#define fMP_ADAPTER_NOT_READY_MASK             0x3ff00000    


#endif  // _MP_CMN_H



