/*
    SNMP
    Version management
    Copyright (c) 2014, eForce Co., Ltd. All rights reserved.
    
    2014-12-08 Created
    2019-11-12 Include net_hdr.h for definition of OS type
*/

#include "kernel.h"
#include "net_hdr.h"
#include "snmp_lib.h"

char *unet3snmp_get_version(unsigned char mode)
{
    if(mode)
    {
        return UNET3SNMP_VERSION" (build:"__DATE__" "__TIME__")";
    }
    else
    {
        return UNET3SNMP_VERSION;
    }
}

