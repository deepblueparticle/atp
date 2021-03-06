
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <ql/quantlib.hpp>

#include "ib/GenericTickRequest.hpp"
#include "ib/ApplicationBase.hpp"

#include "ib/AsioEClientDriver.hpp"
#include "ib/MarketEventDispatcher.hpp"
#include "ib/TestHarness.hpp"
#include "ib/TickerMap.hpp"


using namespace ib::internal;
using namespace IBAPI;
using namespace QuantLib;

/// How many seconds max to wait for certain data before timing out.
const int MAX_WAIT = 20;

static TickerMap tickerMap;

static std::string FormatOptionExpiry(int year, int month, int day)
{
  ostringstream s1;
  string fmt = (month > 9) ? "%4d%2d" : "%4d0%1d";
  string fmt2 = (day > 9) ? "%2d" : "0%1d";
  s1 << boost::format(fmt) % year % month << boost::format(fmt2) % day;
  return s1.str();
}

// Spin around until connection is made.
bool waitForConnection(AsioEClientDriver& ec, int attempts) {
  int tries = 0;
  for (int tries = 0; !ec.IsConnected() && tries < attempts; ++tries) {
    LOG(INFO) << "Waiting for connection setup." << std::endl;
    sleep(1);
  }
  return ec.IsConnected();
}

class TestApp : public IBAPI::ApplicationBase
{
 public:
  virtual IBAPI::ApiEventDispatcher*
  GetApiEventDispatcher(const IBAPI::SessionID& sessionId)
  {
    return new ib::internal::MarketEventDispatcher(*this, sessionId);
  }
};

class TestEWrapperEventCollector : public IBAPI::OutboundChannels
{
 public:
  TestEWrapperEventCollector() : context_(1), socket_(context_, ZMQ_PUB)
  {
    std::string endpoint = "tcp://127.0.0.1:6666";
    LOG(INFO) << "Creating publish socket @ " << endpoint << std::endl;
    socket_.bind(endpoint.c_str());
  }

  virtual zmq::socket_t* getOutboundSocket(unsigned int channel = 0)
  {
    return &socket_;
  }

 private:
  zmq::context_t context_;
  zmq::socket_t socket_;
};

/**
   Basic connection test.
 */
TEST(AsioEClientDriverTest, ConnectionTest)
{
  boost::asio::io_service ioService;

  TestApp app;

  IBAPI::ApiEventDispatcher* dispatcher = app.GetApiEventDispatcher(0);
  TestEWrapperEventCollector eventCollector;
  dispatcher->SetOutboundSockets(eventCollector);
  EWrapper* ew = dispatcher->GetEWrapper();

  TestHarness* th = dynamic_cast<TestHarness*>(ew);
  ApiProtocolHandler h(*ew);
  AsioEClientDriver ec(ioService, h);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.Connect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Disconnect
  ec.Disconnect();

  EXPECT_FALSE(ec.IsConnected());

  // Check invocation of the nextValidId -- this is part of the
  // TWS API protocol.
  EXPECT_EQ(1, th->getCount(NEXT_VALID_ID));
}


/**
   Request basic market data.
 */
TEST(AsioEClientDriverTest, RequestMarketDataTest)
{
  boost::asio::io_service ioService;
  TestApp app;


  IBAPI::ApiEventDispatcher* dispatcher = app.GetApiEventDispatcher(0);
  TestEWrapperEventCollector eventCollector;
  dispatcher->SetOutboundSockets(eventCollector);
  EWrapper* ew = dispatcher->GetEWrapper();

  TestHarness* th = dynamic_cast<TestHarness*>(ew);
  ApiProtocolHandler h(*ew);
  AsioEClientDriver ec(ioService, h);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.Connect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Request market data:
  Contract c;
  c.symbol = "AAPL";
  c.secType = "STK";
  c.exchange = "SMART";
  c.currency = "USD";

  TickerId tid = tickerMap.registerContract(c);
  EXPECT_GT(tid, 0);

  LOG(INFO) << "Requesting market data for " << c.symbol << " with id = "
            << tid << std::endl;

  GenericTickRequest genericTickRequest;
  genericTickRequest
      .add(RTVOLUME)
      .add(OPTION_VOLUME)
      .add(OPTION_IMPLIED_VOLATILITY)
      .add(HISTORICAL_VOLATILITY)
      .add(REALTIME_HISTORICAL_VOLATILITY)
      ;

  LOG(INFO) << "Request generic ticks: " << genericTickRequest.toString()
            << std::endl;

  bool snapShot = false;

  ec.GetEClient()->reqMktData(tid, c, genericTickRequest.toString(), snapShot);

  // Spin and wait for data.
  th->waitForFirstOccurrence(TICK_PRICE, MAX_WAIT);
  th->waitForFirstOccurrence(TICK_SIZE, MAX_WAIT);

  // Disconnect
  ec.Disconnect();

  EXPECT_FALSE(ec.IsConnected());

  EXPECT_GE(th->getCount(TICK_PRICE), 1);
  EXPECT_GE(th->getCount(TICK_SIZE), 1);
  EXPECT_TRUE(th->hasSeenTickerId(tid));
}

/**
   Request index
 */
TEST(AsioEClientDriverTest, RequestIndexMarketDataTest)
{
  boost::asio::io_service ioService;
  TestApp app;

  IBAPI::ApiEventDispatcher* dispatcher = app.GetApiEventDispatcher(0);
  TestEWrapperEventCollector eventCollector;
  dispatcher->SetOutboundSockets(eventCollector);
  EWrapper* ew = dispatcher->GetEWrapper();

  TestHarness* th = dynamic_cast<TestHarness*>(ew);
  ApiProtocolHandler h(*ew);
  AsioEClientDriver ec(ioService, h);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.Connect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Request market data:
  Contract c;
  c.symbol = "SPX";
  c.secType = "IND";
  c.exchange = "CBOE";
  c.primaryExchange = "CBOE";
  c.currency = "USD";

  TickerId tid = tickerMap.registerContract(c);
  EXPECT_GT(tid, 0);

  LOG(INFO) << "Requesting market data for " << c.symbol << " with id = "
            << tid << std::endl;

  GenericTickRequest genericTickRequest;
  genericTickRequest
      .add(RTVOLUME)
      .add(HISTORICAL_VOLATILITY)
      .add(REALTIME_HISTORICAL_VOLATILITY)
      ;


  LOG(INFO) << "Request generic ticks: "
            << genericTickRequest.toString() << std::endl;

  bool snapShot = false;
  ec.GetEClient()->reqMktData(tid, c, genericTickRequest.toString(), snapShot);

  // Spin and wait for data.
  th->waitForFirstOccurrence(TICK_PRICE, MAX_WAIT);
  th->waitForFirstOccurrence(TICK_SIZE, MAX_WAIT);

  // Disconnect
  ec.Disconnect();

  EXPECT_FALSE(ec.IsConnected());

  EXPECT_GE(th->getCount(TICK_PRICE), 1);
  EXPECT_GE(th->getCount(TICK_SIZE), 1);
  EXPECT_TRUE(th->hasSeenTickerId(tid));
}

/**
   Request market depth
 */
TEST(AsioEClientDriverTest, RequestMarketDepthTest)
{
  boost::asio::io_service ioService;
  TestApp app;

  IBAPI::ApiEventDispatcher* dispatcher = app.GetApiEventDispatcher(0);
  TestEWrapperEventCollector eventCollector;
  dispatcher->SetOutboundSockets(eventCollector);
  EWrapper* ew = dispatcher->GetEWrapper();

  TestHarness* th = dynamic_cast<TestHarness*>(ew);
  ApiProtocolHandler h(*ew);
  AsioEClientDriver ec(ioService, h);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.Connect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Request market data:
  Contract c;
  c.symbol = "AAPL";
  c.secType = "STK";
  c.exchange = "SMART";
  c.currency = "USD";

  TickerId tid = tickerMap.registerContract(c);
  EXPECT_GT(tid, 0);

  LOG(INFO) << "Requesting market depth for " << c.symbol << " with id = "
            << tid << std::endl;

  ec.GetEClient()->reqMktDepth(tid, c, 10);

  // Spin and wait for data.
  th->waitForFirstOccurrence(UPDATE_MKT_DEPTH, MAX_WAIT);

  // Disconnect
  ec.Disconnect();

  EXPECT_FALSE(ec.IsConnected());

  EXPECT_EQ(th->getCount(NEXT_VALID_ID), 1);
  EXPECT_GE(th->getCount(UPDATE_MKT_DEPTH), 1);
  EXPECT_TRUE(th->hasSeenTickerId(tid));
}


struct SortByStrike {
  bool operator()(const Contract& a, const Contract& b) {
    return a.strike < b.strike;
  }
} sortByStrikeFunctor;


/**
   Request contract information / option chain
 */
TEST(AsioEClientDriverTest, RequestOptionChainTest)
{
  boost::asio::io_service ioService;
  TestApp app;

  IBAPI::ApiEventDispatcher* dispatcher = app.GetApiEventDispatcher(0);
  TestEWrapperEventCollector eventCollector;
  dispatcher->SetOutboundSockets(eventCollector);
  EWrapper* ew = dispatcher->GetEWrapper();

  TestHarness* th = dynamic_cast<TestHarness*>(ew);
  ApiProtocolHandler h(*ew);
  AsioEClientDriver ec(ioService, h);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.Connect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Request market data:
  Contract c;
  c.symbol = "AAPL";
  c.secType = "OPT";
  c.exchange = "SMART";
  c.currency = "USD";

  // Determine the contract expiration == the friday from this date.
  using QuantLib::Date;
  Date today = Date::todaysDate();
  Date nextFriday = Date::nextWeekday(today, QuantLib::Friday);

  LOG(INFO) << "Expiration Year=" << nextFriday.year()
            << ", Month = " << nextFriday.month()
            << ", Day = " << nextFriday.dayOfMonth()
            << ", day of week = " << nextFriday.weekday()
            << std::endl;

  c.expiry = FormatOptionExpiry(nextFriday.year(),
                                nextFriday.month(),
                                nextFriday.dayOfMonth());
  c.right = "C"; // call - P for put

  int requestId = 1000; // Not a ticker id.  This is just a request id.

  LOG(INFO) << "Requesting option chain for "
            << c.symbol << " with request id = "
            << requestId << std::endl;

  // Collect the option chain
  std::vector<Contract> optionChain;
  th->setOptionChain(&optionChain);

  ec.GetEClient()->reqContractDetails(requestId, c);

  // Spin and wait for data.
  th->waitForFirstOccurrence(CONTRACT_DETAILS_END, MAX_WAIT);

  EXPECT_EQ(th->getCount(CONTRACT_DETAILS_END), 1);
  EXPECT_TRUE(th->hasSeenTickerId(requestId));

  // Check the option chain
  EXPECT_GT(optionChain.size(), 0);

  LOG(INFO) << "Received option chain." << std::endl;

  // Sort the option chain by strike
  std::sort(optionChain.begin(), optionChain.end(), sortByStrikeFunctor);

  GenericTickRequest genericTickRequest;

  // Iterate through the option chain and make market data request:
  std::vector<int> tids;

  // Just request a few
  int max = 5;
  int i = 0;
  for (std::vector<Contract>::iterator itr = optionChain.begin();
       itr != optionChain.end() && i < max;
       ++itr, ++i) {
    LOG(INFO) << "Requesting market data for contract / "
              << itr->localSymbol << std::endl;

    TickerId tid = tickerMap.registerContract(*itr);
    ec.GetEClient()->reqMktData(tid, *itr, genericTickRequest.toString(), false);
    tids.push_back(tid);
  }

  // Wait until all the data has come through for all the contracts.
  LOG(INFO) << "Waiting for data to come in.";

  for (std::vector<int>::iterator itr = tids.begin();
       itr != tids.end();
       ++itr) {
    for (int i = 0; i < MAX_WAIT && !th->hasSeenTickerId(*itr); ++i) {
      sleep(1);
    }
  }

  // Disconnect
  ec.Disconnect();
  EXPECT_FALSE(ec.IsConnected());

  EXPECT_GT(th->getCount(TICK_PRICE), 1);

  // Checks that we have seen the ticker ids for all the option contracts.
  for (std::vector<int>::iterator itr = tids.begin();
       itr != tids.end();
       ++itr) {
    EXPECT_TRUE(th->hasSeenTickerId(*itr));
  }
}


TickerId buildContract(Contract& c, const std::string& symbol)
{
  c.symbol = symbol;
  c.secType = "STK";
  c.exchange = "SMART";
  c.currency = "USD";
  TickerId tid = SymbolToTickerId(symbol);

  LOG(INFO) << "Requesting market data for " << c.symbol << " with id = "
            << tid << std::endl;
  return tickerMap.registerContract(c);
}


TEST(AsioEClientDriverTest, RequestMarketDataLoadTest)
{
  boost::asio::io_service ioService;
  TestApp app;

  IBAPI::ApiEventDispatcher* dispatcher = app.GetApiEventDispatcher(0);
  TestEWrapperEventCollector eventCollector;
  dispatcher->SetOutboundSockets(eventCollector);
  EWrapper* ew = dispatcher->GetEWrapper();

  TestHarness* th = dynamic_cast<TestHarness*>(ew);

  ApiProtocolHandler h(*ew);
  AsioEClientDriver ec(ioService, h);

  LOG(INFO) << "Started " << ioService.run() << std::endl;
  EXPECT_TRUE(ec.Connect("127.0.0.1", 4001, 0));

  bool connected = waitForConnection(ec, 5);
  EXPECT_TRUE(connected);

  // Request market data for a list of stocks:
  std::vector<std::string> stocks = boost::assign::list_of
      ("AAPL")("IBM")("GOOG")("MSFT")("GS")
      ("AMZN")("NFLX")("SPY")("PCLN")("BAC")
      ("GLD")("FAS")("FAZ")("JPM")("ORCL");

  GenericTickRequest genericTickRequest;
  genericTickRequest
      .add(RTVOLUME)
      .add(OPTION_VOLUME)
      .add(OPTION_IMPLIED_VOLATILITY)
      .add(HISTORICAL_VOLATILITY)
      .add(REALTIME_HISTORICAL_VOLATILITY)
      ;

  LOG(INFO) << "Request generic ticks: " << genericTickRequest.toString()
            << std::endl;

  for (std::vector<std::string>::iterator symbol = stocks.begin();
       symbol != stocks.end();
       ++symbol) {
    Contract c;
    TickerId tid = buildContract(c, *symbol);
    EXPECT_GT(tid, 0);

    bool snapShot = false;
    ec.GetEClient()->reqMktData(tid, c, genericTickRequest.toString(), snapShot);
  }

  sleep(5);

  // Sleep for a bit and then start sending data on the socket
  // from this thread:
  int REQUESTS = 5000;
  for (int i = 0; i < REQUESTS; ++i) {
    ec.GetEClient()->reqCurrentTime();
  }

  sleep(10);
  ec.Disconnect();

  EXPECT_EQ(th->getCount(CURRENT_TIME), REQUESTS);
}

