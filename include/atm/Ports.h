#pragma once

#include <string>
#include <vector>

namespace atm
{
    /// Monetary amount in whole dollars. Always non-negative for balances and
    /// transaction amounts; the controller rejects negative or zero values where
    /// they are not meaningful.
    using Money = int;

    /// Port to the backing bank/account system.
    ///
    /// The ATMController owns no account state itself; it delegates all
    /// authentication and balance changes to an implementation of this
    /// interface. Implementations are expected to be authoritative: the balance
    /// returned by deposit/withdraw is the new balance of record.
    class IBankService
    {
    public:
        virtual ~IBankService() = default;

        /// Returns true if pin authenticates the holder of cardNumber.
        virtual bool validatePin(const std::string &cardNumber, const std::string &pin) = 0;

        /// Returns the account identifiers linked to cardNumber. May be empty.
        virtual std::vector<std::string> getAccounts(const std::string &cardNumber) = 0;

        /// Returns the current balance of accountId.
        virtual Money getBalance(const std::string &accountId) = 0;

        /// Credits amount to accountId and returns the new balance.
        virtual Money deposit(const std::string &accountId, Money amount) = 0;

        /// Debits amount from accountId and returns the new balance.
        /// Implementations should throw if funds are insufficient.
        virtual Money withdraw(const std::string &accountId, Money amount) = 0;
    };

    /// Port to the physical cash-handling hardware (dispenser and deposit slot).
    ///
    /// This isolates the controller from device specifics so it can be tested
    /// against a fake. Balance changes are the bank's job; this interface only
    /// moves cash.
    class ICashBin
    {
    public:
        virtual ~ICashBin() = default;

        /// Returns true if the bin currently holds enough cash to hand out
        /// amount. Checked before debiting the account so a withdrawal never
        /// completes when the hardware cannot fulfil it.
        virtual bool canDispense(Money amount) const = 0;

        /// Physically dispenses amount. May throw on a hardware fault; the
        /// controller treats a throw as a failed withdrawal and rolls back.
        virtual void dispense(Money amount) = 0;

        /// Accepts cash inserted by the customer and returns the counted amount
        /// (0 if nothing was inserted).
        virtual Money accept() = 0;
    };
}