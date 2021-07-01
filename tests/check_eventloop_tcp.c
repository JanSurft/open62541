/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>

#include "testing_clock.h"
#include <check.h>

#define N_EVENTS 10000

UA_EventLoop *el;

START_TEST(listenTCP) {
    el = UA_EventLoop_new(UA_Log_Stdout);

    UA_UInt16 port = 4840;
    UA_Variant portVar;
    UA_Variant_setScalar(&portVar, &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_ConnectionManager *cm = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));
    UA_ConfigParameter_setParameter(&cm->eventSource.parameters, "listen-port", &portVar);
    UA_EventLoop_registerEventSource(el, &cm->eventSource);

    UA_EventLoop_start(el);

    for(size_t i = 0; i < 10; i++) {
        UA_DateTime next = UA_EventLoop_run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }

    UA_EventLoop_stop(el);
    while(UA_EventLoop_getState(el) != UA_EVENTLOOPSTATE_STOPPED) {
        UA_DateTime next = UA_EventLoop_run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    UA_EventLoop_delete(el);
    el = NULL;
} END_TEST

static unsigned connCount;
static char *testMsg = "open62541";
static uintptr_t clientId;
static UA_Boolean received;

typedef struct {
    bool isInitial;
} ConnectionContext;

typedef struct {
    ConnectionContext base;
    uintptr_t fd;
} FDConnectionContext;

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode status,
                   UA_ByteString msg) {
    if(*connectionContext != NULL)
        clientId = connectionId;
    if(msg.length == 0 && status == UA_STATUSCODE_GOOD)
        connCount++;
    if(status != UA_STATUSCODE_GOOD)
        connCount--;
    if(msg.length > 0) {
        UA_ByteString rcv = UA_BYTESTRING(testMsg);
        ck_assert(UA_String_equal(&msg, &rcv));
        received = true;
    }
}

static void
connectionContextCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode status,
                   UA_ByteString msg) {

    if (*connectionContext == NULL) {
        clientId = connectionId;
        return;
    }

    ConnectionContext *ctx = (ConnectionContext*) *connectionContext;

    if (ctx->isInitial) {
        FDConnectionContext *fdCtx = (FDConnectionContext*) calloc(1, sizeof(FDConnectionContext));
        fdCtx->base.isInitial = 0;
        fdCtx->fd = connectionId;
        *connectionContext = fdCtx;
    } else {
        FDConnectionContext *fdCtx = (FDConnectionContext*) ctx;
        ck_assert_int_eq(fdCtx->fd, connectionId);
    }
}

START_TEST(connectTCPContextTest) {
        el = UA_EventLoop_new(UA_Log_Stdout);

        UA_UInt16 port = 4840;
        UA_Variant portVar;
        UA_Variant_setScalar(&portVar, &port, &UA_TYPES[UA_TYPES_UINT16]);
        UA_ConnectionManager *cm = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));

        ConnectionContext initialContext = {true};
        cm->connectionCallback = connectionContextCallback;
        cm->initialConnectionContext = &initialContext;
        UA_ConfigParameter_setParameter(&cm->eventSource.parameters, "listen-port", &portVar);
        UA_EventLoop_registerEventSource(el, &cm->eventSource);

        connCount = 0;
        UA_EventLoop_start(el);

        /* Open a client connection */
        clientId = 0;
        UA_StatusCode retval = cm->openConnection(cm, UA_STRING("localhost:4840"), NULL);

        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        for(size_t i = 0; i < 10; i++) {
            UA_DateTime next = UA_EventLoop_run(el, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        }
        ck_assert(clientId != 0);

        /* Send a message from the client */
        received = false;
        UA_ByteString snd;
        retval = cm->allocNetworkBuffer(cm, clientId, &snd, strlen(testMsg));
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        memcpy(snd.data, testMsg, strlen(testMsg));
        retval = cm->sendWithConnection(cm, clientId, &snd);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        for(size_t i = 0; i < 10; i++) {
            UA_DateTime next = UA_EventLoop_run(el, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        }
        ck_assert(received);

        /* Close the connection */
        retval = cm->closeConnection(cm, clientId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        for(size_t i = 0; i < 10; i++) {
            UA_DateTime next = UA_EventLoop_run(el, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        }
        ck_assert_uint_eq(connCount, 0);

        /* Stop the EventLoop */
        UA_EventLoop_stop(el);
        for(size_t i = 0; i < 10; i++) {
            UA_DateTime next = UA_EventLoop_run(el, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        }
        UA_EventLoop_delete(el);
        el = NULL;
    } END_TEST

START_TEST(connectTCP) {
    el = UA_EventLoop_new(UA_Log_Stdout);

    UA_UInt16 port = 4840;
    UA_Variant portVar;
    UA_Variant_setScalar(&portVar, &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_ConnectionManager *cm = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));
    cm->connectionCallback = connectionCallback;
    UA_ConfigParameter_setParameter(&cm->eventSource.parameters, "listen-port", &portVar);
    UA_EventLoop_registerEventSource(el, &cm->eventSource);

    connCount = 0;
    UA_EventLoop_start(el);

    /* Open a client connection */
    clientId = 0;
    UA_StatusCode retval = cm->openConnection(cm, UA_STRING("localhost:4840"), (void*)0x01);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 10; i++) {
        UA_DateTime next = UA_EventLoop_run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert(clientId != 0);
    ck_assert_uint_eq(connCount, 2);

    /* Send a message from the client */
    received = false;
    UA_ByteString snd;
    retval = cm->allocNetworkBuffer(cm, clientId, &snd, strlen(testMsg));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    memcpy(snd.data, testMsg, strlen(testMsg));
    retval = cm->sendWithConnection(cm, clientId, &snd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 10; i++) {
        UA_DateTime next = UA_EventLoop_run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert(received);

    /* Close the connection */
    retval = cm->closeConnection(cm, clientId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(connCount, 2);
    for(size_t i = 0; i < 10; i++) {
        UA_DateTime next = UA_EventLoop_run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert_uint_eq(connCount, 0);

    /* Stop the EventLoop */
    UA_EventLoop_stop(el);
    while(UA_EventLoop_getState(el) != UA_EVENTLOOPSTATE_STOPPED) {
        UA_DateTime next = UA_EventLoop_run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    UA_EventLoop_delete(el);
    el = NULL;
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test TCP EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, listenTCP);
    tcase_add_test(tc, connectTCP);
    tcase_add_test(tc, connectTCPContextTest);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
