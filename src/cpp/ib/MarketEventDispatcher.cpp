
#include <sstream>
#include "common.hpp"
#include "log_levels.h"

#include "ib/TickerMap.hpp"
#include "ib/tick_types.hpp"

#include "proto/common.hpp"
#include "proto/ib.pb.h"
#include "historian/constants.hpp"

#include "varz/varz.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ib/MarketEventDispatcher.hpp"


DEFINE_VARZ_int64(mk_event_dispatch_publish_total_bytes, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_count, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_last_ts, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_interval_micros, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_serialization_errors, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_unresolved_keys, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_micros, 0, "");


DEFINE_VARZ_int64(mk_event_dispatch_publish_depth_total_bytes, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_depth_count, 0, "");
DEFINE_VARZ_int64(mk_event_dispatch_publish_depth_unresolved_keys, 0, "");

namespace ib {
namespace internal {

using proto::ib::MarketData;
using proto::ib::MarketDepth;


MarketEventDispatcher::MarketEventDispatcher(
    IBAPI::Application& app,
    const IBAPI::SessionID& sessionId) :
    IBAPI::ApiEventDispatcher(app, sessionId)
{
  VARZ_mk_event_dispatch_publish_last_ts = now_micros();
}

MarketEventDispatcher::~MarketEventDispatcher() {}


void MarketEventDispatcher::onPublish(boost::uint64_t start, size_t sent)
{
  VARZ_mk_event_dispatch_publish_count++;
  VARZ_mk_event_dispatch_publish_total_bytes += sent;
  VARZ_mk_event_dispatch_publish_interval_micros =
      start - VARZ_mk_event_dispatch_publish_last_ts;
  VARZ_mk_event_dispatch_publish_last_ts = start;
}

void MarketEventDispatcher::onSerializeError()
{
  VARZ_mk_event_dispatch_publish_serialization_errors++;
}

void MarketEventDispatcher::onUnresolvedTopic()
{
    VARZ_mk_event_dispatch_publish_unresolved_keys++;
}

void MarketEventDispatcher::onCompletedPublishRequest(boost::uint64_t start)
{
  VARZ_mk_event_dispatch_publish_micros = now_micros() - start;
}

void MarketEventDispatcher::publishDepth(TickerId tickerId,
                                         int side, int level, int operation,
                                         double price, int size,
                                         TimeTracking& timed,
                                         const std::string& mm)
{
  boost::uint64_t now = now_micros();

  std::string topic;
  if (TickerMap::getSubscriptionKeyFromId(tickerId, &topic)) {

    std::ostringstream zmq_topic;
    zmq_topic << historian::ENTITY_IB_MARKET_DEPTH << ':' << topic;

    MarketDepth ibMarketDepth;
    ibMarketDepth.set_timestamp(timed.getMicros());
    ibMarketDepth.set_symbol(topic);
    ibMarketDepth.set_price(price);
    ibMarketDepth.set_size(size);
    ibMarketDepth.set_level(level);
    ibMarketDepth.set_mm(mm);
    ibMarketDepth.set_contract_id(tickerId);
    using namespace proto::ib;
    switch (side) {

      case 0: ibMarketDepth.set_side(MarketDepth_Side_ASK);
        break;
      case 1: ibMarketDepth.set_side(MarketDepth_Side_BID);
        break;

    }
    switch (operation) {

      case 0: ibMarketDepth.set_operation(MarketDepth_Operation_INSERT);
        break;
      case 1: ibMarketDepth.set_operation(MarketDepth_Operation_UPDATE);
        break;
      case 2: ibMarketDepth.set_operation(MarketDepth_Operation_DELETE);
        break;

    }

    // frames
    // 1. topic
    // 2. protobuff

    std::string proto;
    if (ibMarketDepth.SerializeToString(&proto)) {

      zmq::socket_t* socket = getOutboundSocket(0);
      size_t sent = atp::zmq::send_copy(*socket, zmq_topic.str(), true);
      sent += atp::zmq::send_copy(*socket, proto, false);

      VARZ_mk_event_dispatch_publish_depth_count++;
      VARZ_mk_event_dispatch_publish_depth_total_bytes += sent;
      VARZ_mk_event_dispatch_publish_interval_micros =
          now - VARZ_mk_event_dispatch_publish_last_ts;
      VARZ_mk_event_dispatch_publish_last_ts = now;

    } else {

      LOG(ERROR) << "Unable to serialize: " << timed.getMicros()
                 << zmq_topic.str();

      VARZ_mk_event_dispatch_publish_serialization_errors++;

    }

  } else {

    LOG(ERROR) << "Cannot get subscription key / topic for " << tickerId
               << ", market depth";

    VARZ_mk_event_dispatch_publish_depth_unresolved_keys++;

  }

  VARZ_mk_event_dispatch_publish_micros = now_micros() - now;
}


} // internal
} // ib

