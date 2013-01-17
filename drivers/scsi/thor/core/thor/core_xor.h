#ifndef CORE_XOR_H
#define CORE_XOR_H

#ifdef RAID_DRIVER
void Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request pXORReq);
void MV_DumpXORRegister(MV_PVOID This);
MV_BOOLEAN HandleXORQueue(MV_PVOID This);
#endif /* RAID_DRIVER */

#endif


