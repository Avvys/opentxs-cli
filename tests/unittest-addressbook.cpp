#include "gtest/gtest.h"

#include "../src/base/lib_common2.hpp"
#include "../src/base/lib_common3.hpp"

#include "../src/base/useot.hpp"
#include <typeinfo>

#include "../src/base/addressbook.hpp"

#include <ctime>
#include <cstdlib>

using namespace nOT::nUtils;
using namespace nOT;

class cUseOtAddressBookTest: public testing::Test {
protected:
	std::shared_ptr<nOT::nUse::cUseOT> useOt;
	string preCmd;
	string toNym;
	string toAcc;
	string server;
	string nym;
	string nymID;

	enum color {
		red, blue
	};

	virtual void SetUp() {
		useOt = std::make_shared<nOT::nUse::cUseOT>("test-record");
		preCmd = "ot record ";
		toNym = "trader bob";
		toAcc = "Bob's bitcoin";
		server = "Transactions.com";
		nym = "fellowtraveler";

		useOt->Init();

		nymID = useOt->NymGetId(nym);
	}

	virtual void TearDown() {
		cout << "tear down" << endl;
	}
};

TEST_F(cUseOtAddressBookTest, Operation) {
	useOt->Init();

	const string nym2 = "issuer";
	const auto nymID2 = useOt->NymGetId(nym2);

	auto addressbook = nOT::AddressBookStorage::Get(nymID);
	addressbook->removeAll();
	EXPECT_EQ(0, addressbook->getCount());
	addressbook->display();

	EXPECT_TRUE(addressbook->add(toNym, useOt->NymGetId(toNym)));
	EXPECT_TRUE(addressbook->nymExist(useOt->NymGetId(toNym)));
	EXPECT_FALSE(addressbook->add(toNym, useOt->NymGetId(toNym)));

	addressbook->display();

	EXPECT_TRUE(addressbook->remove(useOt->NymGetId(toNym)));
	EXPECT_FALSE(addressbook->nymExist(useOt->NymGetId(toNym)));
	EXPECT_FALSE(addressbook->remove(useOt->NymGetId(toNym)));

	addressbook->display();

	EXPECT_FALSE(useOt->AddressBookAdd(toNym, nym, useOt->NymGetName(nym), false));
}

TEST_F(cUseOtAddressBookTest, Display) {
	auto addressbook = nOT::AddressBookStorage::Get(nymID);
	addressbook->display();

}

TEST_F(cUseOtAddressBookTest, ClearObject) {
	for (auto nym : useOt->NymGetAllNames()) {
		EXPECT_TRUE(useOt->AddressBookDisplay(nym, false));
	}

	sleep(1);

	for (auto nym : useOt->NymGetAllNames()) {
		EXPECT_TRUE(useOt->AddressBookDisplay(nym, false));
	}
	nOT::AddressBookStorage::ForceClear();
}

TEST_F(cUseOtAddressBookTest, AddAndFind) {
	const string toAddNymName = "alice2";
	const string toAddNymID = "otxVhEonf15M4oN4gt8WSz2VrPV8tDFVxymr";
	const string ownerNymName = nym;
	const string ownerNymID = nymID;

	EXPECT_FALSE(ownerNymID.empty());
	auto addressBook = nOT::AddressBookStorage::Get(ownerNymID);

	addressBook->removeAll();
	EXPECT_EQ(0, addressBook->getCount());

	auto checkExist = [&](bool shouldExist)-> void {
		_info("check exist");
		auto exist = addressBook->nymExist(toAddNymID);
		if(shouldExist) {
			EXPECT_EQ(toAddNymName, addressBook->nymGetName(toAddNymID));
			EXPECT_EQ(toAddNymID, addressBook->nymGetID(toAddNymName));

			EXPECT_TRUE(addressBook->nymExist(toAddNymID));
			EXPECT_TRUE(addressBook->nymNameExist(toAddNymName));

		} else {
			EXPECT_TRUE(addressBook->nymGetName(toAddNymID).empty());
			EXPECT_TRUE(addressBook->nymGetID(toAddNymName).empty());
			EXPECT_FALSE(addressBook->nymExist(toAddNymID));
			EXPECT_FALSE(addressBook->nymNameExist(toAddNymName));
		}
	};

	checkExist(false);
	auto add = addressBook->add(toAddNymName, toAddNymID);
	addressBook->display();
	EXPECT_TRUE(add);

	checkExist(true);
	auto remove = addressBook->remove(toAddNymID);
	EXPECT_TRUE(remove);
	addressBook->display();
	checkExist(false);
}

TEST_F(cUseOtAddressBookTest, Completiton) {
	map <string, string> testNyms {
		{"otxMDWoLhgB6QQJbX57hRc5LYTfLjB6xqjdV", "Betty"},
		{"otxMFse8XYfhYhZdVGzjkAcFmrY99zBr46DG", "Ted"},
		{"otxBQJju43UmSPqWSQa68CN3CmKXkUSzAKVH", "Wilma"},
		{"otxKGnzA76J2Tx9ArEHd4eiPixGWBPoaqEWx", "Barney"},
		{"otxWdYvAHe7i4qE2th6Vc4pzkYMifT3Y4QNi", "Fred"}
	};

	const string ownerNym = "issuer";
	auto addressBook = nOT::AddressBookStorage::Get(useOt->NymGetId(ownerNym));

	_mark("adding nyms to address book");
	for(auto nym :testNyms) {
		EXPECT_TRUE(addressBook->add(nym.second, nym.first));
	}

	_mark("testing useOt function");
	for(auto nym : testNyms) {
		_fact(nym.second);
		EXPECT_EQ(nym.first, useOt->NymGetToNymId(nym.second, useOt->NymGetId(ownerNym)));
		EXPECT_EQ(nym.second, useOt->NymGetName(nym.first));
	}

	_mark("removing nyms from address book");
	for(auto nym : testNyms) {
		EXPECT_TRUE(addressBook->remove(nym.first));
	}
}

TEST_F(cUseOtAddressBookTest, Cache) {

	auto names = AddressBookStorage::GetAllNames(useOt->NymGetAllIDs());
	auto names2 = AddressBookStorage::GetAllNames(useOt->NymGetAllIDs());
}

TEST_F(cUseOtAddressBookTest, NymNameExist) {
	auto addressBook = AddressBookStorage::Get(useOt->NymGetId(toNym));
	auto toAdd = std::make_pair("otxMDWoLhgB6QQJbX57hRc5LYTfLjB6xqjdV", "Betty");
	addressBook->add(toAdd.second, toAdd.first);
	addressBook->display();
	EXPECT_FALSE(useOt->CheckIfExists(nUtils::eSubjectType::User, toAdd.second));
	EXPECT_TRUE(AddressBookStorage::NymNameExist(toAdd.second, useOt->NymGetAllIDs()));

	AddressBookStorage::ForceClear();

	EXPECT_TRUE(useOt->NymNameExist(toAdd.second));

	EXPECT_TRUE(addressBook->remove(toAdd.first));

	EXPECT_FALSE(useOt->NymNameExist(toAdd.second));

}

TEST_F(cUseOtAddressBookTest, ExportImport) {
	srand(time(NULL));
	const string exampleNym = "alice_";
	const string exampleNymID = "otxNhHReyvasXyD9rLnRFpp6SYPXBJ5R9oS7";
	const string filename = "export_" + exampleNym + "_" + std::to_string(rand()%999+1000) ;
	const string filename_wrong = "export_" + exampleNym + "_" + std::to_string(rand()%999+1000) + "_wrong" ;

	_mark("exporting to: " << filename);
	auto addressbook = nOT::AddressBookStorage::Get(nymID);
	ASSERT_TRUE(addressbook->nymExport(exampleNym, exampleNymID, filename));

	ASSERT_TRUE(opentxs::OTPaths::PathExists(opentxs::String(filename)));
	ASSERT_TRUE(addressbook->nymImport(filename));

	ASSERT_TRUE(addressbook->nymExist(exampleNymID));

	ASSERT_TRUE(addressbook->remove(exampleNymID));

	ASSERT_FALSE(addressbook->nymImport(filename_wrong));
	ASSERT_TRUE(addressbook->nymExport(exampleNym, "wrong id", filename_wrong));

	ASSERT_FALSE(addressbook->nymImport(filename_wrong));

}


