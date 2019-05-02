#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

using namespace eosio;

class [[eosio::contract]] zetaproto: public eosio::contract{

    private:
        struct [[eosio::table]] order{
            uint64_t id;
            name maker;
            uint64_t quantity;
            std::string currencyhave;
            asset tokenneed;
            std::string wallet_address;
            name taker;
            std::string receiving_wallet_address;
            std::string status;

            uint64_t primary_key()const {return id;};
        };

        typedef eosio::multi_index<"listedorders"_n, order> listed_order;

        struct [[eosio::table]] confirmorder{
            uint64_t id;
            name maker;
            name taker;
            std::string maker_voted;
            std::string taker_voted;
            std::string maker_confirmed;
            std::string taker_confirmed;

            uint64_t primary_key()const {return id;};
        };

        typedef eosio::multi_index<"confirmlist"_n, confirmorder> confirm_list;

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

        //  void sub_balance(name maker, asset amount, name fordata){
        //      action(
        //          permission_level{maker, "active"_n},
        //          get_self(),
        //          "subetabalance"_n,
        //          std::make_tuple(maker, amount, fordata)
        //      ).send();
        //  }

        void zetaexchange(uint64_t order_id){
            action(
                permission_level{_self, "active"_n},
                get_self(),
                "zetatransfer"_n,
                std::make_tuple(order_id)
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

         void add_bal(name to, asset val, name from) {
            
            action add = action(
              permission_level{get_self(),"active"_n},
              "dep.token"_n,
              "addbalance"_n,
              std::make_tuple(to, val, from)
            );
            add.send();
         };

         void check_commission(uint64_t id){
             action(
                 permission_level{_self, "active"_n},
                 get_self(),
                 "comshow"_n,
                 std::make_tuple(id)
             ).send();
         }

    public:
        using contract::contract;

        uint64_t commissionbaseperc;
        uint64_t lowerlimit;
        uint64_t upperlimit;
        uint64_t currentcommissionperc;

        zetaproto(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds) {};

        [[eosio::action]]
        void commission(uint64_t lower, uint64_t upper, uint64_t comperc){
            require_auth(_self);
            lowerlimit = lower;
            upperlimit = upper;
            commissionbaseperc = comperc;
        }

        [[eosio::action]]
        void comshow(uint64_t id){

            listed_order order(_self, _self.value);
            auto find_order = order.find(id);
            eosio_assert(find_order != order.end(), "Invalid order ID. ");
            const auto& get_order = order.get(id);
            if(get_order.quantity < lowerlimit) {
                currentcommissionperc = commissionbaseperc;
            }
            else if(get_order.quantity > lowerlimit && get_order.quantity < upperlimit){
                currentcommissionperc = commissionbaseperc - 1; 
            }
            else
                currentcommissionperc = commissionbaseperc - 2;
        }

        [[eosio::action]]
        void makeorder(name maker, uint64_t quantity, std::string currency, asset tokenneed, std::string wallet_address){
            require_auth(maker);
            
            eosio_assert(quantity > 0, "Quantity must be positive.");

            eosio_assert( tokenneed.is_valid(), "invalid supply");
            // eosio_assert( tokenneed.amount == quantity, "amount must be same as quantity of currency.");
            
            auto sym_need = tokenneed.symbol;

            tokenneed.amount = quantity;

            stats statstable( "dep.token"_n, "dep.token"_n.value );
            auto existing_symbol = statstable.find( sym_need.code().raw());
            eosio_assert( existing_symbol != statstable.end(), "token doesn't exist" );
            
            const auto& exchangecoins = statstable.get( sym_need.code().raw());
            eosio_assert(exchangecoins.native_currency == currency, "You can exchange currency with it's cosh equivalent only.");

            uint64_t oid = (maker.value*quantity)/(tokenneed.symbol.code().raw());

            // check_commission(oid)

            listed_order order(_self, _self.value);
            order.emplace(maker, [&](auto& row){
                row.id = oid;
                row.maker = maker;
                row.quantity = quantity;
                row.currencyhave = currency;
                row.tokenneed = tokenneed;
                row.wallet_address = wallet_address;
                row.taker = ""_n;
                row.receiving_wallet_address = "";
                row.status = "Open";
            });
        };

        [[eosio::action]]
        void takeorder(name taker, uint64_t orderid, std::string receiving_wallet_address){
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

                auto check_symbol = takeract.find(val.tokenneed.symbol.code().raw());
                eosio_assert( check_symbol != takeract.end(), "You don't have this token.");

                auto check_balance = takeract.get(val.tokenneed.symbol.code().raw());
                print("this is the balance in table", check_balance.balance.amount);
                eosio_assert(val.quantity <= check_balance.balance.amount, "Not enough balance");

                order.modify(open_order, taker,  [&](auto&row){
                    row.taker = taker;
                    row.receiving_wallet_address = receiving_wallet_address;
                    row.status = "Locked";
                });

                sub_bal(taker, val.tokenneed);
            }
        };

        [[eosio::action]]
        void add(name to, asset val, name from){
            add_bal(to,val, from);
        }

        [[eosio::action]]
        void cancelorder(name canceler, uint64_t order_id){
            require_auth(canceler);
            listed_order order(_self, _self.value);
            auto get_order = order.find(order_id);
            eosio_assert(get_order != order.end(), "Invalid order ID. ");

            const auto& del_order = order.get(order_id);
            eosio_assert(del_order.maker == canceler, "You can't cancel this order, because you are not the maker. ");
            eosio_assert(del_order.status == "Completed", "You can't cancel this order, because it's not open now. ");

            order.erase(get_order);
        }

        [[eosio::action]]
        void confirmtxn(name user, std::string transactionid, uint64_t order_id, std::string confrm){
            require_auth(user);
            listed_order order(_self, _self.value);
            auto get_order = order.find(order_id);
            eosio_assert(get_order != order.end(), "Invalid order ID. ");

            print(" After ORDER ");

            const auto& order_check = order.get(order_id);
            eosio_assert(user == order_check.maker || user == order_check.taker, "You can't confirm for this order. ");
            eosio_assert(order_check.status == "Locked", "You can't confirm this");

            print(" After ORDER CHECK ");

            confirm_list confirmlist(_self, _self.value);
            auto get_list = confirmlist.find(order_id);

            print(" After CONFIRM IN ");

            if(get_list == confirmlist.end()) {
                print(" After CONFIRM If ");
                if (user == order_check.maker) {
                    print(" After CONFIRM If MAKER ");
                    confirmlist.emplace(user, [&]( auto& row ) {
                        row.id = order_id;
                        row.maker = order_check.maker;
                        row.taker = order_check.taker;
                        row.maker_voted = "true";
                        row.taker_voted = "false";
                        row.maker_confirmed = confrm;
                        row.taker_confirmed = "false";
                    });
                }
                if (user == order_check.taker) {              print(" After CONFIRM If TAKER ");
                    confirmlist.emplace(user, [&]( auto& row ) {
                        row.id = order_id;
                        row.maker = order_check.maker;
                        row.taker = order_check.taker;
                        row.maker_voted = "false";
                        row.taker_voted = "true";
                        row.maker_confirmed = "false";
                        row.taker_confirmed = confrm;
                    });
                }
            }
            else {
                std::string changes;
                print(" After CONFIRM Else ");
                if (user == order_check.maker) {
                    print(" After CONFIRM Else MAKER");
                    confirmlist.modify(get_list, user, [&]( auto& row ) {
                        row.maker_voted = "true";
                        row.maker_confirmed = confrm;
                    });
                }
                if (user == order_check.taker) {
                    print(" After CONFIRM Else TAKER");
                    confirmlist.modify(get_list, user, [&]( auto& row ) {
                        row.taker_voted = "true";
                        row.taker_confirmed = confrm;
                    });
                }
            }
            
            print(" Running ZETA ");
            zetaexchange(order_id);
        }

        [[eosio::action]]
        void zetatransfer(uint64_t order_id){
            require_auth(_self);
            listed_order order(_self, _self.value);
            auto find_order = order.find(order_id);
            eosio_assert(find_order != order.end(), "Invalid order ID. ");

            confirm_list confirmlist(_self, _self.value);
            auto find_list = confirmlist.find(order_id);
            eosio_assert(find_list != confirmlist.end(), "No confirmation exist. ");

            const auto& get_order = order.get(order_id);

            const auto& get_list = confirmlist.get(order_id);

            if(get_list.maker_voted == "true" && get_list.taker_voted == "false"){
                print(" Waiting for taker to confirm the transaction. Once he confirms, transaction will proceed. ");
            }
            else if(get_list.maker_voted == "false" && get_list.taker_voted == "true"){
                print(" Waiting for maker to confirm the transaction. Once he confirms, transaction will proceed. ");
            }
            else {
                if(get_list.maker_confirmed == "true" && get_list.taker_confirmed == "true"){
                    add_bal(get_order.maker, get_order.tokenneed, get_order.taker);
                    print(" Zeta-transfer done. ");
                    order.modify(get_order, _self,  [&](auto& row){
                        row.status = "Completed";
                    });

                    confirmlist.erase(get_list);
                }
                else {
                    print(" Running Cosh-Voting. ");
                }
            }
        }

};

EOSIO_DISPATCH(zetaproto, (makeorder)(takeorder)(cancelorder)(add)(confirmtxn)(zetatransfer))