
#include <string>
#include <iostream>
#include <vector>

#include <boost/optional.hpp>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <leveldb/db.h>

#include "proto/common.hpp"
#include "proto/historian.hpp"
#include "historian/constants.hpp"
#include "historian/internal.hpp"
#include "historian/time_utils.hpp"

using std::string;
using boost::optional;
using boost::posix_time::ptime;
using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::Record;
using proto::historian::SessionLog;
using proto::historian::Query;
using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;

namespace testing {

using namespace leveldb;
struct DbUtil
{
  DbUtil(const string& file)
  {
    Options options;
    options.create_if_missing = true;
    EXPECT_TRUE(DB::Open(options, file, &db).ok());
  }

  ~DbUtil()
  {
    delete db;
  }

  bool Exists(const string& key)
  {
    string buffer;
    Status status = db->Get(ReadOptions(), key, &buffer);
    return !status.IsNotFound();
  }

  bool GetRaw(const string& key, string* buffer)
  {
    Status status = db->Get(ReadOptions(), key, buffer);
    return !status.IsNotFound();
  }

  template <typename P> bool Get(const string& key, P* proto)
  {
    string buffer;
    Status status = db->Get(ReadOptions(), key, &buffer);
    if (!status.IsNotFound()) {
      proto->ParseFromString(buffer);
      return proto->IsInitialized();
    } else {
      return false;
    }
  }

  bool Delete(const string& key)
  {
    return db->Delete(WriteOptions(), key).ok();
  }
  DB* db;
};

} // utils

const static std::string DB_FILE("/tmp/testdb");

TEST(InternalTest, MarketDataWriterTest)
{
  namespace c = proto::common;
  namespace h = proto::historian;

  const string est("2012-02-14 04:30:34.567899");
  ptime t;
  historian::parse(est, &t);

  MarketData d;

  d.set_timestamp(historian::as_micros(t));
  d.set_symbol("AAPL.STK");
  d.set_event("ASK");
  c::set_as(500., d.mutable_value());
  d.set_contract_id(9999);

  EXPECT_TRUE(d.IsInitialized());

  // Get the key from the writer
  using namespace historian::internal;
  Writer<MarketData> writer;
  const optional<string> key = writer.buildKey(d);
  EXPECT_TRUE(key);
  LOG(INFO) << "key = " << *key;

  // Now delete it to make sure there's nothing
  testing::DbUtil util(DB_FILE);
  util.Delete(*key);

  // Now try to write it
  EXPECT_TRUE(writer(d, util.db, false));
  EXPECT_TRUE(util.Exists(*key));

  // Now read data out
  Record record;
  EXPECT_TRUE(util.Get<Record>(*key, &record));
  EXPECT_TRUE(record.IsInitialized());
  optional<MarketData> read = h::as<MarketData>(record);
  EXPECT_TRUE(read);
  EXPECT_TRUE((*read).IsInitialized());

  // check the index key
  const optional<string> indexKey = writer.buildIndexKey(d);
  EXPECT_TRUE(indexKey);
  LOG(INFO) << "index key = " << *indexKey;
  EXPECT_TRUE(util.Exists(*indexKey));
  Record record2;
  EXPECT_TRUE(util.Get<Record>(*indexKey, &record2));
  EXPECT_TRUE(record2.IsInitialized());

  optional<Value> indexed2 = h::as<Value>(record2);
  EXPECT_TRUE(indexed2);
  EXPECT_TRUE((*indexed2).IsInitialized());

  double v1 =  *(c::as<double>(read->value()));
  double v2 =  *(c::as<double>(*indexed2));
  EXPECT_EQ(500., v1);
  EXPECT_EQ(v1, v2);
}

