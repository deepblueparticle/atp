
// api964
#include "ib/ReactorStrategyFactory.hpp"
#include "ApiMessages.hpp"


namespace ib {
namespace internal {

using ib::internal::ReactorStrategy;
using ib::internal::ReactorStrategyFactory;

using IBAPI::V964::MarketDataRequest;

class ReactorStrategyImpl : public ReactorStrategy
{
  virtual bool handleInboundMessage(Message& message,
                                    EClientPtr eclient)
  {
    FIX::MsgType msgType;
    message.getHeader().get(msgType);
    if (msgType.getString() == MarketDataRequest::MESSAGE_TYPE) {
      MarketDataRequest copy(message);
      return copy.callApi(eclient);
    }

    return false;
  }
};

ReactorStrategy* ReactorStrategyFactory::getInstance()
{
  return new ReactorStrategyImpl();
};

} // internal
} // ib

