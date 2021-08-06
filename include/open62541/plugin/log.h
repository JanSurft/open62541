/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_LOG_H_
#define UA_PLUGIN_LOG_H_

#include <open62541/config.h>

#include <stdarg.h>

_UA_BEGIN_DECLS

/**
 * Logging Plugin API
 * ==================
 *
 * Servers and clients define a logger in their configuration. The logger is a
 * plugin. A default plugin that logs to ``stdout`` is provided as an example.
 * The logger plugin is stateful and can point to custom data. So it is possible
 * to keep open file handlers in the logger context.
 *
 * Every log message consists of a log level, a log category and a string
 * message content. The timestamp of the log message is created within the
 * logger. */

typedef enum {
    UA_LOGLEVEL_TRACE = 0,
    UA_LOGLEVEL_DEBUG,
    UA_LOGLEVEL_INFO,
    UA_LOGLEVEL_WARNING,
    UA_LOGLEVEL_ERROR,
    UA_LOGLEVEL_FATAL
} UA_LogLevel;

typedef enum {
    UA_LOGCATEGORY_NETWORK = 0,
    UA_LOGCATEGORY_SECURECHANNEL,
    UA_LOGCATEGORY_SESSION,
    UA_LOGCATEGORY_SERVER,
    UA_LOGCATEGORY_CLIENT,
    UA_LOGCATEGORY_USERLAND,
    UA_LOGCATEGORY_SECURITYPOLICY
} UA_LogCategory;

typedef struct {
    /* Log a message. The message string and following varargs are formatted
     * according to the rules of the printf command. Use the convenience macros
     * below that take the minimum log level defined in ua_config.h into
     * account. */
    void (*log)(void *logContext, UA_LogLevel level, UA_LogCategory category,
                const char *msg, va_list args);

    void *context; /* Logger state */

    void (*clear)(void *context); /* Clean up the logger plugin */
} UA_Logger;

#define UA_MACRO_EXPAND(x) x

#ifdef UA_DEBUG_FILE_LINE_INFO


static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_TRACE_FUN(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 100
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_TRACE, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_DEBUG_FUN(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 200
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_DEBUG, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_INFO_FUN(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 300
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_INFO, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_WARNING_FUN(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 400
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_WARNING, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_ERROR_FUN(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 500
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_ERROR, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_FATAL_FUN(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 600
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_FATAL, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}
#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define UA_LOG_INTERNAL(LOG_FUN, LOGGER, CAT, MSG, ...)           \
    UA_MACRO_EXPAND(                                                                     \
        if (errno == 0) { LOG_FUN(LOGGER, CAT, "" MSG "%s (%s:%d)", __VA_ARGS__, __FILE__, __LINE__   \
                        );} else {               \
        LOG_FUN(LOGGER, CAT, "" MSG "%s (%s:%d: errno: %s)", __VA_ARGS__, __FILE__, __LINE__, clean_errno()   \
                        ); errno=0;})

#define UA_LOG(FLAVOUR, LOGGER, CAT, ...) \
    UA_MACRO_EXPAND(UA_LOG_INTERNAL(UA_LOG_##FLAVOUR##_FUN,    \
                                          LOGGER, CAT, __VA_ARGS__, ""))

#define UA_LOG_TRACE(LOGGER, CAT, ...) \
    UA_MACRO_EXPAND(UA_LOG(TRACE, LOGGER, CAT, __VA_ARGS__))
#define UA_LOG_DEBUG(LOGGER, CAT, ...) \
    UA_MACRO_EXPAND(UA_LOG(DEBUG, LOGGER, CAT, __VA_ARGS__))
#define UA_LOG_INFO(LOGGER, CAT, ...) \
    UA_MACRO_EXPAND(UA_LOG(INFO, LOGGER, CAT, __VA_ARGS__))
#define UA_LOG_WARNING(LOGGER, CAT, ...) \
    UA_MACRO_EXPAND(UA_LOG(WARNING, LOGGER, CAT, __VA_ARGS__))
#define UA_LOG_ERROR(LOGGER, CAT, ...) \
    UA_MACRO_EXPAND(UA_LOG(ERROR, LOGGER, CAT, __VA_ARGS__))
#define UA_LOG_FATAL(LOGGER, CAT, ...) \
    UA_MACRO_EXPAND(UA_LOG(FATAL, LOGGER, CAT, __VA_ARGS__))


#else
static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_TRACE(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 100
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_TRACE, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_DEBUG(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 200
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_DEBUG, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_INFO(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 300
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_INFO, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_WARNING(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 400
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_WARNING, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_ERROR(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 500
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_ERROR, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_FATAL(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 600
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_FATAL, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}
#endif
_UA_END_DECLS

#endif /* UA_PLUGIN_LOG_H_ */
