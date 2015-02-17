#include "gtest/gtest.h"

#include "../src/base/lib_common2.hpp"
#include "../src/base/lib_common3.hpp"

#include "../src/base/useot.hpp"

#include "../src/base/cmd.hpp"
using namespace nOT::nUtils;

class cUseOtVoucherTest: public testing::Test {
protected:
	std::shared_ptr<nOT::nUse::cUseOT> useOt;
	std::string nym1;
	std::string acc1;
	std::string toNym;
	std::string toAcc;
	std::string fromAcc;
	std::string fromNym;
	int32_t amount;
	int32_t amount2;

	virtual void SetUp() {
		nym1 = "Trader Bob";
		acc1 = "Bob's Silver";

		toNym = "FT's Test Nym";
		toAcc = "FT's Tokens";

		fromNym = "Trader Bob";
		fromAcc = "Bob's Tokens";



		amount = 3;
		amount2 = 5;
		useOt = std::make_shared<nOT::nUse::cUseOT>("test");

		_note("amount1 " << amount);
		_note("amount2 " << amount2);
	}

	virtual void TearDown() {
		cout << "END" << endl;
	}
};

TEST_F(cUseOtVoucherTest, VoucherCreate) {
	EXPECT_FALSE(useOt->VoucherWithdraw("bitcoins", toNym, -20, "some memo", 0));

	const auto balance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(acc1));

	EXPECT_FALSE(useOt->VoucherWithdraw(acc1, toNym, balance + 1, "memo", false));

	sleep(3);
	EXPECT_TRUE(useOt->VoucherWithdraw(acc1, toNym, amount, "memo", false));
	EXPECT_TRUE(useOt->OutpaymentShow(nym1, 0, false));

	sleep(10);
	EXPECT_EQ(balance - amount, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(acc1)));
}

TEST_F(cUseOtVoucherTest, VoucherCancel) {
	auto accID = useOt->AccountGetId(acc1);
	auto currentBallance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(accID);

	ASSERT_TRUE(useOt->VoucherCancel(acc1, nym1, 0, false));
	EXPECT_TRUE(useOt->AccountInAccept(acc1, 0, false, false));

	useOt->NymRefresh(acc1, true, false);
	useOt->AccountRefresh(acc1, true, false);
	EXPECT_EQ(currentBallance + amount, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(accID));
}

TEST_F(cUseOtVoucherTest, Create) {
	const auto startBalance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(fromAcc));

	// create
	ASSERT_TRUE(useOt->VoucherWithdraw(fromAcc, toNym, amount2, "", false));
	sleep(10);
	_info("after sleeping");
	EXPECT_EQ(startBalance - amount2, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(fromAcc)));
	EXPECT_TRUE(useOt->OutpaymentShow(fromNym, 0, false));
}

TEST_F(cUseOtVoucherTest, Send) {
	// send
	ASSERT_TRUE(useOt->PaymentSend(fromNym, toNym, 0, false));

	// refresh
	useOt->NymRefresh(toNym, false, false);
	useOt->AccountRefresh(toAcc, false, false);
}

TEST_F(cUseOtVoucherTest, Accept) {
	const auto toNymBalance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(toAcc));

	// accept
	sleep(3);
	EXPECT_TRUE(useOt->PaymentAccept(toAcc, -1, false));

	sleep(10);
	EXPECT_EQ(toNymBalance + amount2, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(toAcc)));
}
/*
 TEST_F(cUseOtVoucherTest, cmd) {
 shared_ptr<nOT::nNewcli::cCmdParser> parser(new nOT::nNewcli::cCmdParser);
 parser->Init();
 auto tmp = parser->StartProcessing("ot account ls2", useOt);

 tmp.UseExecute();
 }*/
