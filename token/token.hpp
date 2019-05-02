/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

using namespace eosio;

   class [[eosio::contract]] token : public eosio::contract {
      
      public:
         static name owner;
         token( name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
            owner = _self;
            print("The owner is: ", owner);
         }

      [[eosio::action]]
        void make(name maker);

      [[eosio::action]]
         void create( name issuer, std::string token_name, std::string native_currency, 
                      asset        maximum_supply);

      [[eosio::action]]
         void issue(asset quantity, std::string memo );

      [[eosio::action]]
         void transfer( name from,
                        name to,
                        asset        quantity,
                        std::string       memo );

      [[eosio::action]]
         void subbalance( name owner, asset value );

      [[eosio::action]]
         void addbalance(name owner, asset value, name payer);
      
      [[eosio::action]]
         void etasub(name owner, asset value);

      [[eosio::action]]
         void etaadd(name owner, asset value, name payer);
      // [[eosio::action]]
      //    name static get_owner(){
      //       return owner;
      //    }

      private:
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

         void sub_bal(name from, asset val) {
            action(
              permission_level{from,"active"_n},
              get_self(),
              "subbalance"_n,
              std::make_tuple(from, val)
            ).send();
         };

         void add_bal(name to, asset val, name from) {
            action(
              permission_level{to,"active"_n},
              get_self(),
              "addbalance"_n,
              std::make_tuple(to, val, from)
            ).send();
         };

         // void get_owner(){
         //     action owner = action(
         //         permission_level{get_self(), "active"_n},
         //         "token"_n,
         //     )
         // }

         typedef eosio::multi_index<"accounts"_n, account> accounts;
         typedef eosio::multi_index<"stat"_n, currency_stats> stats;

      public:
         struct transfer_args {
            name  from;
            name  to;
            asset         quantity;
            std::string        memo;
         };
   };

