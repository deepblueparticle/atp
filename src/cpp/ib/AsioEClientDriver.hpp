#ifndef IB_ASIO_ECLIENT_DRIVER_H_
#define IB_ASIO_ECLIENT_DRIVER_H_

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/internal.hpp"
#include "ib/ApiProtocolHandler.hpp"


using boost::asio::ip::tcp;

namespace ib {
namespace internal {


class AsioEClientDriver : public ApiSocket {

 public:

  class EventCallback
  {
   public:
    virtual void onEventThreadStart() = 0;
    virtual void onEventThreadStop() = 0;
    virtual void onSocketConnect(bool success) = 0;
    virtual void onSocketClose(bool success) = 0;
  };

  explicit AsioEClientDriver(boost::asio::io_service& ioService,
                             ApiProtocolHandler& protocolHandler,
                             EventCallback* callback = NULL);

  ~AsioEClientDriver();

  bool Connect(const char *host, unsigned int port, int clientId=0);

  void Disconnect();

  int Send(const char* buf, size_t sz);

  int Receive(char* buf, size_t sz);

  bool IsConnected() const;

  bool IsSocketOK() const;

  /// Returns clientId
  int getClientId();

  EClientPtr GetEClient();



  /// Event loop that checks messages on the socket
  void block();

  void reset();

 private:


  bool closeSocket();

  boost::asio::io_service& ioService_;
  ApiProtocolHandler& protocolHandler_;

  tcp::socket socket_;
  boost::mutex socketMutex_;

  // No ownership of this object.
  EventCallback* callback_;

  volatile bool socketOk_;

  enum State {  STARTING, RUNNING, STOPPING, STOPPED };
  volatile State state_;

  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;

  boost::condition_variable socketRunning_;
  int clientId_;

  int64 sendDt_;
  int64 receiveDt_;
  int64 processMessageDt_;
};

} // namespace internal
} // namespace ib


#endif //IB_ASIO_ECLIENT_DRIVER_H_
