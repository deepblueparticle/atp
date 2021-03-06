
#include "ib/SocketConnectorBase.hpp"
#include "ib/ZmqMessage.hpp"

#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"


namespace IBAPI {

using ib::internal::SocketConnectorBase;

class SocketConnector::implementation : public SocketConnectorBase
{
 public:
  implementation(const ZmqAddress& reactorAddress,
                 const ZmqAddressMap& outboundChannels,
                 Application& app, int timeout,
                 zmq::context_t* inboundContext = NULL,
                 zmq::context_t* outboundContext = NULL) :
      SocketConnectorBase(app, timeout,
                          reactorAddress,
#ifdef SOCKET_CONNECTOR_USES_BLOCKING_REACTOR
                          ZMQ_REP,
#else
                          ZMQ_PULL,
#endif
                          outboundChannels,
                          inboundContext,
                          outboundContext)
  {
  }

  ~implementation()
  {
  }

  friend class IBAPI::SocketConnector;
};

SocketConnector::SocketConnector(const ZmqAddress& reactorAddress,
                                 const ZmqAddressMap& outboundChannels,
                                 Application& app, int timeout,
                                 ::zmq::context_t* inboundContext,
                                 ::zmq::context_t* outboundContext) :
    impl_(new SocketConnector::implementation(reactorAddress, outboundChannels,
                                              app, timeout,
                                              inboundContext,
                                              outboundContext))
{
  IBAPI_SOCKET_CONNECTOR_LOGGER << "SocketConnector started.";
  impl_->socketConnector_ = this;
}

SocketConnector::~SocketConnector()
{
}


/// QuickFIX implementation returns the socket fd.  In our case, we simply
/// return the clientId.
int SocketConnector::connect(const string& host,
                             unsigned int port,
                             unsigned int clientId,
                             Strategy* strategy,
                             int maxAttempts)
{
  return impl_->connect(host, port, clientId, strategy, maxAttempts);
}

bool SocketConnector::start(unsigned int clientId)
{
  IBAPI_SOCKET_CONNECTOR_LOGGER << "Starting inbound reactor.";
  impl_->start(clientId);
  return true;
}

bool SocketConnector::stop()
{
  IBAPI_SOCKET_CONNECTOR_LOGGER << "STOP";
  // By default we don't wait for the reactor to stop.  This is because
  // reactor will keep running in a loop and to properly stop it, we'd need
  // a protocol to tell the reactor to exit the running loop.  This isn't
  // worth the extra complexity.  Instead, just let the code continue to
  // exit and eventually the zmq context will be destroyed and force the
  // reactor loop to exit on exception.
  return impl_->stop(false);
}

void SocketConnector::SkipConnection()
{
  impl_->SkipConnection();
}

} // namespace IBAPI

