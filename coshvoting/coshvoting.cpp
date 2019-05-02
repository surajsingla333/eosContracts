#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include "../token/token.hpp"

using namespace eosio;

class [[eosio::contract]] coshvoting: public eosio::contract{

    private:
        struct [[eosio::table]] voter{
            name key;
            bool vote;

            uint64_t primary_key()const {return key.value;};
        };
        typedef eosio::multi_index<"voters"_n, voter> voter_list;

        // struct [[eosio::table]] account {
        //     asset    balance;

        //     uint64_t primary_key()const { return balance.symbol.code().raw(); }
        //  };

        //  struct [[eosio::table]] currency_stats {
        // string token_name;
        //     std::string native_currency;
        //     asset          supply;
        //     asset          max_supply;
        //     name   issuer;

        //     uint64_t primary_key()const { return supply.symbol.code().raw(); }
        //  };

        //  typedef eosio::multi_index<"accounts"_n, account> accounts;
        //  typedef eosio::multi_index<"stat"_n, currency_stats> stats;

        //  void sub_balance(name maker, asset amount, name fordata){
        //      action(
        //          permission_level{maker, "active"_n},
        //          get_self(),
        //          "subetabalance"_n,
        //          std::make_tuple(maker, amount, fordata)
        //      ).send();
        //  }

    public:

        coshvoting(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {
            name owner = token::get_owner();
            print("Owner of token is: ", "token"_n);
        };

        [[eosio::action]]
        void make(name maker){
            print("Hello ",name{maker});
        };

        [[eosio::action]]
        void vote(name voter, bool vote){
            
            require_auth(voter);
            
            voter_list voters(_self, _self.value);
            auto vlist = voters.find(voter.value);
            eosio_assert( vlist == voters.end(), "You have already voted");
            
            voters.emplace(_self, [&](auto & row){
                row.key = voter;
                row.vote = vote;
            });
        }

//         [[eosio::action]]
//             auto sym_have = amount.symbol;

//             // change 2nd para according to the account which will deploy the token contract
//             stats statstable( "dep.token"_n, "dep.token"_n.value );
//             auto existing_symbol = statstable.find( sym_have.code().raw());
//             eosio_assert( existing_symbol != statstable.end(), "token doesn't exist" );

//             auto sym_need = exchangetoken.symbol;
//             auto existing_token = statstable.find( sym_need.code().raw() );
//             eosio_assert( existing_token != statstable.end(), "token doesn't exist" );

//             accounts makeract( "dep.token"_n , maker.value);

//             auto check_symbol = makeract.find(sym_have.code().raw());
//             eosio_assert( check_symbol != makeract.end(), "You don't have this token.");

//             auto check_balance = makeract.get(sym_have.code().raw());
//             print("this is the balance in table", check_balance.balance.amount);
//             eosio_assert(amount.amount <= check_balance.balance.amount, "Not enough balance");

//             listed_order order(_self, _self.value);
//             order.emplace(maker, [&](auto& row){
//                 row.id = 1;
//                 row.maker = maker;
//                 row.amount = amount;
//                 row.exchangetoken = exchangetoken;
//                 row.status = "Open";
//             });
//             sub_balance(maker, amount, "dep.token"_n);
//          };

//         [[eosio::action]]
//         void takeorder(name taker, uint64_t orderid){
//             require_auth(taker);
            
//             listed_order order(_self, _self.value);
//             auto open_order = order.find( orderid );
//             eosio_assert(open_order != order.end(), "Invalid order ID.");
            
//             const auto& val = order.get(orderid);
//             if(taker==val.maker){
//                 print("Maker can't take back the order. You need yo cancel it.");
//             }
//             else{
//                 eosio_assert(val.status == "Open", "Order   already locked.");

//                 accounts takeract( "dep.token"_n , taker.value);

//                 auto check_symbol = takeract.find(val.exchangetoken.symbol.code().raw());
//                 eosio_assert( check_symbol != takeract.end(), "You don't have this token.");
               
//                 auto check_balance = takeract.get(val.exchangetoken.symbol.code().raw());
//                 print("this is the balance in table", check_balance.balance.amount);
//                 eosio_assert(val.amount.amount <= check_balance.balance.amount, "Not enough balance");

//                 order.modify(open_order, _self,  [&](auto&row){
//                     row.status = "Locked";
//                 });
//             }
//         };

//         [[eosio::action]]
//         void cancelorder(name canceler, uint64_t order_id){
//             require_auth(canceler);
//             listed_order order(_self, _self.value);
//             auto get_order = order.find(order_id);
//             eosio_assert(get_order != order.end(), "Invalid order ID. ");

//             const auto& del_order = order.get(order_id);
//             eosio_assert(del_order.maker == canceler, "You can't cancel this order, because you are not the maker. ");
//             eosio_assert(del_order.status == "Open", "You can't cancel this order, because it's not open now. ");

//             order.erase(get_order);
//         }

//         [[eosio::action]]
//         void subetabalance( name fromacc, asset value, name fordata) 
//         {
//              require_recipient( fordata );
//              print(" sub1 ",fordata);
//              accounts from_acnts( fordata, fromacc.value );
//              print(" sub2 ");
//              auto _from = from_acnts.find( value.symbol.code().raw() );
//              eosio_assert( _from != from_acnts.end(), "No   balance object" );
//              print(" sub3 ", value.symbol);

//              const auto& from = from_acnts.get(     value.symbol.code().raw());
//              eosio_assert( from.balance.amount >=   value.amount, "overdrawn balance" );
//              print("sub4");
//              if( from.balance.amount == value.amount ) {
//                 from_acnts.erase( _from );
//                 print("subif1");
//              } else {
//                 from_acnts.modify( _from, fromacc, [&]( auto&a) {
//                     a.balance -= value;
//                     print("subif2");
//                 });
//             }
//        };
};

EOSIO_DISPATCH( coshvoting, (make)(vote))