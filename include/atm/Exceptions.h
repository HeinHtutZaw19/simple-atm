#pragma once

#include <stdexcept>
#include <string>

namespace atm
{
    class ATMError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class InvalidPinError : public ATMError
    {
    public:
        int attemptsRemaining;
        explicit InvalidPinError(int attemptsRemaining)
            : ATMError("Incorrect Pin. " + std::to_string(attemptsRemaining) + "attempts remaining."),
              attemptsRemaining(attemptsRemaining) {}
    };

    class CardBlockedError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
    class InvalidStateError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
    class InvalidAmountError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
    class InsufficientFundsError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
    class CashDispenseError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
    class AccountNotFoundError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
    class InsufficientCashError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
    class CashBinError : public ATMError
    {
    public:
        using ATMError::ATMError;
    };
}