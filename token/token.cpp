/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "token.hpp"

using namespace eosio;


      void token::make(name maker){
         print("Hello ",name{maker});
      };

      void token::create( name issuer, std::string token_name, std::string native_currency, 
                          asset        maximum_supply )
      {
          require_auth( _self );
          print("\nThis is the owner: ", owner);
          auto sym = maximum_supply.symbol;
          eosio_assert( sym.is_valid(), "invalid symbol name" );
          eosio_assert( maximum_supply.is_valid(), "invalid supply");
          eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

          stats statstable( _self, _self.value );
          auto existing_symbol = statstable.find( sym.code().raw() );
          eosio_assert( existing_symbol == statstable.end(), "token with symbol already exists" );

          statstable.emplace( _self, [&]( auto& s ) {
             s.token_name = token_name;
             s.native_currency = native_currency; 
             s.supply.symbol = maximum_supply.symbol;
             s.max_supply    = maximum_supply;
             s.issuer        = issuer; 
          });
      }

      void token::issue(asset quantity, std::string memo )
      {
          auto sym = quantity.symbol;
          eosio_assert( sym.is_valid(), "invalid symbol name" );
          eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

          auto sym_name = sym;
          stats statstable( _self, _self.value );
          auto existing_symbol = statstable.find( sym_name.code().raw() );
          eosio_assert( existing_symbol != statstable.end(), "token with symbol does not exist, create token before issue" );
          const auto& st = *existing_symbol;

          require_auth( st.issuer );
          eosio_assert( quantity.is_valid(), "invalid quantity" );
          eosio_assert( quantity.amount > 0, "must issue positive quantity" );

          eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
          eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

          statstable.modify( st, _self, [&]( auto& s ) {
             s.supply += quantity;
          });

          add_bal( st.issuer, quantity, st.issuer );
      }

      void token::transfer( name from, name to, asset quantity, std::string memo )
      {
          eosio_assert( from != to, "cannot transfer to self" );
          require_auth( from );
          eosio_assert( is_account( to ), "to account does not exist");
          auto sym = quantity.symbol;
          stats statstable( _self, _self.value );
          const auto& st = statstable.get( sym.code().raw() );

          require_recipient( from );
          require_recipient( to );

          eosio_assert( quantity.is_valid(), "invalid quantity" );
          eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
          eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
          eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

          sub_bal( from, quantity );
          add_bal( to, quantity, from );
      }

      void token::subbalance( name fromacc, asset value ) 
      {
         require_recipient( _self );
         print("sub1");
         accounts from_acnts( _self, fromacc.value );
         print("sub2");
         auto _from = from_acnts.find( value.symbol.code().raw() );
         eosio_assert( _from != from_acnts.end(), "No balance object" );
         print("sub3");
         const auto& from = from_acnts.get( value.symbol.code().raw());
         eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );
         print("sub4");
         if( from.balance.amount == value.amount ) {
            from_acnts.erase( from );
            print("subif1");
         } else {
            from_acnts.modify( from, _self, [&]( auto& a ) {
                a.balance -= value;
                print("subif2");
            });
         }
      }

      void token::addbalance( name owner, asset value, name ram_payer )
      {
         require_recipient( owner );
         print("add1");
         accounts to_acnts( _self, owner.value );
         print("add2");
         auto to = to_acnts.find( value.symbol.code().raw() );
         print("add3");
         if( to == to_acnts.end() ) {
            to_acnts.emplace( _self, [&]( auto& a ){
            a.balance = value;
            print("addifinplace1");
            });
            print("addif1");
         } else {
            to_acnts.modify( to, _self, [&]( auto& a ) {
              a.balance += value;
              print("addelseinmod1");
            });
         }
      }

      [[eosio::action]]
      void token::etasub( name fromacc, asset value ) 
      {
         // require_auth(name("dep.eta"));
         print(" in tokens (self) ", _self);
         print(" in token (code) ", _code);
         // require_recipient( _code );
         // print("sub1");
         // accounts from_acnts( name(_code), fromacc.value );
         // print("sub2");
         // auto _from = from_acnts.find( value.symbol.code().raw() );
         // eosio_assert( _from != from_acnts.end(), "No balance object" );
         // print("sub3");
         // const auto& from = from_acnts.get( value.symbol.code().raw());
         // eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );
         // print("sub4");
         // if( from.balance.amount == value.amount ) {
         //    from_acnts.erase( from );
         //    print("subif1");
         // } else {
         //    from_acnts.modify( from, fromacc, [&]( auto& a ) {
         //        a.balance -= value;
         //        print("subif2");
         //    });
         // }
      }

      void token::etaadd( name owner, asset value, name ram_payer )
      {
         require_auth("dep.token"_n);
         require_recipient( owner );
         print("add1");
         accounts to_acnts( name(_code), owner.value );
         print("add2");
         auto to = to_acnts.find( value.symbol.code().raw() );
         print("add3");
         if( to == to_acnts.end() ) {
            to_acnts.emplace( _code, [&]( auto& a ){
            a.balance = value;
            print("addifinplace1");
            });
            print("addif1");
         } else {
            to_acnts.modify( to, _code, [&]( auto& a ) {
              a.balance += value;
              print("addelseinmod1");
            });
         }
      }


EOSIO_DISPATCH( token, (create)(issue)(transfer)(make)(subbalance)(addbalance)(etasub)(etaadd))