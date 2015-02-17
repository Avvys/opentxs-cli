#include "gtest/gtest.h"

#include "../src/base/lib_common2.hpp"
#include "../src/base/lib_common3.hpp"

#include "../src/base/useot.hpp"
#include "../src/base/cmd.hpp"

using namespace nOT::nUtils;

class cUseOtOutpaymentTest: public testing::Test {
protected:
	std::shared_ptr<nOT::nUse::cUseOT> useOt;
	string nym1;
	string nym2;

	string preCmd;

	virtual void SetUp() {
		useOt = std::make_shared<nOT::nUse::cUseOT>("outpayment-cheque");
		nym1 = "FT's Test Nym";
		nym2 = "Trader Bob";
		preCmd = "ot outpayment ";

		cout << "outpayment test" << endl;
	}

	virtual void TearDown() {
		cout << "tear down" << endl;
	}
};

TEST_F(cUseOtOutpaymentTest, cmdDisplayOk) {
	shared_ptr<nOT::nNewcli::cCmdParser> parser(new nOT::nNewcli::cCmdParser);
	parser->Init();
	parser->StartProcessing("ot outpayment ls", useOt).UseExecute();
}

TEST_F(cUseOtOutpaymentTest, cmdShowFail) {
	shared_ptr<nOT::nNewcli::cCmdParser> parser(new nOT::nNewcli::cCmdParser);
	parser->Init();

	string cmd = preCmd + "show " + nym1 + " -1";
	_fact(cmd);
	EXPECT_ANY_THROW(parser->StartProcessing(cmd, useOt).UseExecute());

	cmd = preCmd + "show " + nym1 + " 1000";
	_fact(cmd);
	EXPECT_ANY_THROW(parser->StartProcessing(cmd, useOt).UseExecute());

}
/*
TEST_F(cUseOtOutpaymentTest, removeAllCmd) {
	shared_ptr<nOT::nNewcli::cCmdParser> parser(new nOT::nNewcli::cCmdParser);
	parser->Init();
	useOt->Init();

	const string cmdOk = preCmd + "rm \"" + nym1 + "\" --all --dryrun";
	parser->StartProcessing(cmdOk, useOt).UseExecute() ;
}*/
/*
TEST_F(cUseOtOutpaymentTest, removeAll) {

	EXPECT_TRUE(useOt->OutpaymentRemove(nym1, 0, true, false));
	EXPECT_FALSE(useOt->OutpaymentShow(nym1, 0, false));

	useOt->OutpaymentDisplay(nym1, false);

}*/

