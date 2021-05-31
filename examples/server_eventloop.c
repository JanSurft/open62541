/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <server/ua_server_internal.h>
#include <ua_util_internal.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

typedef struct {
    uintptr_t connectionId;
    UA_Boolean isInitialized;
    UA_Server *server;
    UA_SecureChannel *channel;
    UA_ServerNetworkLayer *layer;
    UA_Connection connection;
} UA_ConnectionContext;

/* In this example, we integrate the server into an external "mainloop". This
   can be for example the event-loop used in GUI toolkits, such as Qt or GTK. */

// UA_Connection connection;

static UA_StatusCode UA_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                               UA_ByteString *buf) {
    UA_ConnectionManager *cm = connection->cm;
    return cm->allocNetworkBuffer(cm, connection->connectionId, buf, length);
}

static UA_StatusCode UA_Connection_send(UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionManager *cm = connection->cm;
    return cm->sendWithConnection(cm, connection->connectionId, buf);
}

static
void UA_Connection_releaseBuffer (UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionManager *cm = connection->cm;
    cm->freeNetworkBuffer(cm, connection->connectionId, buf);
}

static void UA_Connection_close(UA_Connection *connection) {
    UA_ConnectionManager *cm = connection->cm;
    cm->closeConnection(cm, connection->connectionId);
}


// /* Release the send buffer manually */
// void UA_Connection_releaseSendBuffer(UA_Connection *connection, UA_ByteString *buf) {
//
// }


static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode stat,
                   UA_ByteString msg) {

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER,
                 "connection callback for id: %lu", connectionId);

    // if (stat != UA_STATUSCODE_GOOD) {
    //     UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER, "error");
    // }

    // UA_CHECK_STATUS_ERROR(
    //     stat,
    //     return,
    //     UA_EventLoop_getLogger(cm->eventSource.eventLoop),
    //     UA_LOGCATEGORY_SERVER,
    //     "error"
    //     )

// #ifdef UA_DEBUG_DUMP_PKGS
//     UA_dump_hex_pkg(msg.data, msg.length);
// #endif

    UA_ConnectionContext *ctx = (UA_ConnectionContext *) *connectionContext;

    UA_StatusCode rv = UA_STATUSCODE_GOOD;

    if (!ctx->isInitialized) {
        ctx->connectionId = connectionId;
        ctx->connection.connectionId = connectionId;
        ctx->connection.cm = cm;
        ctx->connection.close = UA_Connection_close;
        ctx->connection.free = NULL;
        ctx->connection.getSendBuffer = UA_Connection_getSendBuffer;
        ctx->connection.recv = NULL;
        ctx->connection.releaseRecvBuffer = UA_Connection_releaseBuffer;
        ctx->connection.releaseSendBuffer = UA_Connection_releaseBuffer;
        ctx->connection.send = UA_Connection_send;
        ctx->connection.state = UA_CONNECTIONSTATE_CLOSED;
        ctx->isInitialized = true;
    }

    if (ctx->connectionId != connectionId) {
        ctx->connectionId = connectionId;
        ctx->connection.connectionId = connectionId;
    }

    if (msg.length > 0) {
        UA_Server_processBinaryMessage(ctx->server, &ctx->connection, &msg);
    }


    // if(*connectionContext != NULL)
    //     clientId = connectionId;
    // if(msg.length == 0 && status == UA_STATUSCODE_GOOD)
    //     connCount++;
    // if(status != UA_STATUSCODE_GOOD)
    //     connCount--;
    // if(msg.length > 0) {
    //     UA_ByteString rcv = UA_BYTESTRING(testMsg);
    //     ck_assert(UA_String_equal(&msg, &rcv));
    //     received = true;
    // }
}

static void UA_Server_setup(UA_Server *server) {

    UA_ConnectionContext *ctx = (UA_ConnectionContext*) UA_malloc(sizeof(UA_ConnectionContext));
    memset(ctx, 0, sizeof(UA_ConnectionContext));

    ctx->server = server;

    UA_UInt16 port = 4840;
    UA_Variant portVar;
    UA_Variant_setScalar(&portVar, &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_ConnectionManager *cm = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));
    cm->connectionCallback = connectionCallback;
    cm->initialConnectionContext = ctx;
    UA_ConfigParameter_setParameter(&cm->eventSource.parameters, "listen-port", &portVar);

    UA_EventLoop_registerEventSource(UA_Server_getConfig(server)->eventLoop, (UA_EventSource *) cm);
}


int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *conf = (UA_ServerConfig*) UA_calloc(1, sizeof(UA_ServerConfig));
    UA_ServerConfig_setDefault(conf);

    // UA_Server *server = UA_Server_newWithConfig(conf);
    // UA_Server_setup(server);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(&server->config);
    UA_Server_setup(server);

    /* Should the server networklayer block (with a timeout) until a message
       arrives or should it return immediately? */
    UA_Boolean waitInternal = false;

    UA_StatusCode rv = UA_EventLoop_start(server->config.eventLoop);
    UA_CHECK_STATUS(rv, goto cleanup);

    rv = UA_Server_run_startup(server);
    if(rv != UA_STATUSCODE_GOOD)
        goto cleanup;

    // UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;

    // UA_EventLoop_start(el);

    // UA_EventLoop *el = UA_EventLoop_new(UA_Log_Stdout);


    // UA_EventLoop_start(el);

    while(running) {
        /* timeout is the maximum possible delay (in millisec) until the next
           _iterate call. Otherwise, the server might miss an internal timeout
           or cannot react to messages with the promised responsiveness. */
        /* If multicast discovery server is enabled, the timeout does not not consider new input data (requests) on the mDNS socket.
         * It will be handled on the next call, which may be too late for requesting clients.
         * if needed, the select with timeout on the multicast socket server->mdnsSocket (see example in mdnsd library)
         */

        UA_EventLoop_run(server->config.eventLoop, 1000000);
        // UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);
    }
    rv = UA_Server_run_shutdown(server);

 cleanup:
    UA_Server_delete(server);
    return rv == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
