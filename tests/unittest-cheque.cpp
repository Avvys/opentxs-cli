#include "gtest/gtest.h"

#include "../src/base/lib_common2.hpp"
#include "../src/base/lib_common3.hpp"

#include "../src/base/useot.hpp"

using namespace nOT::nUtils;

class cUseOtChequeTest: public testing::Test {
protected:
	std::shared_ptr<nOT::nUse::cUseOT> useOt;

	string fromAcc;
	string fromNym;
	string toNym;
	string toAcc;
	string server;
	int64_t amount;

	virtual void SetUp() {
		useOt = std::make_shared<nOT::nUse::cUseOT>("test-cheque");

		fromAcc = "FT's Tokens";
		fromNym = "FT's Test Nym";
		toNym = "Trader Bob";
		toAcc = "Bob's Tokens";
		server = "Transactions.com";
		amount = 2;

		cout << "cheque test" << endl;
	}

	virtual void TearDown() {
		cout << "tear down" << endl;
	}
};



TEST_F(cUseOtChequeTest, CreateCheque) {
	auto result = useOt->ChequeCreate(fromAcc, toNym, amount, server, "test cheque", false);
	EXPECT_TRUE(result);
}
/*
TEST_F(cUseOtChequeTest, VoucherCancel) {
	EXPECT_FALSE(useOt->VoucherCancel(fromAcc, fromNym, 0, false));

}*/

TEST_F(cUseOtChequeTest, SendCheque) {
	//EXPECT_FALSE(useOt->PaymentSend("Alice", toNym, 0, false));
	auto result = useOt->PaymentSend(fromNym, toNym, 0, false);
	useOt->NymRefresh(toNym, true, false);
	useOt->AccountRefresh(fromAcc, true, false);
	ASSERT_TRUE(result);
}

TEST_F(cUseOtChequeTest, DepositCheque) {
	const auto toNymBalance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(toAcc));
	const auto fromNymBalance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(fromAcc));

	ASSERT_TRUE(useOt->PaymentAccept(toAcc, -1, false));
//	ASSERT_TRUE(useOt->PaymentAccept(toAcc, -1, false));

	sleep(15);
	useOt->NymRefresh(toNym, true, false);
	useOt->AccountRefresh(toAcc, true, false);

	sleep(15);
	EXPECT_EQ(toNymBalance + amount, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(toAcc)));
	EXPECT_EQ(fromNymBalance - amount, opentxs::OTAPI_Wrap::GetAccountWallet_Balance(useOt->AccountGetId(fromAcc)));

}

TEST_F(cUseOtChequeTest, CreateAndDiscard) {
	auto create = useOt->ChequeCreate(fromAcc, toNym, amount, server, "to discard", false);
	ASSERT_TRUE(create);

	auto discard = useOt->ChequeDiscard(fromAcc, fromNym, 0, false);
	EXPECT_TRUE(discard);

	useOt->NymRefresh(toNym, true, false);

	auto send = useOt->PaymentSend(fromNym, toNym, 0, false);
	ASSERT_TRUE(send);

	_dbg1(send);
	useOt->NymRefresh(toNym, true, false);

	ASSERT_TRUE(useOt->PaymentAccept(toAcc, -1, false));

}
/*
TEST_F(cUseOtChequeTest, BasketNew) {

	useOt->BasketNew();
}

TEST_F(cUseOtChequeTest, BasketDisplay) {
	using namespace opentxs;
	auto tmp = useOt->Init();

	auto assetCount = OTAPI_Wrap::GetAssetTypeCount();
	_dbg2("asset count: " << assetCount);

	int32_t index = 0;
	bool ok = false;

	for (int i = 0; i < assetCount; ++i) {
		const auto assetID = OTAPI_Wrap::GetAssetType_ID(i);
		const auto asset = useOt->AssetGetName(assetID);

		_dbg2("asset: " << asset);

		auto isb = OTAPI_Wrap::IsBasketCurrency(assetID);
		_dbg2(isb);
		ok = ok || isb;
	}
	EXPECT_TRUE(ok);
}*/

