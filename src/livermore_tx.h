#ifndef LIVERMORE_TX_H
#define LIVERMORE_TX_H

#include <hj/os/dll.h>

// ---------------- error code [3000, 3999] -----------------
#ifndef OK
#define OK 0
#endif

#define TX_ERR_INVALID_PARAM 3000
#define TX_ERR_SUBSCRIBE_FAILED 3001
#define TX_ERR_UNSUBSCRIBE_FAILED 3002
#define TX_ERR_NOT_INITIALIZED 3003
#define TX_ERR_ALREADY_INITIALIZED 3004
#define TX_ERR_NOT_SUBSCRIBED 3005

// --------------- global define --------------------
typedef void (*err_handler)(int err_code, const char *err_msg);
typedef void (*md_handler)(void *md);

// --------------- data struct --------------------
typedef struct livermore_tx_param_version
{
    int major;
    int minor;
    int patch;
} livermore_tx_param_version;

typedef struct livermore_tx_param_init
{
    err_handler err_fn;
    md_handler  md_fn;
    int         result;
} livermore_tx_param_init;

typedef struct livermore_tx_param_quit
{
    int result;
} livermore_tx_param_quit;

typedef struct livermore_tx_param_subscribe
{
    const char *instrument;
    md_handler  md_fn;
    int         result;
} livermore_tx_param_subscribe;

typedef struct livermore_tx_param_unsubscribe
{
    const char *instrument;
    int         result;
} livermore_tx_param_unsubscribe;

typedef struct livermore_tx_param_query
{
    const char *instrument;
    void       *md;
    int         result;
} livermore_tx_param_query;

// --------------- API --------------------
C_STYLE_EXPORT void livermore_tx_version(sdk_context ctx);

C_STYLE_EXPORT void livermore_tx_init(sdk_context ctx);

C_STYLE_EXPORT void livermore_tx_quit(sdk_context ctx);

C_STYLE_EXPORT void livermore_tx_subscribe(sdk_context ctx);

C_STYLE_EXPORT void livermore_tx_unsubscribe(sdk_context ctx);

C_STYLE_EXPORT void livermore_tx_query(sdk_context ctx);
#endif // LIVERMORE_TX_H