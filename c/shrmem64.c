

/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

  SPDX-License-Identifier: EPL-2.0

  Copyright Contributors to the Zowe Project.
*/

#ifdef METTLE
#include <metal/metal.h>
#include <metal/stdbool.h>
#include <metal/stddef.h>
#include <metal/stdint.h>
#else
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#endif

#include "shrmem64.h"
#include "zos.h"

#ifndef _LP64
#error ILP32 is not supported
#endif

typedef uint64_t MemObj;

#define IARV64_V4PLIST_SIZE 160

static bool isIARV64OK(int iarv64RC) {
  return iarv64RC < 8;
}

static int makeRSN(int shrmem64RC, int iarv64RC, int iarv64RSN) {

  int rc = ((unsigned)shrmem64RC << 24) |
           ((unsigned)iarv64RC << 16) |
           ((iarv64RSN >> 8) & 0x0000FFFF);

  return rc;
}

static MemObj getSharedMemObject(uint64_t segmentCount,
                                 MemObjToken token,
                                 uint32_t *iarv64RC,
                                 uint32_t *iarv64RSN) {

  MemObj result = 0;
  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=GETSHARED"
      ",USERTKN=(%[token])"
      ",COND=YES"
      ",SEGMENTS=(%[size])"
      ",ORIGIN=(%[result])"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [token]"r"(&token), [size]"r"(&segmentCount), [result]"r"(&result),
        [parm]"r"(&parmList)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

  return result;
}

static MemObj getCommonMemObject(uint64_t segmentCount,
                                 MemObjToken token,
				 int key,
                                 uint32_t *iarv64RC,
                                 uint32_t *iarv64RSN){

  MemObj result = 0;
  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};
  char keyByte = key & 0xF;
  keyByte = (keyByte << 4);  /* because there's always one more thing in MVS */

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=GETCOMMON"
      ",MOTKN=(%[token])"
      ",COND=YES"
      ",KEY=%[key]"
      ",FPROT=NO"
      ",SEGMENTS=(%[size])"
      ",ORIGIN=(%[result])"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [token]"r"(&token), [size]"r"(&segmentCount), [result]"r"(&result),
        [parm]"r"(&parmList),[key]"m"(keyByte)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

  return result;
}

static void shareMemObject(MemObj object,
                           MemObjToken token,
                           uint32_t *iarv64RC,
                           uint32_t *iarv64RSN) {

  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};

  struct {
    MemObj vsa;
    uint64_t reserved;
  } rangeList = {object, 0};

  uint64_t rangeListAddress = (uint64_t)&rangeList;

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=SHAREMEMOBJ"
      ",USERTKN=(%[token])"
      ",RANGLIST=(%[range])"
      ",NUMRANGE=1"
      ",COND=YES"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [token]"r"(&token), [range]"r"(&rangeListAddress), [parm]"r"(&parmList)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

}

static void detachSingleSharedMemObject(MemObj object,
                                        MemObjToken token,
                                        uint32_t *iarv64RC,
                                        uint32_t *iarv64RSN) {

  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=DETACH"
      ",MATCH=SINGLE"
      ",MEMOBJSTART=(%[mobj])"
      ",MOTKN=(%[token])"
      ",MOTKNCREATOR=USER"
      ",AFFINITY=LOCAL"
      ",OWNER=YES"
      ",COND=YES"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [mobj]"r"(&object), [parm]"r"(&parmList), [token]"r"(&token)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

}

static void detachSingleSharedMemObjectNotOwner(MemObj object,
						MemObjToken token,
						uint32_t *iarv64RC,
						uint32_t *iarv64RSN) {
  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=DETACH"
      ",MATCH=SINGLE"
      ",MEMOBJSTART=(%[mobj])"
      ",MOTKN=(%[token])"
      ",MOTKNCREATOR=USER"
      ",AFFINITY=LOCAL"
      ",OWNER=NO"
      ",COND=YES"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [mobj]"r"(&object), [parm]"r"(&parmList), [token]"r"(&token)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

}

static void detachSharedMemObjects(MemObjToken token,
                                   uint32_t *iarv64RC,
                                   uint32_t *iarv64RSN) {

  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=DETACH"
      ",MATCH=MOTOKEN"
      ",MOTKN=(%[token])"
      ",MOTKNCREATOR=USER"
      ",AFFINITY=LOCAL"
      ",OWNER=YES"
      ",COND=YES"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [token]"r"(&token), [parm]"r"(&parmList)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

}


static void removeSystemInterestForAllObjects(MemObjToken token,
                                              uint32_t *iarv64RC,
                                              uint32_t *iarv64RSN) {

  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=DETACH"
      ",MATCH=MOTOKEN"
      ",MOTKN=(%[token])"
      ",MOTKNCREATOR=USER"
      ",AFFINITY=SYSTEM"
      ",COND=YES"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [token]"r"(&token), [parm]"r"(&parmList)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

}

static void removeSystemInterestForSingleObject(MemObj object,
                                                MemObjToken token,
                                                uint32_t *iarv64RC,
                                                uint32_t *iarv64RSN) {

  int localRC = 0;
  int localRSN = 0;
  char parmList[IARV64_V4PLIST_SIZE] = {0};

  __asm(
      ASM_PREFIX
      "         IARV64 REQUEST=DETACH"
      ",MATCH=SINGLE"
      ",MEMOBJSTART=(%[mobj])"
      ",MOTKN=(%[token])"
      ",MOTKNCREATOR=USER"
      ",AFFINITY=SYSTEM"
      ",COND=YES"
      ",RETCODE=%[rc]"
      ",RSNCODE=%[rsn]"
      ",PLISTVER=4"
      ",MF=(E,(%[parm]),COMPLETE)                                              \n"
      : [rc]"=m"(localRC), [rsn]"=m"(localRSN)
      : [mobj]"r"(&object), [token]"r"(&token), [parm]"r"(&parmList)
      : "r0", "r1", "r14", "r15"
  );

  if (iarv64RC) {
    *iarv64RC = localRC;
  }
  if (iarv64RSN) {
    *iarv64RSN = localRSN;
  }

}

static ASCB *localGetASCB(){
  int *mem = (int*)0;
  return (ASCB*)(mem[CURRENT_ASCB/sizeof(int)]&0x7FFFFFFF);
}

MemObjToken shrmem64GetAddressSpaceToken(void) {

  union {
    MemObj token;
    struct {
      ASCB * __ptr32 ascb;
      uint32_t asid;
    };
  } result = { .ascb = localGetASCB(), .asid = result.ascb->ascbasid };

  return result.token;
}

int shrmem64Alloc(MemObjToken userToken, size_t size, void **result, int *rsn) {

  uint32_t iarv64RC = 0, iarv64RSN = 0;

  /*
   * Convert size in bytes into segments (megabytes), round up if necessary.
   */
  uint64_t segmentCount = 0;
  if ((size & 0xFFFFF) == 0) {
    segmentCount = size >> 20;
  } else{
    segmentCount = (size >> 20) + 1;
  }

  MemObj mobj = getSharedMemObject(segmentCount, userToken,
                                   &iarv64RC, &iarv64RSN);
  if (!isIARV64OK(iarv64RC)) {
    *rsn = makeRSN(RC_SHRMEM64_GETSHARED_FAILED, iarv64RC, iarv64RSN);
    return RC_SHRMEM64_GETSHARED_FAILED;
  }

  *result = (void *)mobj;

  return RC_SHRMEM64_OK;
}

int shrmem64CommonAlloc(MemObjToken userToken, size_t size, void **result, int *rsn) {
  return shrmem64CommonAlloc2(userToken,size,0,result,rsn);
}

int shrmem64CommonAlloc2(MemObjToken userToken, size_t size, int key, void **result, int *rsn) {

  uint32_t iarv64RC = 0, iarv64RSN = 0;

  /*
   * Convert size in bytes into segments (megabytes), round up if necessary.
   */
  uint64_t segmentCount = 0;
  if ((size & 0xFFFFF) == 0) {
    segmentCount = size >> 20;
  } else{
    segmentCount = (size >> 20) + 1;
  }

  MemObj mobj = getCommonMemObject(segmentCount, userToken, key,
                                   &iarv64RC, &iarv64RSN);
  if (!isIARV64OK(iarv64RC)) {
    *rsn = makeRSN(RC_SHRMEM64_GETCOMMON_FAILED, iarv64RC, iarv64RSN);
    return RC_SHRMEM64_GETSHARED_FAILED;
  }

  *result = (void *)mobj;

  return RC_SHRMEM64_OK;
}

int shrmem64Release(MemObjToken userToken, void *target, int *rsn) {

  uint32_t iarv64RC = 0, iarv64RSN = 0;

  MemObj mobj = (MemObj)target;

  removeSystemInterestForSingleObject(mobj, userToken, &iarv64RC, &iarv64RSN);
  if (!isIARV64OK(iarv64RC)) {
    *rsn = makeRSN(RC_SHRMEM64_SINGLE_SYS_DETACH_FAILED, iarv64RC, iarv64RSN);
    return RC_SHRMEM64_SINGLE_SYS_DETACH_FAILED;
  }

  return RC_SHRMEM64_OK;
}

int shrmem64ReleaseAll(MemObjToken userToken, int *rsn) {

  uint32_t iarv64RC = 0, iarv64RSN = 0;

  removeSystemInterestForAllObjects(userToken, &iarv64RC, &iarv64RSN);
  if (!isIARV64OK(iarv64RC)) {
    *rsn = makeRSN(RC_SHRMEM64_ALL_SYS_DETACH_FAILED, iarv64RC, iarv64RSN);
    return RC_SHRMEM64_ALL_SYS_DETACH_FAILED;
  }

  return RC_SHRMEM64_OK;
}

int shrmem64GetAccess(MemObjToken userToken, void *target, int *rsn) {

  uint32_t iarv64RC = 0, iarv64RSN = 0;

  MemObj mobj = (MemObj)target;

  shareMemObject(mobj, userToken, &iarv64RC, &iarv64RSN);
  if (!isIARV64OK(iarv64RC)) {
    *rsn = makeRSN(RC_SHRMEM64_SHAREMEMOBJ_FAILED, iarv64RC, iarv64RSN);
    return RC_SHRMEM64_SHAREMEMOBJ_FAILED;
  }

  return RC_SHRMEM64_OK;
}

int shrmem64RemoveAccess(MemObjToken userToken, void *target, int *rsn) {

  uint32_t iarv64RC = 0, iarv64RSN = 0;

  MemObj mobj = (MemObj)target;

  detachSingleSharedMemObject(mobj, userToken, &iarv64RC, &iarv64RSN);
  if (!isIARV64OK(iarv64RC)) {
    *rsn = makeRSN(RC_SHRMEM64_DETACH_FAILED, iarv64RC, iarv64RSN);
    return RC_SHRMEM64_DETACH_FAILED;
  }

  return RC_SHRMEM64_OK;
}

int shrmem64RemoveAccess2(MemObjToken userToken, void *target, bool isOwner, int *rsn) {

  uint32_t iarv64RC = 0, iarv64RSN = 0;

  MemObj mobj = (MemObj)target;

  if (isOwner){
    detachSingleSharedMemObject(mobj, userToken, &iarv64RC, &iarv64RSN);
  } else{
    detachSingleSharedMemObjectNotOwner(mobj, userToken, &iarv64RC, &iarv64RSN);
  }
  if (!isIARV64OK(iarv64RC)) {
    *rsn = makeRSN(RC_SHRMEM64_DETACH_FAILED, iarv64RC, iarv64RSN);
    return RC_SHRMEM64_DETACH_FAILED;
  }

  return RC_SHRMEM64_OK;
}


/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

  SPDX-License-Identifier: EPL-2.0

  Copyright Contributors to the Zowe Project.
*/
