#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <vector>
#include "atm/Ports.h"

namespace atm
{
    /// Default number of incorrect PIN entries allowed before the card is blocked.
    constexpr int kDefaultMaxPinAttempts = 3;

    /// Stage of the customer session. The controller is a state machine that
    /// advances Idle -> Authenticating -> SelectingAccount -> Transacting and
    /// returns to Idle when the card is ejected or the session is reset.
    enum class State
    {
        Idle,             ///< No card inserted; waiting for a customer.
        Authenticating,   ///< Card inserted; awaiting a valid PIN.
        SelectingAccount, ///< PIN accepted; awaiting account selection.
        Transacting,      ///< An account is selected; transactions allowed.
    };

    /// Orchestrates a single ATM customer session as a state machine.
    ///
    /// The controller holds no persistent account data: it drives an
    /// IBankService for authentication and balances and an ICashBin for cash
    /// movement. Each public operation is only valid in particular states and
    /// throws an ATMError subclass (see Exceptions.h) when called out of turn or
    /// when the underlying service reports a problem. The controller is not
    /// thread-safe; a single instance models one ATM serving one customer at a
    /// time.
    class ATMController
    {
    public:
        /// Constructs a controller bound to the given services. References must
        /// outlive the controller. maxPinAttempts caps incorrect PIN entries
        /// before the card is blocked. Starts in the Idle state.
        ATMController(IBankService &bank, ICashBin &cashBin, int maxPinAttempts = kDefaultMaxPinAttempts);

        /// Current session state.
        State state() const { return state_; }

        /// The account selected for transacting, or empty if none is selected yet.
        const std::optional<std::string> &selectedAccount() const
        {
            return selectedAccount_;
        }

        /// Begins a session for the given card. Valid only in Idle; advances to
        /// Authenticating. Throws InvalidStateError if a session is already in
        /// progress, or ATMError if the card number is empty.
        void insertCard(const std::string &cardNumber);

        /// Submits a PIN. Valid only in Authenticating. On success advances to
        /// SelectingAccount and returns the accounts linked to the card. On
        /// failure throws InvalidPinError while attempts remain, or
        /// CardBlockedError once they are exhausted (which also resets the
        /// session).
        std::vector<std::string> enterPin(const std::string &pin);

        /// Returns the accounts linked to the card. Valid in SelectingAccount or
        /// Transacting; throws InvalidStateError otherwise.
        std::vector<std::string> listAccounts() const;

        /// Chooses the account to transact on. Valid in SelectingAccount or
        /// Transacting (to switch accounts); advances to Transacting. Throws
        /// AccountNotFoundError if the account is not linked to the card.
        void selectAccount(const std::string &accountId);

        /// Returns the balance of the selected account. Valid only in
        /// Transacting.
        Money checkBalance();

        /// Accepts cash from the bin and credits it to the selected account,
        /// returning the new balance. A zero deposit is a no-op that returns the
        /// current balance. Valid only in Transacting; throws InvalidAmountError
        /// if the bin reports a negative amount.
        Money deposit();

        /// Withdraws amount from the selected account and dispenses it,
        /// returning the new balance. Valid only in Transacting. Throws
        /// InvalidAmountError for non-positive amounts and InsufficientCashError
        /// when the bin cannot dispense the amount. The account is debited only
        /// after a successful dispense: if the hardware throws, the debit is
        /// rolled back and the original error is rethrown.
        Money withdraw(Money amount);

        /// Ends the session and returns to Idle, clearing all card and account
        /// state. Safe to call in any state.
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