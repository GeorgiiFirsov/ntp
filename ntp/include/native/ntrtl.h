/**
 * @file ntrtl.h
 * @brief Contains necessary declarations/definitions of internal windows objects and functions
 */

#pragma once

#include <windows.h>
#include <winternl.h>


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus

/**
 * @brief RTL fat read/write lock structure.
 *
 * A fat read/write lock works exactly like the SRW locks featured in the Windows SDK.
 * The main difference is that these fat versions can be acquired recursively. Access
 * cannot be upgraded from shared to exclusive without releasing the lock first but
 * shared access will be granted if the thread already has exclusive access.
 */
typedef struct _RTL_RESOURCE
{
    RTL_CRITICAL_SECTION CriticalSection;

    HANDLE SharedSemaphore;
    ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    ULONG NumberOfWaitingExclusive;

    LONG NumberOfActive;  // negative: exclusive acquire; zero: not acquired; positive: shared acquire(s)
    HANDLE ExclusiveOwnerThread;

    ULONG Flags;  // RTL_RESOURCE_FLAG_*

    PVOID DebugInfo;  // Type replaced with PVOID, since this field is unused in ntp
} RTL_RESOURCE, *PRTL_RESOURCE;


/**
 * @brief Initializes a fat read/write lock structure.
 * 
 * @param pResource A pointer to the caller allocated structure to initialize
 */
VOID NTAPI RtlInitializeResource(PRTL_RESOURCE pResource);


/**
 * @brief Deallocates and frees the contents of a fat read/write lock.
 *
 * @param pResource A pointer to the resource object to delete
 */
VOID NTAPI RtlDeleteResource(PRTL_RESOURCE pResource);


/**
 * @brief Acquires a fat read/write for shared access, optionally waiting until access can be granted.
 * 
 * @param pResource A pointer to the resource object to acquire for shared access
 * @param bWait Boolean value specifying whether the function can wait for access to be granted
 */
BOOLEAN NTAPI RtlAcquireResourceShared(PRTL_RESOURCE pResource, BOOLEAN bWait);


/**
 * @brief Acquires a fat read/write for exclusive access, optionally waiting until access can be granted.
 * 
 * @param pResource A pointer to the resource object to exclusively acquire
 * @param bWait Boolean value specifying whether the function can wait for access to be granted
 */
BOOLEAN NTAPI RtlAcquireResourceExclusive(PRTL_RESOURCE pResource, BOOLEAN bWait);


/**
 * @brief Releases a reference to the lock made by the RtlAcquireResource functions.
 *
 * @param pResource A pointer to the resource to release
 */
VOID NTAPI RtlReleaseResource(PRTL_RESOURCE pResource);

#if defined(__cplusplus)
}
#endif  // __cplusplus
