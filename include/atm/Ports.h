#pragma once

#include <string>
#include <vector>

namespace atm
{
    using Money = int;
    class IBankService
    {
    public:
        virtual ~IBankService() = default;
        virtual bool validatePin(const std::string &cardNumber, const std::string &pin) = 0;
        virtual std::vector<std::string> getAccounts(const std::string &cardNumber) = 0;
        virtual Money getBalance(const std::string &accountId) = 0;
        virtual Money deposit(const std::string &accountId, Money amount) = 0;
        virtual Money withdraw(const std::string &accountId, Money amount) = 0;
    };
    class ICashBin
    {
    public:
        virtual ~ICashBin() = default;
        virtual bool canDispense(Money amount) const = 0;
        virtual void dispense(Money amount) = 0;
        virtual Money accept() = 0;
    };
}