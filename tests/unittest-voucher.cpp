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
	std::string toNym;
	std::string toAcc;
	std::string fromAcc;
	std::string fromNym;
	int32_t amount;
	int32_t amount2;

	virtual void SetUp() {
		nym1 = "trader bob";

		toNym = "fellowtraveler";
		toAcc = "FT's dollars";

		fromNym = "trader bob";
		fromAcc = "Bob's dollars";

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

	auto fromAccID = useOt->AccountGetId(fromAcc);
	const auto balance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(fromAccID);

	EXPECT_FALSE(useOt->VoucherWithdraw(fromAcc, toNym, balance + 1, "memo", false));

	sleep(3);
	EXPECT_TRUE(useOt->VoucherWithdraw(fromAcc, toNym, amount, "memo", false));
	EXPECT_TRUE(useOt->OutpaymentShow(fromNym, 0, false));

	sleep(120);
	EXPECT_EQ(balance - amount, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(fromAccID));
}

TEST_F(cUseOtVoucherTest, VoucherCancel) {
	auto accID = useOt->AccountGetId(fromAcc);
	auto currentBallance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(accID);

	ASSERT_TRUE(useOt->VoucherCancel(fromAcc, fromNym, 0, false));
	EXPECT_TRUE(useOt->AccountInAccept(fromAcc, 0, false, false));

	useOt->NymRefresh(fromNym, true, false);
	useOt->AccountRefresh(fromAcc, true, false);
	sleep(15);
	EXPECT_EQ(currentBallance + amount, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(accID));
}

TEST_F(cUseOtVoucherTest, Create) {
	const auto startBalance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(fromAcc));

	// create
	ASSERT_TRUE(useOt->VoucherWithdraw(fromAcc, toNym, amount2, "", false));
	sleep(15);
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
	sleep(5);
	EXPECT_TRUE(useOt->PaymentAccept(toAcc, -1, false));

	sleep(30);
	EXPECT_EQ(toNymBalance + amount2, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(toAcc)));
}

TEST_F(cUseOtVoucherTest, OutpaymentRemove) {
	EXPECT_TRUE(useOt->OutpaymentRemove(fromNym, 0, true, false));
	EXPECT_FALSE(useOt->OutpaymentShow(fromNym, 0, false));

	useOt->OutpaymentDisplay(nym1, false);
}

TEST_F(cUseOtVoucherTest, test) {
//	 	bool VoucherWithdraw(const ID & fromAccID, const ID & fromNymID, const ID & toNymID, const ID & assetID, const ID & srvID, int64_t amount,
//	string memo, bool send);
	useOt->Init();

	auto fromAccID = useOt->AccountGetId("");
	auto fromNymID = useOt->NymGetId("");
	auto toNymID = "";
	auto assetID = useOt->AssetGetId("Bitcoins");
	auto srvID = useOt->ServerGetId("Transactions.com");
	int64_t amount = 100;

	auto result = useOt->VoucherWithdraw(fromAccID, fromNymID, toNymID, assetID, srvID, amount, "test", true);


}

TEST_F(cUseOtVoucherTest, test2) {
	useOt->Init();

	string accID = "";
	string nymID = "";
	string srvID = "";
	string filename = "/tmp/OT-VOUCHER";
	string instrument;


	nOT::nUtils::cEnvUtils envUtils;

	if(!filename.empty()) {
			_dbg3("Loading from file: " << filename);
			instrument = envUtils.ReadFromFile(filename);
			_dbg3("Loaded: " << instrument);
		}

		if ( instrument.empty()) instrument = envUtils.Compose();
		if( instrument.empty() ) {
			_warn("Can't import, empty input");
		}


	auto dep = opentxs::OTAPI_Wrap::depositCheque(srvID, nymID, accID, instrument);
}

