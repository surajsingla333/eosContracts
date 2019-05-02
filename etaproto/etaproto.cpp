#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include "../token/token.hpp"
#include <eosiolib/time.hpp>

using namespace eosio;

class [[eosio::contract]] etaproto: public eosio::contract{

    private:
        struct [[eosio::table]] order{
            uint64_t id;
            name maker;
            asset amount;
            asset exchangetoken;
            std::string status;

            uint64_t primary_key()const {return id;};
        };
        typedef eosio::multi_index<"listedorders"_n, order> listed_order;

        struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            std::string token_name;
            std::string native_currency;
            asset          supply;
            asset          max_supply;
            name   issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index<"accounts"_n, account> accounts;
         typedef eosio::multi_index<"stat"_n, currency_stats> stats;

         void add_bal(name to, asset val, name from) {
            
            action add = action(
              permission_level{get_self(),"active"_n},
              "dep.token"_n,
              "addbalance"_n,
              std::make_tuple(to, val, from)
            );
            add.send();
         };

         void eta_exchange(name maker, name taker, asset maker_quantity, asset taker_quantity, std::string memo ,name fordata) {
            action(
              permission_level{_self,"active"_n},
              get_self(),
              "etatransfer"_n,
              std::make_tuple(maker, taker, maker_quantity, taker_quantity, memo, fordata)
            ).send();
         };

         void sub_bal(name maker, asset amount){
             
             action minus = action(
                 permission_level{get_self(), "active"_n},
                 "dep.token"_n,
                 "subbalance"_n,
                 std::make_tuple(maker, amount)
             );

             minus.send();
         };

         void makein(name maker){
             action(
              permission_level{maker,"active"_n},
              get_self(),
              "make"_n,
              std::make_tuple(maker)
            ).send();
         }


    public:

        etaproto(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {
            // name owner = token::get_owner();
            print("Owner of token is: ", "token"_n);
        };

        [[eosio::action]]
        void subamount(name from, asset value){
            require_auth(from);
            sub_bal(from, value);
            print(" in etaproto (self) ", name(_self));
            print(" in etaproto (code) ", name(_code));
        }

        [[eosio::action]]
        void addamount(name from, asset value, name payer){
            require_auth(from);
            add_bal(from, value, payer);
            print(" in etaproto (self) ", name(_self));
            print(" in etaproto (code) ", name(_code));
        }

        [[eosio::action]]
        void make(name maker){
            require_auth(maker);
            // eosio_assert(user == "alice"_n || user == "bob"_n || user == ""_n,  "error");
            name user = ""_n;
            // utc_seconds 
            print(" eta in make ", user);
            print(" eta in make ", maker);
        }

        [[eosio::action]]
        void makeorder(name maker, asset amount, asset exchangetoken){
            require_auth(maker);
            
            eosio_assert( amount.is_valid(), "invalid supply");
            eosio_assert( amount.amount > 0, "amount must be positive");

            auto sym_have = amount.symbol;

            // change 2nd para according to the account which will deploy the token contract
            stats statstable( "dep.token"_n, "dep.token"_n.value );
            auto existing_symbol = statstable.find( sym_have.code().raw());
            eosio_assert( existing_symbol != statstable.end(), "token doesn't exist" );

            auto sym_need = exchangetoken.symbol;
            auto existing_token = statstable.find( sym_need.code().raw() );
            eosio_assert( existing_token != statstable.end(), "token doesn't exist" );

            exchangetoken.amount = amount.amount;

            accounts makeract( "dep.token"_n , maker.value);

            auto check_symbol = makeract.find(sym_have.code().raw());
            eosio_assert( check_symbol != makeract.end(), "You don't have this token.");

            auto check_balance = makeract.get(sym_have.code().raw());
            print("this is the balance in table", check_balance.balance.amount);
            eosio_assert(amount.amount <= check_balance.balance.amount, "Not enough balance");

            uint64_t oid = (maker.value*amount.amount)/(amount.symbol.code().raw());

            listed_order order(_self, _self.value);
            
            auto open_order = order.find( oid );
            if(open_order != order.end()){
                oid = (oid*maker.value)/(amount.symbol.code().raw());
            }
        
            order.emplace(maker, [&](auto& row){
                row.id = oid;
                row.maker = maker;
                row.amount = amount;
                row.exchangetoken = exchangetoken;
                row.status = "Open";
            });
            // sub_balance(maker, amount, "dep.token"_n);
         };

        [[eosio::action]]
        void takeorder(name taker, uint64_t orderid){
            require_auth(taker);
            
            listed_order order(_self, _self.value);
            auto open_order = order.find( orderid );
            eosio_assert(open_order != order.end(), "Invalid order ID.");
            
            const auto& val = order.get(orderid);
            if(taker==val.maker){
                print("Maker can't take back the order. You need yo cancel it.");
            }
            else{
                eosio_assert(val.status == "Open", "Order   already locked.");

                accounts takeract( "dep.token"_n , taker.value);

                auto check_symbol = takeract.find(val.exchangetoken.symbol.code().raw());
                eosio_assert( check_symbol != takeract.end(), "You don't have this token.");
               
                auto check_balance = takeract.get(val.exchangetoken.symbol.code().raw());
                print("this is the balance in table", check_balance.balance.amount);
                eosio_assert(val.amount.amount <= check_balance.balance.amount, "Not enough balance");

                order.modify(open_order, _self,  [&](auto& row){
                    row.status = "Locked";
                });
                eta_exchange(val.maker, taker, val.amount, val.exchangetoken,  "ETA EXCHANGE","dep.token"_n);
                makein(val.maker);
            }
        };

        [[eosio::action]]
        void cancelorder(name canceler, uint64_t order_id){
            require_auth(canceler);
            listed_order order(_self, _self.value);
            auto get_order = order.find(order_id);
            eosio_assert(get_order != order.end(), "Invalid order ID. ");

            const auto& del_order = order.get(order_id);
            eosio_assert(del_order.maker == canceler, "You can't cancel this order, because you are not the maker. ");
            eosio_assert(del_order.status == "Locked", "You can't cancel this order, because it's not open now. ");

            order.erase(get_order);
        }

    //     [[eosio::action]]
    //     void subetabalance( name fromacc, asset value, name fordata) 
    //     {
    //          eosio_assert(fordata == "dep.token"_n, "wrong worker data");
    //          require_recipient( fordata );
    //          print(" sub1 ",fordata);
    //          accounts from_acnts( fordata, fromacc.value );
    //          print(" sub2 ");
    //          auto _from = from_acnts.find( value.symbol.code().raw() );
    //          eosio_assert( _from != from_acnts.end(), "No   balance object" );
    //          print(" sub3 ", value.symbol);

    //          const auto& from = from_acnts.get(     value.symbol.code().raw());
    //          eosio_assert( from.balance.amount >=   value.amount, "overdrawn balance" );
    //          print("sub4");
    //          if( from.balance.amount == value.amount ) {
    //             from_acnts.erase( _from );
    //             print(" subif1 ");
    //          } else {
    //             from_acnts.modify( _from, fromacc, [&]( auto&a) {
    //                 // a.balance -= value;
    //                 print(" subif2 ");
    //             });
    //             print("else");
    //         }
    //    };

    //    [[eosio::action]]
    //    void addetabalance( name owner, asset value, name ram_payer , name fordata)
    //   {
    //      eosio_assert(fordata == "dep.token"_n, "wrong worker data");
    //      require_recipient( owner );
    //      print("add1");
    //      accounts to_acnts( fordata, owner.value );
    //      print("add2");
    //      auto to = to_acnts.find( value.symbol.code().raw() );
    //      print("add3");
    //      if( to == to_acnts.end() ) {
    //         // to_acnts.emplace( fordata, [&]( auto& a ){
    //         // a.balance = value;
    //         // print("addifinplace1");
    //         // });
    //         print(" addif1 ");
    //      } else {
    //         // to_acnts.modify( to, fordata, [&]( auto& a ) {
    //         //   a.balance += value;
    //         //   print("addelseinmod1");
    //         // });
    //         print(" addelse2 ");
    //      }
    //   };

    //     [[eosio::action]]
    //     void transfer( name from, name to, asset quantity, std::string memo , name fordata)
    //   {
    //       eosio_assert( from != to, "cannot transfer to self" );
    //       require_auth( from );
    //       eosio_assert( is_account( to ), "to account does not exist");
    //       auto sym = quantity.symbol;
    //       stats statstable( fordata, fordata.value );
    //       const auto& st = statstable.get( sym.code().raw() );

    //       require_recipient( from );
    //       require_recipient( to );

    //       eosio_assert( quantity.is_valid(), "invalid quantity" );
    //       eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    //       eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    //       eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    //     //   sub_bal( from, quantity, fordata );
    //     //   add_bal( to, quantity, from, fordata );
    //   }


      [[eosio::action]]
      void etatransfer( name maker, name taker, asset maker_quantity, asset taker_quantity, std::string memo ,name fordata)
      {
          require_auth(_self);
          eosio_assert(fordata == "dep.token"_n, "wrong worker data");
          eosio_assert( is_account( maker ), "maker account does not exist");
          eosio_assert( is_account( taker ), "taker account does not exist");

          auto sym_maker = maker_quantity.symbol;
          auto sym_taker = taker_quantity.symbol;

          stats statstable( fordata, fordata.value );

          const auto& st_maker = statstable.get( sym_maker.code().raw() );
          const auto& st_taker = statstable.get( sym_taker.code().raw() );

          require_recipient( maker );
          require_recipient( taker );

          eosio_assert( maker_quantity.is_valid(), "invalid quantity" );
          eosio_assert( maker_quantity.symbol == st_maker.supply.symbol, "symbol precision mismatch" );
          eosio_assert( taker_quantity.is_valid(), "invalid quantity" );
          eosio_assert( taker_quantity.symbol == st_taker.supply.symbol, "symbol precision mismatch" );

          eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

          sub_bal( maker, maker_quantity);
          add_bal( taker, maker_quantity, maker);
          sub_bal( taker, taker_quantity);
          add_bal( maker, taker_quantity, taker);
      }
};

EOSIO_DISPATCH( etaproto, (make)(subamount)(makeorder)(takeorder)(cancelorder)(addamount)(etatransfer))