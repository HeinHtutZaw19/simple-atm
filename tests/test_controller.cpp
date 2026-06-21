#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "atm/ATMController.h"
#include "atm/Exceptions.h"
#include "Fakes.h"

using namespace atm;

namespace {

// Builds a controller wired to fakes with one card ("card-1", PIN "1234")
// linked to two accounts: "checking" ($100) and "savings" ($30).
// The cash bin starts loaded with `binCash` dollars.
struct Fixture {
    FakeBank bank;
    FakeCashBin bin;
    ATMController atm;

    explicit Fixture(Money binCash = 1000)
        : bin(binCash), atm(bank, bin) {
        bank.addCard("card-1", "1234", {"checking", "savings"});
        bank.setBalance("checking", 100);
        bank.setBalance("savings", 30);
    }

    // Drive the controller to the Transacting state on "checking".
    void authenticateToChecking() {
        atm.insertCard("card-1");
        atm.enterPin("1234");
        atm.selectAccount("checking");
    }
};

}  // namespace

TEST_CASE("a fresh ATM is idle") {
    Fixture f;
    CHECK(f.atm.state() == State::Idle);
}

TEST_CASE("happy path: insert card -> PIN -> select account -> balance") {
    Fixture f;

    f.atm.insertCard("card-1");
    CHECK(f.atm.state() == State::Authenticating);

    auto accounts = f.atm.enterPin("1234");
    CHECK(f.atm.state() == State::SelectingAccount);
    REQUIRE(accounts.size() == 2);
    CHECK(accounts[0] == "checking");
    CHECK(accounts[1] == "savings");

    f.atm.selectAccount("checking");
    CHECK(f.atm.state() == State::Transacting);
    CHECK(f.atm.selectedAccount().value() == "checking");

    CHECK(f.atm.checkBalance() == 100);
}

// ---------------------------------------------------------------------------
// State-machine guards
// ---------------------------------------------------------------------------
TEST_CASE("cannot enter a PIN before a card is inserted") {
    Fixture f;
    CHECK_THROWS_AS(f.atm.enterPin("1234"), InvalidStateError);
}

TEST_CASE("cannot transact before an account is selected") {
    Fixture f;
    f.atm.insertCard("card-1");
    f.atm.enterPin("1234");  // now SelectingAccount
    CHECK_THROWS_AS(f.atm.checkBalance(), InvalidStateError);
    CHECK_THROWS_AS(f.atm.withdraw(10), InvalidStateError);
    CHECK_THROWS_AS(f.atm.deposit(), InvalidStateError);
}

TEST_CASE("cannot insert a card while a session is already active") {
    Fixture f;
    f.atm.insertCard("card-1");
    CHECK_THROWS_AS(f.atm.insertCard("card-2"), InvalidStateError);
}

TEST_CASE("an empty card number is rejected") {
    Fixture f;
    CHECK_THROWS_AS(f.atm.insertCard(""), ATMError);
}

// ---------------------------------------------------------------------------
// PIN handling
// ---------------------------------------------------------------------------
TEST_CASE("wrong PIN reports remaining attempts and keeps the card") {
    Fixture f;
    f.atm.insertCard("card-1");

    try {
        f.atm.enterPin("0000");
        FAIL("expected InvalidPinError");
    } catch (const InvalidPinError& e) {
        CHECK(e.attemptsRemaining == 2);
    }
    // Still authenticating; the customer can try again.
    CHECK(f.atm.state() == State::Authenticating);
}

TEST_CASE("a correct PIN after a wrong one still works") {
    Fixture f;
    f.atm.insertCard("card-1");
    CHECK_THROWS_AS(f.atm.enterPin("0000"), InvalidPinError);
    auto accounts = f.atm.enterPin("1234");
    CHECK(accounts.size() == 2);
    CHECK(f.atm.state() == State::SelectingAccount);
}

TEST_CASE("exhausting PIN attempts blocks the card and resets the session") {
    Fixture f;
    f.atm.insertCard("card-1");
    CHECK_THROWS_AS(f.atm.enterPin("x"), InvalidPinError);  // 2 left
    CHECK_THROWS_AS(f.atm.enterPin("x"), InvalidPinError);  // 1 left
    CHECK_THROWS_AS(f.atm.enterPin("x"), CardBlockedError); // 0 left -> blocked
    CHECK(f.atm.state() == State::Idle);
}

TEST_CASE("the controller only ever asks the bank to verify the PIN") {
    // The PIN never lives in the controller; it is handed straight to the bank.
    Fixture f;
    f.atm.insertCard("card-1");
    CHECK(f.bank.validatePinCalls == 0);
    f.atm.enterPin("1234");
    CHECK(f.bank.validatePinCalls == 1);
}

// ---------------------------------------------------------------------------
// Account selection
// ---------------------------------------------------------------------------
TEST_CASE("selecting an unknown account is rejected") {
    Fixture f;
    f.atm.insertCard("card-1");
    f.atm.enterPin("1234");
    CHECK_THROWS_AS(f.atm.selectAccount("not-mine"), AccountNotFoundError);
}

TEST_CASE("the customer can switch accounts mid-session") {
    Fixture f;
    f.authenticateToChecking();
    CHECK(f.atm.checkBalance() == 100);
    f.atm.selectAccount("savings");
    CHECK(f.atm.checkBalance() == 30);
}

// ---------------------------------------------------------------------------
// Deposit
// ---------------------------------------------------------------------------
TEST_CASE("deposit credits the bank and lands the cash in the bin") {
    Fixture f(/*binCash=*/0);
    f.authenticateToChecking();

    f.bin.pendingDeposit = 40;  // the customer will insert $40
    Money newBalance = f.atm.deposit();

    CHECK(newBalance == 140);
    CHECK(f.atm.checkBalance() == 140);
    CHECK(f.bin.cash == 40);    // the deposited cash is now in the machine
}

TEST_CASE("depositing nothing is a harmless no-op") {
    Fixture f;
    f.authenticateToChecking();
    f.bin.pendingDeposit = 0;
    CHECK(f.atm.deposit() == 100);
}

// ---------------------------------------------------------------------------
// Withdraw
// ---------------------------------------------------------------------------
TEST_CASE("withdraw debits the bank and dispenses cash") {
    Fixture f(/*binCash=*/1000);
    f.authenticateToChecking();

    Money newBalance = f.atm.withdraw(60);

    CHECK(newBalance == 40);
    CHECK(f.atm.checkBalance() == 40);
    CHECK(f.bin.cash == 940);       // $60 left the machine
    CHECK(f.bin.dispenseCalls == 1);
}

TEST_CASE("withdrawing more than the balance fails and changes nothing") {
    Fixture f(/*binCash=*/1000);
    f.authenticateToChecking();

    CHECK_THROWS_AS(f.atm.withdraw(500), InsufficientFundsError);
    CHECK(f.atm.checkBalance() == 100);  // untouched
    CHECK(f.bin.cash == 1000);           // no cash dispensed
    CHECK(f.bin.dispenseCalls == 0);
}

TEST_CASE("withdrawing more than the bin holds fails before touching the bank") {
    Fixture f(/*binCash=*/20);  // bin only has $20
    f.authenticateToChecking();

    CHECK_THROWS_AS(f.atm.withdraw(50), InsufficientCashError);
    CHECK(f.atm.checkBalance() == 100);  // bank was never debited
    CHECK(f.bin.dispenseCalls == 0);
}

TEST_CASE("non-positive withdrawal amounts are rejected") {
    Fixture f;
    f.authenticateToChecking();
    CHECK_THROWS_AS(f.atm.withdraw(0), InvalidAmountError);
    CHECK_THROWS_AS(f.atm.withdraw(-5), InvalidAmountError);
}

TEST_CASE("a dispense failure rolls back the bank debit (no money lost)") {
    Fixture f(/*binCash=*/1000);
    f.authenticateToChecking();
    f.bin.dispenseShouldFail = true;  // hardware jams mid-withdrawal

    CHECK_THROWS_AS(f.atm.withdraw(60), CashBinError);

    // The customer got no cash, so they must not be charged.
    CHECK(f.atm.checkBalance() == 100);
}

// ---------------------------------------------------------------------------
// Ending the session
// ---------------------------------------------------------------------------
TEST_CASE("ejecting the card resets the session") {
    Fixture f;
    f.authenticateToChecking();
    f.atm.ejectCard();

    CHECK(f.atm.state() == State::Idle);
    CHECK_FALSE(f.atm.selectedAccount().has_value());
    // A new session can begin cleanly.
    f.atm.insertCard("card-1");
    CHECK(f.atm.state() == State::Authenticating);
}

TEST_CASE("a custom PIN-attempt limit is honoured") {
    FakeBank bank;
    FakeCashBin bin(100);
    bank.addCard("c", "1", {"a"});
    ATMController atm(bank, bin, /*maxPinAttempts=*/1);

    atm.insertCard("c");
    CHECK_THROWS_AS(atm.enterPin("wrong"), CardBlockedError);  // blocked at once
    CHECK(atm.state() == State::Idle);
}
