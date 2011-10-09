#ifndef ATP_LOG_LEVELS_H_
#define ATP_LOG_LEVELS_H_

#include <glog/logging.h>

// Verbose level.  Use flag --v=N where N >= VLOG_LEVEL_* to see.

#define VLOG_LEVEL_IBAPI_APPLICATION 1

#define VLOG_LEVEL_ECLIENT  1
#define VLOG_LEVEL_EWRAPPER 1

#define VLOG_LEVEL_ASIO_ECLIENT_SOCKET 10
#define VLOG_LEVEL_ASIO_ECLIENT_SOCKET_DEBUG 30

#define VLOG_LEVEL_IBAPI_SOCKET_INITIATOR 5
#define VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR 5
#define VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR_STRATEGY 1

#define VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR_IMPL 20

#define VLOG_LEVEL_ZMQ_MEM_FREE 40
#define VLOG_LEVEL_ZMQ_RESPONDER 20
#define VLOG_LEVEL_ZMQ_MESSAGE 20


// LogReader
#define LOG_READER_LOGGER VLOG(10)
#define LOG_READER_DEBUG VLOG(20)


#endif //ATP_LOG_LEVELS_H_
