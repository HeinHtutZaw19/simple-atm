#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <vector>
#include "atm/Ports.h"

namespace atm
{
    constexpr int kDefaultMaxPinAttempts = 3;
    enum class State
    {
        Idle,
        Authenticating,
        SelectingAccount,
        Transacting,
    };

    class ATMController
    {
    public:
        ATMController(IBankService &bank, ICashBin &cashBin, int maxPinAttempts = kDefaultMaxPinAttempts);
        State state() const { return state_; }
        const std::optional<std::string> &selectedAccount() const
        {
            return selectedAccount_;
        }
        void insertCard(const std::string &cardNumber);
        std::vector<std::string> enterPin(const std::string &pin);
        std::vector<std::string> listAccounts() const;
        void selectAccount(const std::string &accountId);

        Money checkBalance();
        Money deposit();
        Money withdraw(Money amount);
        void ejectCard();

    private:
        void reset();
        void requireState(std::initializer_list<State> allowed) const;
        const std::string &requireAccount() const;
        IBankService &bank_;
        ICashBin &cashBin_;
        int maxPinAttempts_;
        State state_;
        std::string cardNumber_;
        int pinAttempts_;
        std::vector<std::string> accounts_;
        std::optional<std::string> selectedAccount_;
    };
}