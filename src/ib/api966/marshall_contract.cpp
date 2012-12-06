
#include <iostream>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "marshall.hpp"

bool ParseDate(const std::string& date, int* year, int* month, int* day)
{
  if (date.length() != 8) return false;

  *year = boost::lexical_cast<int>(date.substr(0,4));
  *month = boost::lexical_cast<int>(date.substr(4,2));
  *day = boost::lexical_cast<int>(date.substr(6,2));
  return true;
}

std::string FormatExpiry(int year, int month, int day)
{
  ostringstream s1;
  string fmt = (month > 9) ? "%4d%2d" : "%4d0%1d";
  string fmt2 = (day > 9) ? "%2d" : "0%1d";
  s1 << boost::format(fmt) % year % month << boost::format(fmt2) % day;
  return s1.str();
}


bool operator<<(Contract& c, const proto::ib::Contract& p)
{
  // check required:
  if (!p.IsInitialized()) {
    return false;
  }

  switch (p.type()) {
    case proto::ib::Contract::INDEX:
      c.secType = "IND";
      break;
    case proto::ib::Contract::STOCK:
      c.secType = "STK";
      break;
    case proto::ib::Contract::OPTION:
      c.secType = "OPT";

      // check required:
      if (p.id() == 0) break; // This is a query. conId is not known.

      if (!p.has_right() ||
          !p.has_strike() ||
          !p.has_expiry() ||
          p.multiplier() < 100) {
        return false;
      }
      break;
  }

  c.conId = p.id();
  c.symbol = p.symbol();
  c.exchange = p.exchange();
  if (p.has_local_symbol()) c.localSymbol = p.local_symbol();
  if (p.has_right()) {
    switch (p.right()) {
      case proto::ib::Contract::PUT:
        c.right = "P";
        break;
      case proto::ib::Contract::CALL:
        c.right = "C";
        break;
    }
  }
  if (p.has_strike()) {
    c.strike = p.strike().amount();
    switch (p.strike().currency()) {
      case proto::common::Money::USD:
        c.currency = "USD";
        break;
    }
  }
  if (p.has_expiry()) {
    c.expiry = FormatExpiry(p.expiry().year(),
                            p.expiry().month(),
                            p.expiry().day());
  }

  return true;
}

bool operator<<(proto::ib::Contract& p, const Contract& c)
{
  p.Clear();

  p.set_id(c.conId);
  p.set_symbol(c.symbol);
  if (c.secType == "IND") {
    p.set_type(proto::ib::Contract::INDEX);
  } else if (c.secType == "STK") {
    p.set_type(proto::ib::Contract::STOCK);
  } else if (c.secType == "OPT") {
    p.set_type(proto::ib::Contract::OPTION);
  }
  if (c.exchange.length() > 0) {
    p.set_exchange(c.exchange);
  }

  p.set_local_symbol(c.localSymbol);
  if (c.right == "P") {
    p.set_right(proto::ib::Contract::PUT);
  } else if (c.right == "C") {
    p.set_right(proto::ib::Contract::CALL);
  }
  p.mutable_strike()->set_amount(c.strike);
  if (c.currency == "USD") {
    p.mutable_strike()->set_currency(proto::common::Money::USD);
  }
  if (c.expiry.length() == 8) {

    int year, month, day;
    if (ParseDate(c.expiry, &year, &month, &day)) {
      p.mutable_expiry()->set_year(year);
      p.mutable_expiry()->set_month(month);
      p.mutable_expiry()->set_day(day);
    }
  }
  return true;
}

//////////////// CONTRACT DETAIL /////////////////

bool operator<<(ContractDetails& c, const proto::ib::ContractDetails& p)
{
  // NOT IMPLEMENTED -- the API never really requires sending contract details.
  return true;
}

bool operator<<(proto::ib::ContractDetails& p, const ContractDetails& c)
{
  p.Clear();

  proto::ib::Contract *summary = p.mutable_summary();
  if (*summary << c.summary) {
    p.set_id(c.summary.conId);
    // compute the standard symbol... AAPL.STK or AAPL.OPT.20121130.650.C
    std::ostringstream symbol;

    symbol << c.summary.symbol << '.' << c.summary.secType;
    if (summary->type() == proto::ib::Contract::OPTION) {
      symbol << '.'
             << summary->expiry().year()
             << summary->expiry().month()
             << summary->expiry().day()
             << '.'
             << c.summary.strike << '.' << c.summary.right;
    }

    p.set_symbol(symbol.str());
    p.set_marketname(c.marketName);
    p.set_tradingclass(c.tradingClass);
    p.set_mintick(c.minTick);

    p.set_ordertypes(c.orderTypes);
    p.set_validexchanges(c.validExchanges);
    p.set_pricemagnifier(c.priceMagnifier);
    p.set_underconid(c.underConId);
    p.set_longname(c.longName);
    p.set_contractmonth(c.contractMonth);
    p.set_industry(c.industry);
    p.set_category(c.category);
    p.set_subcategory(c.subcategory);
    p.set_timezoneid(c.timeZoneId);
    p.set_tradinghours(c.tradingHours);
    p.set_liquidhours(c.liquidHours);

    return true;
  }
  return false;
}

