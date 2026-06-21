#pragma once

#include <stdexcept>
#include <string>

namespace atm
{
    /// Base class for every error the ATM domain raises. Catch this to handle
    /// any ATM failure generically; catch a subclass for a specific case.
    class ATMError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /// The PIN was wrong but attempts remain. attemptsRemaining reports how
    /// many tries are left before the card is blocked. The session stays in the
    /// authenticating state so the customer can retry.
    class InvalidPinError : public ATMError
    {
    public:
        int attemptsRemaining;
        explicit InvalidPinError(int attemptsRemaining)
            : ATMError("Incorrect Pin. " + std::to_string(attemptsRemaining) + "attempts remaining."),
              attemptsRemaining(attemptsRemaining) {}
    };

    /// The allowed number of PIN attempts was exhausted. The session is reset
    /// and the card is no longer usable for this session.
    class CardBlockedError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };

    /// An operation was requested in a state where it is not permitted (for
    /// example withdrawing before an account has been selected).
    class InvalidStateError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };

    /// A transaction amount was invalid (non-positive, or a negative deposit
    /// reported by the cash bin).
    class InvalidAmountError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };

    /// The account lacks sufficient funds for the requested withdrawal.
    class InsufficientFundsError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };

    /// The dispenser failed to hand out cash. Reserved for hardware-level
    /// dispense faults surfaced to callers.
    class CashDispenseError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };

    /// The requested account is not linked to the inserted card.
    class AccountNotFoundError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };

    /// The cash bin does not currently hold enough notes to dispense the
    /// requested amount, even though the account could cover it.
    class InsufficientCashError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };

    /// A general cash-bin / hardware error not covered by a more specific type.
    class CashBinError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
}