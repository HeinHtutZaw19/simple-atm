#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "atm/Exceptions.h"
#include "atm/Ports.h"

// Test doubles for the two ports. These are the stand-ins for the real bank and
// real hardware. The controller cannot tell the difference -- it only sees the
// IBankService / ICashBin interfaces -- which is exactly why we can test the
// controller in isolation, with no real bank or cash machine.

namespace atm {

// A simple in-memory bank.
//
//   addCard("card", "1234", {"checking", "savings"});
//   setBalance("checking", 100);
class FakeBank : public IBankService {
public:
    void addCard(const std::string& card, const std::string& pin,
                 const std::vector<std::string>& accounts) {
        pins_[card] = pin;
        cardAccounts_[card] = accounts;
        for (const auto& a : accounts) {
            balances_.try_emplace(a, 0);
        }
    }

    void setBalance(const std::string& account, Money amount) {
        balances_[account] = amount;
    }

    bool validatePin(const std::string& card, const std::string& pin) override {
        ++validatePinCalls;
        auto it = pins_.find(card);
        return it != pins_.end() && it->second == pin;
    }

    std::vector<std::string> getAccounts(const std::string& card) override {
        auto it = cardAccounts_.find(card);
        if (it == cardAccounts_.end()) return {};
        return it->second;
    }

    Money getBalance(const std::string& account) override {
        return balances_.at(account);
    }

    Money deposit(const std::string& account, Money amount) override {
        balances_.at(account) += amount;
        return balances_.at(account);
    }

    Money withdraw(const std::string& account, Money amount) override {
        Money& bal = balances_.at(account);
        if (bal < amount) {
            throw InsufficientFundsError("Insufficient funds.");
        }
        bal -= amount;
        return bal;
    }

    int validatePinCalls = 0;

private:
    std::map<std::string, std::string> pins_;
    std::map<std::string, std::vector<std::string>> cardAccounts_;
    std::map<std::string, Money> balances_;
};

// A cash bin we can fully control from a test.
//
//   FakeCashBin bin(/*cash=*/500);     // bin starts with $500
//   bin.pendingDeposit = 40;           // customer will insert $40 on accept()
//   bin.dispenseShouldFail = true;     // simulate a hardware fault
class FakeCashBin : public ICashBin {
public:
    explicit FakeCashBin(Money cash = 0) : cash(cash) {}

    bool canDispense(Money amount) const override { return amount <= cash; }

    void dispense(Money amount) override {
        ++dispenseCalls;
        if (dispenseShouldFail) {
            throw CashBinError("Cash bin jammed.");
        }
        if (amount > cash) {
            throw CashBinError("Not enough cash in bin.");
        }
        cash -= amount;
    }

    Money accept() override {
        ++acceptCalls;
        Money a = pendingDeposit;
        pendingDeposit = 0;
        cash += a;
        return a;
    }

    Money cash = 0;
    Money pendingDeposit = 0;
    bool dispenseShouldFail = false;
    int dispenseCalls = 0;
    int acceptCalls = 0;
};

}  // namespace atm
