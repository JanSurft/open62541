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
} UA_ConnectionContext;

/* In this example, we integrate the server into an external "mainloop". This
   can be for example the event-loop used in GUI toolkits, such as Qt or GTK. */

UA_Connection connection;
UA_Server *SERVER = NULL;

static UA_StatusCode *UA_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                               UA_ByteString *buf) {

}


static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode stat,
                   UA_ByteString msg) {

#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(msg.data, msg.length);
#endif

    // UA_ConnectionContext *ctx = (UA_ConnectionContext *) *connectionContext;

    // UA_StatusCode rv = UA_STATUSCODE_GOOD;

    // if (!ctx->isInitialized) {

    //     ctx->connectionId = connectionId;
    //     ctx->layer = &ctx->server->config.networkLayers[0];

    //     /* Add a SecureChannel to a new connection */
    //     if(!ctx->channel) {
    //         rv = UA_Server_createSecureChannel(ctx->server, connection);
    //         if(rv != UA_STATUSCODE_GOOD)
    //             goto error;

    //         channel = connection->channel;
    //         UA_assert(channel);
    //     }


    //     /* Open a client connection */
    //     // uintptr_t serverId = connectionId;
    //     // UA_StatusCode retval = cm->openConnection(cm, UA_STRING("localhost:4840"), (void*)0x01);
    //     // UA_ASSERT_STATUS(retval);

    //     // for(size_t i = 0; i < 10; i++) {
    //     //     UA_DateTime next = UA_EventLoop_run(el, 1);
    //     //     UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    //     // }

    //     /* Send a message from the client */
    //     // received = false;
    //     // UA_ByteString snd;
    //     // retval = cm->allocNetworkBuffer(cm, serverId, &snd, strlen(testMsg));

    //     // memcpy(snd.data, testMsg, strlen(testMsg));
    //     // retval = cm->sendWithConnection(cm, clientId, &snd);

    //     // *connectionContext = &connection;
    // }



    if (msg.length > 0) {
        UA_Server_processBinaryMessage(SERVER, &connection, &msg);
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

    UA_ConnectionContext *ctx = UA_malloc(sizeof(UA_ConnectionContext));
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

    SERVER = UA_Server_newWithConfig(conf);
    UA_Server *server = SERVER;

    UA_Server_setup(server);

    /* Should the server networklayer block (with a timeout) until a message
       arrives or should it return immediately? */
    UA_Boolean waitInternal = false;

    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
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
        UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);
    }
    retval = UA_Server_run_shutdown(server);

 cleanup:
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
