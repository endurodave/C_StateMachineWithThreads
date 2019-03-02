#ifndef _SYS_DATA_H
#define _SYS_DATA_H

#include "callback.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    STARTING,
    NORMAL,
    SERVICE,
    SYS_INOP
} SystemModeType;

typedef struct
{
    SystemModeType PreviousSystemMode;
    SystemModeType CurrentSystemMode;
} SystemModeData;

// Declare a public SysData callback interface
CB_DECLARE(SystemModeChangedCb, const SystemModeData*)

void SD_Init(void);
void SD_Term(void);
void SD_SetSystemMode(SystemModeType systemMode);

#ifdef __cplusplus
}
#endif

#endif