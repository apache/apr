/*------------------------------------------------------------------
  These functions are to be called when the shared NLM starts and
  stops.  By using these functions instead of defining a main()
  and calling ExitThread(TSR_THREAD, 0), the load time of the
  shared NLM is faster and memory size reduced.
   
  You may also want to override these in your own Apache module
  to do any cleanup other than the mechanism Apache modules
  provide.
------------------------------------------------------------------*/
#include <netware.h>
#include <library.h>
#include <nks/synch.h>
#include "novsock2.h"

#include "apr_pools.h"
#include "apr_private.h"


/* library-private data...*/
int          gLibId = -1;
void         *gLibHandle = (void *) NULL;
NXMutex_t    *gLibLock = (NXMutex_t *) NULL;

/* internal library function prototypes...*/
int DisposeLibraryData(void *);

int _NonAppStart
(
    void        *NLMHandle,
    void        *errorScreen,
    const char  *cmdLine,
    const char  *loadDirPath,
    size_t      uninitializedDataLength,
    void        *NLMFileHandle,
    int         (*readRoutineP)( int conn, void *fileHandle, size_t offset,
                    size_t nbytes, size_t *bytesRead, void *buffer ),
    size_t      customDataOffset,
    size_t      customDataSize,
    int         messageCount,
    const char  **messages
)
{
    NX_LOCK_INFO_ALLOC(liblock, "Per-Application Data Lock", 0);

#pragma unused(cmdLine)
#pragma unused(loadDirPath)
#pragma unused(uninitializedDataLength)
#pragma unused(NLMFileHandle)
#pragma unused(readRoutineP)
#pragma unused(customDataOffset)
#pragma unused(customDataSize)
#pragma unused(messageCount)
#pragma unused(messages)

    WSADATA wsaData;
    apr_status_t status;
    
    gLibId = register_library(DisposeLibraryData);

    if (gLibId < -1)
    {
        OutputToScreen(errorScreen, "Unable to register library with kernel.\n");
        return -1;
    }

    gLibHandle = NLMHandle;

    gLibLock = NXMutexAlloc(0, 0, &liblock);

    if (!gLibLock)
    {
        OutputToScreen(errorScreen, "Unable to allocate library data lock.\n");
        return -1;
    }

    apr_netware_setup_time();

    if ((status = apr_pool_initialize()) != APR_SUCCESS)
        return status;

    return WSAStartup((WORD) MAKEWORD(2, 0), &wsaData);
}

void _NonAppStop( void )
{
    apr_pool_terminate();

    WSACleanup();

    unregister_library(gLibId);
    NXMutexFree(gLibLock);
}

int  _NonAppCheckUnload( void )
{
    return 0;
}

int register_NLM(void *NLMHandle)
{
    APP_DATA *app_data = (APP_DATA*) get_app_data(gLibId);

    NXLock(gLibLock);
    if (!app_data) {
        app_data = (APP_DATA*)library_malloc(gLibHandle, sizeof(APP_DATA));

        if (app_data) {
            memset (app_data, 0, sizeof(APP_DATA));
            set_app_data(gLibId, app_data);
        }
    }

    if (app_data && (!app_data->initialized)) {
        app_data->initialized = 1;
        NXUnlock(gLibLock);
        return 0;
    }

    NXUnlock(gLibLock);
    return 1;
}

int unregister_NLM(void *NLMHandle)
{
    APP_DATA *app_data = (APP_DATA*) get_app_data(gLibId);

    NXLock(gLibLock);
    if (app_data) {
        app_data->initialized = 0;
        NXUnlock(gLibLock);
        return 0;
    }
    NXUnlock(gLibLock);
    return 1;
}

int DisposeLibraryData(void *data)
{
    if (data)
    {
        library_free(data);
    }

    return 0;
}

int setGlobalPool(void *data)
{
    APP_DATA *app_data = (APP_DATA*) get_app_data(gLibId);

    NXLock(gLibLock);

    if (app_data && !app_data->gPool) {
        app_data->gPool = data;
    }

    NXUnlock(gLibLock);
    return 1;
}

void* getGlobalPool()
{
    APP_DATA *app_data = (APP_DATA*) get_app_data(gLibId);

    if (app_data) {
        return app_data->gPool;
    }

    return NULL;
}

