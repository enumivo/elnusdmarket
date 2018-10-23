#include "ex.hpp"

#include <cmath>
#include <enulib/action.hpp>
#include <enulib/asset.hpp>
#include "enu.token.hpp"

using namespace enumivo;
using namespace std;

void ex::buy(const currency::transfer &transfer) {
  if (transfer.to != _self) {
    return;
  }

  // get ELN balance
  double eln_balance = enumivo::token(N(eln.coin)).
	   get_balance(_self, enumivo::symbol_type(ELN_SYMBOL).name()).amount;
  
  eln_balance = eln_balance/10000;

  double received = transfer.quantity.amount;
  received = received/10000;

  eln_balance = eln_balance-received;  

  // get USD balance
  double usd_balance = enumivo::token(N(stable.coin)).
	   get_balance(_self, enumivo::symbol_type(USD_SYMBOL).name()).amount;

  usd_balance = usd_balance/10000;

  // get USD supply
  double usd_supply = enumivo::token(N(stable.coin)).
	   get_supply(enumivo::symbol_type(USD_SYMBOL).name()).amount;

  usd_supply = usd_supply/10000;

  //y = 100k / sqrt(x)
  double buy = pow((received/200000)+sqrt(usd_supply),2)-usd_supply;

  auto to = transfer.from;

  auto quantity = asset(10000*buy, USD_SYMBOL);

  action(permission_level{_self, N(active)}, N(stable.coin), N(issue),
         std::make_tuple(to, quantity,
                         std::string("Create USD")))
      .send();
}

void ex::sell(const currency::transfer &transfer) {
  if (transfer.to != _self) {
    return;
  }

  // get USD balance
  double usd_balance = enumivo::token(N(stable.coin)).
	   get_balance(_self, enumivo::symbol_type(USD_SYMBOL).name()).amount;
  
  usd_balance = usd_balance/10000;

  double received = transfer.quantity.amount;
  received = received/10000;

  usd_balance = usd_balance-received;  

  // get ELN balance
  double eln_balance = enumivo::token(N(eln.coin)).
	   get_balance(_self, enumivo::symbol_type(ELN_SYMBOL).name()).amount;

  eln_balance = eln_balance/10000;

    // get USD supply
  double usd_supply = enumivo::token(N(stable.coin)).
	   get_supply(enumivo::symbol_type(USD_SYMBOL).name()).amount;

  usd_supply = usd_supply/10000;

  //y = 100k / sqrt(x)
  double sell = 200000*(sqrt(usd_supply+received)-sqrt(usd_supply));

  auto to = transfer.from;

  auto quantity = asset(10000*sell, ELN_SYMBOL);

  action(permission_level{_self, N(active)}, N(eln.coin), N(transfer),
         std::make_tuple(_self, to, quantity,
                         std::string("Sell USD for ELN")))
      .send();

  auto toretire = asset(10000*received, USD_SYMBOL);

  action(permission_level{_self, N(active)}, N(stable.coin), N(retire),
         std::make_tuple(toretire, std::string("Destroy USD")))
      .send();
}

void ex::apply(account_name contract, action_name act) {
  if (contract == N(eln.coin) && act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();
    enumivo_assert(transfer.quantity.symbol == ELN_SYMBOL,
                 "Must send ELN");
    buy(transfer);
    return;
  }

  if (contract == N(stable.coin) && act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();
    enumivo_assert(transfer.quantity.symbol == USD_SYMBOL,
                 "Must send USD");
    sell(transfer);
    return;
  }

  if (act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();
    enumivo_assert(false, "Must send USD or ELN");
    sell(transfer);
    return;
  }

  if (contract != _self) return;
}

extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
  ex elnusd(receiver);
  elnusd.apply(code, action);
  enumivo_exit(0);
}
}
