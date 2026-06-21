#include "atm/ATMController.h"
#include "atm/Exceptions.h"
#include <algorithm>
using namespace std;

namespace atm
{
	namespace
	{
		string stateName(State s)
		{
			switch (s)
			{
			case State::Idle:
				return "Idle";
			case State::Authenticating:
				return "Authenticating";
			case State::SelectingAccount:
				return "SelectingAccount";
			case State::Transacting:
				return "Transacting";
			}
			return "Unknown";
		}
	}

	ATMController::ATMController(IBankService &bank, ICashBin &cashBin, int maxPinAttempts)
		 : bank_(bank), cashBin_(cashBin), maxPinAttempts_(maxPinAttempts)
	{
		reset();
	}

	void ATMController::insertCard(const std::string &cardNumber)
	{
		requireState({State::Idle});
		if (cardNumber.empty())
		{
			throw ATMError("A card is required");
		}
		cardNumber_ = cardNumber;
		pinAttempts_ = 0;
		state_ = State::Authenticating;
	}

	std::vector<std::string> ATMController::enterPin(const std::string &pin)
	{
		requireState({State::Authenticating});
		if (bank_.validatePin(cardNumber_, pin))
		{
			accounts_ = bank_.getAccounts(cardNumber_);
			state_ = State::SelectingAccount;
			return accounts_;
		}
		++pinAttempts_;
		const int attemptsRemaining = maxPinAttempts_ - pinAttempts_;
		if (attemptsRemaining <= 0)
		{
			reset();
			throw CardBlockedError(
				 "Too many incorrect PIN attempts. Card retained/ ejected");
		}
		throw InvalidPinError(attemptsRemaining);
	}

	std::vector<std::string> ATMController::listAccounts() const
	{
		requireState({State::SelectingAccount, State::Transacting});
		return accounts_;
	}

	void ATMController::selectAccount(const std::string &accountId)
	{
		requireState({State::SelectingAccount, State::Transacting});
		if (std::find(accounts_.begin(), accounts_.end(), accountId) == accounts_.end())
		{
			throw AccountNotFoundError("Account " + accountId + " is not linked to this card");
		}
		selectedAccount_ = accountId;
		state_ = State::Transacting;
	}

	Money ATMController::checkBalance()
	{
		requireState({State::Transacting});
		return bank_.getBalance(requireAccount());
	}

	Money ATMController::deposit()
	{
		requireState({State::Transacting});
		const std::string &account = requireAccount();
		const Money amount = cashBin_.accept();
		if (amount < 0)
		{
			throw InvalidAmountError("Cash bin reported a negative amount");
		}
		if (amount == 0)
		{
			return bank_.getBalance(account);
		}
		return bank_.deposit(account, amount);
	}

	Money ATMController::withdraw(Money amount)
	{
		requireState({State::Transacting});
		const std::string &account = requireAccount();
		if (amount <= 0)
		{
			throw InvalidAmountError("Amount must be a positive number of dollars.");
		}
		if (!cashBin_.canDispense(amount))
		{
			throw InsufficientCashError("The ATM cannot dispense that amount right now.");
		}
		const Money newBalance = bank_.withdraw(account, amount);
		try
		{
			cashBin_.dispense(amount);
		}
		catch (...)
		{
			bank_.deposit(account, amount);
			throw;
		}
		return newBalance;
	}

	void ATMController::ejectCard()
	{
		reset();
	}

	void ATMController::requireState(std::initializer_list<State> allowed) const
	{
		if (std::find(allowed.begin(), allowed.end(), state_) == allowed.end())
		{
			std::string allowedNames;
			for (State s : allowed)
			{
				if (!allowedNames.empty())
				{
					allowedNames += ", ";
				}
				allowedNames += stateName(s);
			}
			throw InvalidStateError("Operation not allowed in state " + stateName(state_) + "; requires one of: " + allowedNames + ".");
		}
	}

	const std::string &ATMController::requireAccount() const
	{
		if (!selectedAccount_.has_value())
		{
			throw InvalidStateError("No account selected.");
		}
		return *selectedAccount_;
	}

	void ATMController::reset()
	{
		state_ = State::Idle;
		cardNumber_.clear();
		pinAttempts_ = 0;
		accounts_.clear();
		selectedAccount_.reset();
	}
}