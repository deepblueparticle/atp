
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"
#include "zmq/Reactor.hpp"

namespace atp {
namespace zmq {

Reactor::Reactor(const int socket_type,
                 const string& addr,
                 Reactor::Strategy& strategy,
                 ::zmq::context_t* context) :
    socket_type_(socket_type),
    addr_(addr),
    strategy_(strategy),
    context_(context),
    ready_(false)
{
  // start thread
  thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
      boost::bind(&Reactor::process, this)));

  // Wait here for the connection to be ready.
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_) {
    isReady_.wait(lock);
  }

  LOG(INFO) << "Reactor of socket type (" << socket_type_
            << ") is ready: " << addr_;
}

Reactor::~Reactor()
{
}

const std::string& Reactor::addr()
{
  return addr_;
}

void Reactor::block()
{
  thread_->join();
}

void Reactor::process()
{
  bool localContext = false;
  if (context_ == NULL) {
    // Create own context
    context_ = new ::zmq::context_t(1);
    localContext = true;
    ZMQ_REACTOR_LOGGER << "Created local context.";
  } else {
    ZMQ_REACTOR_LOGGER << "Using shared context: " << context_;
  }

  switch (socket_type_) {
    case ZMQ_PULL : ZMQ_REACTOR_LOGGER << "ZMQ_PULL/" << ZMQ_PULL;
      break;
    case ZMQ_REP : ZMQ_REACTOR_LOGGER << "ZMQ_REP/" << ZMQ_REP;
      break;
    default : LOG(FATAL) << "NOT SUPPORTED";
  }

  ::zmq::socket_t socket(*context_, socket_type_);

  try {
    socket.bind(addr_.c_str());
    ZMQ_REACTOR_LOGGER << "listening @ " << addr_;
  } catch (::zmq::error_t e) {
    LOG(FATAL) << "Cannot bind " << addr_ << ":" << e.what();
  }

  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    ready_ = true;
  }
  isReady_.notify_all();

  try {
    while (strategy_.respond(socket)) {
      // just loop
    }
  } catch (::zmq::error_t e) {
    LOG(ERROR) << "Exception while processing messages: " << e.what()
               << ", stopping.";
  }

  if (localContext) delete context_;
  LOG(ERROR) << "Reactor listening thread stopped.";
}


} // namespace zmq
} // namespace atp
