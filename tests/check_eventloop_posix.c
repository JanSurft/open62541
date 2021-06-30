/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>

#include <check.h>
#include <time.h>

#include "eventloop_posix.h"
#include "testing_clock.h"

// #define N_EVENTS 10000

UA_EventLoop *el;
// size_t count = 0;

// static void
// timerCallback(void *application, void *data) {
//     count++;
// }

// /* Create empty events with different callback intervals */
// static void
// createEvents(UA_UInt32 events) {
//     for(size_t i = 0; i < events; i++) {
//         UA_Double interval = (UA_Double)i+1;
//         UA_StatusCode retval =
//             UA_EventLoop_addCyclicCallback(el, timerCallback, NULL, NULL, interval, NULL,
//                                            UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME, NULL);
//         ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
//     }
// }
typedef struct FDContext {
    int fd;
} FDContext;

static void
Generic_iterationCallback(UA_EventSource *es, UA_FD fd, void *fdcontext, short event) {
    FDContext *ctx = (FDContext*) fdcontext;
    ck_assert_int_eq(ctx->fd, fd);
}
static void
Generic_connectionSocketCallback(UA_ConnectionManager *cm, UA_FD fd,
                             void **fdcontext, short event) {

    FDContext *ctx = (FDContext*) *fdcontext;
    ck_assert_int_eq(ctx->fd, fd);
}

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode stat,
                   UA_ByteString msg) {

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER,
                 "connection callback for id: %lu", connectionId);
}



START_TEST(registerFDSimple) {
    el = UA_EventLoop_new(UA_Log_Stdout);

    UA_UInt16 port = 4840;
    UA_Variant portVar;
    UA_Variant_setScalar(&portVar, &port, &UA_TYPES[UA_TYPES_UINT16]);

    UA_ConnectionManager *cm = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));
    cm->connectionCallback = connectionCallback;
    UA_ConfigParameter_setParameter(&cm->eventSource.parameters, "listen-port", &portVar);
    UA_EventLoop_registerEventSource(el, &cm->eventSource);

    FDContext fd0 = {0};
    FDContext fd1 = {1};

    UA_EventLoop_registerFD(el, 0, 0, (UA_FDCallback) Generic_connectionSocketCallback, &cm->eventSource, &fd0);
    UA_EventLoop_registerFD(el, 1, 0, (UA_FDCallback) Generic_connectionSocketCallback, &cm->eventSource, &fd1);

    UA_EventLoop_iterateFD(el, &cm->eventSource,(UA_FDCallback)Generic_iterationCallback);


        // createEvents(N_EVENTS);

    // clock_t begin = clock();
    // for(size_t i = 0; i < 1000; i++) {
    //     UA_DateTime next = UA_EventLoop_run(el, 1);
    //     UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    // }

    // clock_t finish = clock();
    // double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    // printf("duration was %f s\n", time_spent);
    // printf("%lu callbacks\n", (unsigned long)count);

    // UA_EventLoop_stop(el);
    // UA_EventLoop_delete(el);
    // el = NULL;
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, registerFDSimple);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
