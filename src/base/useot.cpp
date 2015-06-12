/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "useot.hpp"

#include "lib_common3.hpp"

#include "bprinter/table_printer.h"

// Editline. Check 'git checkout linenoise' to see linenoise version.
#ifndef CFG_USE_EDITLINE // should we use editline?
	#define CFG_USE_EDITLINE 1 // default
#endif

#if CFG_USE_EDITLINE
	#ifdef __unix__
		#include <editline/readline.h> // to display history, TODO move that functionality somewhere else
	#else // not unix
		// TODO: do support MinGWEditline for windows)
	#endif // not unix
#endif // not use editline

namespace nOT {
namespace nUse {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_3 // <=== namespaces


cUseCache::cUseCache()
: mNymsLoaded(false)
, mAccountsLoaded(false)
, mAssetsLoaded(false)
, mServersLoaded(false)
{}

cUseOT::cUseOT(const string &mDbgName)
: mDbgName(mDbgName)
, mMadeEasy(new opentxs::OT_ME())
, mDataFolder( opentxs::OTPaths::AppDataFolder().Get() )
, mDefaultIDsFile( mDataFolder + "defaults.opt" )
{
	_dbg1("Creating cUseOT "<<DbgName());
	FPTR fptr;
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::Account, fptr = &cUseOT::AccountGetId) );
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::Asset, fptr = &cUseOT::AssetGetId) );
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::User, fptr = &cUseOT::NymGetId) );
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::Server, fptr = &cUseOT::ServerGetId) );

	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::Account, fptr = &cUseOT::AccountGetName) );
	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::Asset, fptr = &cUseOT::AssetGetName) );
	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::User, fptr = &cUseOT::NymGetName) );
	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::Server, fptr = &cUseOT::ServerGetName) );
}


string cUseOT::DbgName() const NOEXCEPT {
	return "cUseOT-" + ToStr((void*)this) + "-" + mDbgName;
}

void cUseOT::CloseApi() {
	if (OTAPI_loaded) {
		_dbg1("Will cleanup OTAPI");
		opentxs::OTAPI_Wrap::AppCleanup(); // Close OTAPI
		_dbg2("Will cleanup OTAPI - DONE");
	} else _dbg3("Will cleanup OTAPI ... was already not loaded");
}

cUseOT::~cUseOT() {
	delete mMadeEasy;
}

bool cUseOT::PrintInstrumentInfo(const string &instrument) {
	const auto txn = opentxs::OTAPI_Wrap::Instrmnt_GetTransNum(instrument);
	const auto assetID = opentxs::OTAPI_Wrap::Instrmnt_GetInstrumentDefinitionID(instrument);
	const auto serverID = opentxs::OTAPI_Wrap::Instrmnt_GetNotaryID(instrument);
	const auto senderAccID = opentxs::OTAPI_Wrap::Instrmnt_GetSenderAcctID(instrument);
	const auto senderNymID = opentxs::OTAPI_Wrap::Instrmnt_GetSenderNymID(instrument);
	const auto recNymID = opentxs::OTAPI_Wrap::Instrmnt_GetRecipientNymID(instrument);
	const auto amount = opentxs::OTAPI_Wrap::Instrmnt_GetAmount(instrument);
	const auto memo = opentxs::OTAPI_Wrap::Instrmnt_GetMemo(instrument);
	const auto validTo = opentxs::OTAPI_Wrap::Instrmnt_GetValidTo(instrument);
	const auto type = opentxs::OTAPI_Wrap::Instrmnt_GetType(instrument);

	auto col = zkr::cc::fore::cyan;
	auto col2 = zkr::cc::fore::blue;
	auto ncol = zkr::cc::console;
	auto err = zkr::cc::fore::lightred;


	auto now = OTTimeGetCurrentTime();
	cout << col << "              TXN: " << ncol << txn << endl;
	cout << col << "           Server: " << ncol << ServerGetName(serverID) << col2 << " (" << serverID << ")" << endl;
	cout << col << "   Sender account: " << ncol << AccountGetName(senderAccID) << col2 << " (" << senderAccID << ")" << endl;
	cout << col << "       Sender nym: " << ncol << NymGetName(senderNymID) << col2 << " (" << senderNymID << ")" << endl;
	cout << col << "    Recipient nym: " << ncol << NymGetRecipientName(recNymID) << col2 << " (" << recNymID << ")" << endl;
	cout << col << "       Asset type: " << ncol << AssetGetName(assetID) << col2 << " (" << assetID << ")" << endl;
	cout << col << "           Amount: " << ncol << amount << endl;
	cout << col << "             Memo: " << ncol << memo << endl;
	cout << col << "             Type: " << ncol << type << endl;

	if(validTo - now < 0) cout << err << "\n\nexpired " << ncol << endl;

	return true;
}


void cUseOT::LoadDefaults() {
	// TODO Check if defaults are correct.
	nUtils::cEnvUtils envUtils;
	if (!envUtils.FileExist(mDefaultIDsFile)) {
		string skeleton = "Account\nAsset\nUser\nServer";
		_dbg3("With content: " << skeleton);

		envUtils.WriteToFile(mDefaultIDsFile, skeleton);
		if(!envUtils.FileExist(mDefaultIDsFile))
			throw "Can't create " + mDefaultIDsFile;
	}

	if ( !configManager.Load(mDefaultIDsFile, mDefaultIDs) ) {
		_warn("Cannot open " + mDefaultIDsFile + " file, setting IDs with ID 0 as default");

		ID accountID = opentxs::OTAPI_Wrap::GetAccountWallet_ID(0);
        ID assetID = opentxs::OTAPI_Wrap::GetAssetType_ID(0);
        ID NymID = opentxs::OTAPI_Wrap::GetNym_ID(0);
        ID serverID = opentxs::OTAPI_Wrap::GetServer_ID(0);

		if ( accountID.empty() )
			_warn("There is no accounts in the wallet, can't set default account");

		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::Account, accountID));

		if ( assetID.empty() )
			_warn("There is no assets in the wallet, can't set default asset");

		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::Asset, assetID));

		if ( NymID.empty() )
			_erro("There is no nyms in the wallet, can't set default nym");

		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::User, NymID));

		if ( serverID.empty() )
			_erro("There is no servers in the wallet, can't set default server");

		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::Server, serverID));
		nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	}
}

bool cUseOT::DisplayDefaultSubject(const nUtils::eSubjectType type, bool dryrun) {
	_fact("display default " << nUtils::SubjectType2String(type) );
	if(dryrun) return true;
	if(!Init()) return false;
	ID defaultID = mDefaultIDs.at(type);
	string defaultName = (this->*cUseOT::subjectGetNameFunc.at(type))(defaultID);
	nUtils::DisplayStringEndl("Defaut " + nUtils::SubjectType2String(type) + ":" );
	nUtils::DisplayStringEndl(stringToColor(defaultID) + defaultID + " " + defaultName );
	return true;
}

bool cUseOT::DisplayAllDefaults(bool dryrun) {
	_fact("display all defaults" );
	if(dryrun) return true;
	if(!Init()) return false;
	auto nocol = zkr::cc::console;
	for (auto var : mDefaultIDs) {
		string defaultName = (this->*cUseOT::subjectGetNameFunc.at(var.first))(var.second);

		nUtils::DisplayStringEndl(cout,
				nUtils::SubjectType2String(var.first) + "\t" + nUtils::stringToColor(var.second) + var.second + " "
						+ defaultName + nocol);
	}
	return true;
}

bool cUseOT::DefaultsExport(const string & filename, bool dryrun) {
	_fact("export all defaults");
	if(dryrun) return true;
	if(!Init()) return false;

	auto p = [](string what, string value) -> string {return what + "=" + value + "\n";};
	string all = "";
	for (auto var : mDefaultIDs) {
		string defaultName = (this->*cUseOT::subjectGetNameFunc.at(var.first))(var.second);
		all += p(nUtils::SubjectType2String(var.first) + "ID", var.second); // ID
		all += p(nUtils::SubjectType2String(var.first), defaultName);
	}
	nUtils::cEnvUtils envUtils;
	envUtils.WriteToFile(filename, all);

	return true;
}

bool cUseOT::DisplayHistory(bool dryrun) {
#ifdef USE_EDITLINE
	_fact("ot history");
	if(dryrun) return true;

	for (int i=1; i<history_length; i++) {
		DisplayStringEndl( cout, history_get(i)->line );
	}
#endif
    return true;
}

string cUseOT::SubjectGetDescr(const nUtils::eSubjectType type, const string & subject) {
	ID subjectID = (this->*cUseOT::subjectGetIDFunc.at(type))(subject);
	string subjectName = (this->*cUseOT::subjectGetNameFunc.at(type))(subjectID);
	string description = subjectName + "(" + subjectID + ")";
	return nUtils::stringToColor(description);
}

bool cUseOT::Refresh(bool dryrun) {
	_fact("refresh all");
	if (dryrun)
		return true;
	if (!Init())
		return false;
	try {
		bool StatusAccountRefresh = AccountRefresh(AccountGetName(AccountGetDefault()), true, false);
		bool StatusNymRefresh = NymRefresh("^" + NymGetDefault(), true, false);
		if (StatusAccountRefresh == true && StatusNymRefresh == true) {
			_info("Succesfull refresh");
			return true;
		} else if (StatusAccountRefresh == true && StatusNymRefresh == false) {
			_dbg1("Can not refresh Nym");
			return false;
		} else if (StatusAccountRefresh == false && StatusNymRefresh == true) {
			_dbg1("Can not refresh Account");
			return false;
		}
		_dbg1("Can not refresh ");
		return false;
	} catch (std::exception& e) {
		_erro(e.what());
		return false;
	}
}

bool cUseOT::Init() { // TODO init on the beginning of application execution
	if (mDefaultIDs.empty()) LoadDefaults();
	if (OTAPI_error) return false;
	if (OTAPI_loaded) return true;
	try {
        if (!opentxs::OTAPI_Wrap::AppInit()) { // Init OTAPI
			_erro("Error while initializing wrapper");
			return false; // <--- RET
		}

		_info("Trying to load wallet now.");
		// if not pWrap it means that AppInit is not initialized
        opentxs::OTAPI_Exec *pWrap = opentxs::OTAPI_Wrap::It(); // TODO check why OTAPI_Exec is needed
		if (!pWrap) {
			OTAPI_error = true;
			_erro("Error while init OTAPI (1)");
			return false;
		}

        if (opentxs::OTAPI_Wrap::LoadWallet()) {
			_info("wallet was loaded.");
			OTAPI_loaded = true;
			LoadDefaults();
		}	else _erro("Error while loading wallet.");
	}
	catch(const std::exception &e) {
		_erro("Error while OTAPI init (2) - " << e.what());
		return false;
	}
	catch(...) {
		_erro("Error while OTAPI init thrown an UNKNOWN exception!");
		OTAPI_error = true;
		return false;
	}
	return OTAPI_loaded;
}

bool cUseOT::CheckIfExists(const nUtils::eSubjectType type, const string & subject) {
	if (!Init()) return false;
	if (subject.empty()) {
		_erro("Subject identifier is empty");
		return false;
	}
	ID subjectID = (this->*cUseOT::subjectGetIDFunc.at(type))(subject);

	if (!subjectID.empty()) {
		_dbg3(SubjectType2String(type) + " " + subject + " exists");
		return true;
	}
	_warn("Can't find this " + SubjectType2String(type) + " " + subject);
	return false;
}

bool cUseOT::CheckIfExists(const nUtils::eSubjectType type, const string & subject, const string & without) {
	return (subject == without)? false : CheckIfExists(type, subject);
}

vector<ID> cUseOT::AccountGetAllIds() {
	if(!Init())
	return vector<string> {};

	_dbg3("Retrieving accounts ID's");
	vector<string> accountsIDs;
	for(int i = 0 ; i < opentxs::OTAPI_Wrap::GetAccountCount ();i++) {
		accountsIDs.push_back(opentxs::OTAPI_Wrap::GetAccountWallet_ID (i));
	}
	return accountsIDs;
}

ID cUseOT::AccountGetAssetID(const string & account) {
	auto accID = AccountGetId(account);
	return (!accID.empty()) ? opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accID) : "";
}

string cUseOT::AccountGetAsset(const string & account) {
	return AssetGetName(AccountGetAssetID(account));
}

int64_t cUseOT::AccountGetBalance(const string & accountName) {
	if(!Init()) return 0; //FIXME

	int64_t balance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance ( AccountGetId(accountName) );
	return balance;
}

string cUseOT::AccountGetDefault() {
	if(!Init())
		return "";
	auto defaultAcc = mDefaultIDs.at(nUtils::eSubjectType::Account);
	if(defaultAcc.empty() || defaultAcc == "-") throw string("No default account!");
	return defaultAcc;
}

ID cUseOT::AccountGetId(const string & accountName) {
	if (!Init())
		return "";
	if (nUtils::checkPrefix(accountName))
		return accountName.substr(1);
	else {
		for (int i = 0; i < opentxs::OTAPI_Wrap::GetAccountCount(); i++) {
			if (opentxs::OTAPI_Wrap::GetAccountWallet_Name(opentxs::OTAPI_Wrap::GetAccountWallet_ID(i)) == accountName)
				return opentxs::OTAPI_Wrap::GetAccountWallet_ID(i);
		}
	}
	return "";
}

string cUseOT::AccountGetName(const ID & accountID) {
	if(!Init())
		return "";
	if(accountID.empty())
		return "";
	auto account = opentxs::OTAPI_Wrap::GetAccountWallet_Name(accountID);
	return (account.empty())? "" : account;
}

string cUseOT::AccountGetNym(const string & account) {
	if(!Init())
		return "";

	return NymGetName(AccountGetNymID(account));
}

ID cUseOT::AccountGetNymID(const string & account) {
	if(!Init())
		return "";

	return opentxs::OTAPI_Wrap::GetAccountWallet_NymID(AccountGetId(account));
}

bool cUseOT::AccountIsOwnerNym(const string & account, const string & nym) {
	if(!Init())
		return false;
	const ID rightNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(AccountGetId(account));
	if(nym.empty() || rightNymID.empty()) return false;
	return rightNymID == NymGetId(nym);
}


bool cUseOT::AccountRemove(const string & account, bool dryrun) { ///<
	_fact("account rm " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	if (opentxs::OTAPI_Wrap::Wallet_CanRemoveAccount(AccountGetId(account))) {
		return nUtils::reportError("Account cannot be deleted: doesn't have a zero balance?/outstanding receipts?");
	}

	if (opentxs::OTAPI_Wrap::deleteAssetAccount(mDefaultIDs.at(nUtils::eSubjectType::Server),
			mDefaultIDs.at(nUtils::eSubjectType::User), AccountGetId(account))) { //FIXME should be
		return nUtils::reportError("Failure deleting account: " + account);
	}
	_info("Account: " + account + " was successfully removed");
	cout << zkr::cc::fore::lightgreen << "Account: " << account << " was successfully removed" << zkr::cc::console
			<< endl;
	return true;
}

bool cUseOT::AccountRefresh(const string & accountName, bool all, bool dryrun) {
	_fact("account refresh " << accountName << " all=" << all);
	if(dryrun) return true;
	if(!Init()) return false;

	// int32_t serverCount = opentxs::OTAPI_Wrap::GetServerCount(); // unused

	if (all) {
		int32_t accountsRetrieved = 0;
		int32_t accountCount = opentxs::OTAPI_Wrap::GetAccountCount();

		if (accountCount == 0){
			_warn("No accounts to retrieve");
			return true;
		}

		for (int32_t accountIndex = 0; accountIndex < accountCount; ++accountIndex) {
			ID accountID = opentxs::OTAPI_Wrap::GetAccountWallet_ID(accountIndex);
			ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);
			ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
			if ( mMadeEasy->retrieve_account(accountServerID, accountNymID, accountID, false) ) { // forcing download
				_info("Account " + AccountGetName(accountID) + "(" + accountID +  ")" + " retrieval success from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
				++accountsRetrieved;
			}else
				_erro("Account " + AccountGetName(accountID) + "(" + accountID +  ")" + " retrieval failure from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
		}
        string count = ToStr(accountsRetrieved) + "/" + ToStr(accountCount);
		if (accountsRetrieved == accountCount) {
			_info("All accounts were successfully retrieved " << count);
			return true;
		} else if (accountsRetrieved == 0) {
			_erro("Accounts retrieval failure " << count);
			return false;
		} else {
			_erro("Some accounts cannot be retrieved " << count);
			return true;
		}
	}
	else {
		ID accountID = AccountGetId(accountName);
		ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);
		ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
		if ( mMadeEasy->retrieve_account(accountServerID, accountNymID, accountID, true) ) { // forcing download
			_info("Account " + accountName + "(" + accountID +  ")" + " retrieval success from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
			return true;
		}
		_warn("Account " + accountName + "(" + accountID +  ")" + " retrieval failure from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
		return false;
	}
	return false;
}

bool cUseOT::AccountRename(const string & account, const string & newAccountName, bool dryrun) {
	_fact("account mv from " << account << " to " << newAccountName);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);

	if( AccountSetName (accountID, newAccountName, false) ) {
		_info("Account " << AccountGetName(accountID) + "(" + accountID + ")" << " renamed to " << newAccountName);
		return true;
	}
	_erro("Failed to rename account " << AccountGetName(accountID) + "(" + accountID + ")" << " to " << newAccountName);
	return false;
}

bool cUseOT::AccountCreate(const string & nym, const string & asset, const string & newAccountName, const string & server, bool dryrun) {
	_fact("account new nym=" << nym << " asset=" << asset << " accountName=" << newAccountName << " serverName=" << server);
	if(dryrun) return true;
	if(!Init()) return false;

	if ( CheckIfExists(nUtils::eSubjectType::Account, newAccountName) ) {
		_erro("Cannot create new account: '" << newAccountName << "'. Account with that name exists" );
		return false;
	}

	ID nymID = NymGetId(nym);
	ID assetID = AssetGetId(asset);
	ID serverID = ServerGetId(server);

	string response;
	response = mMadeEasy->create_asset_acct(serverID, nymID, assetID);

	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy->VerifyMessageSuccess(response)) {
		_erro("Failed trying to create Account at Server.");
		return false;
	}

	// Get the ID of the new account.
	ID accountID = opentxs::OTAPI_Wrap::Message_GetNewAcctID(response);
	if (!accountID.size()){
		_erro("Failed trying to get the new account's ID from the server response.");
		return false;
	}

	// Set the Name of the new account.
	if ( AccountSetName(accountID, newAccountName, false) ){
		cout << "Account " << newAccountName << "(" << accountID << ")" << " created successfully." << endl;
		AccountRefresh("^" + accountID, false, dryrun);
	}

	try {
		auto defaultAcc = AccountGetDefault();
	} catch(...) {
		return AccountSetDefault(newAccountName, false);
	}
	return true;
}

vector<string> cUseOT::AccountGetAllNames() {
	if(!Init())
	return vector<string> {};

	_dbg3("Retrieving all accounts names");
	vector<string> accounts;
	for(int i = 0 ; i < opentxs::OTAPI_Wrap::GetAccountCount ();i++) {
		accounts.push_back(opentxs::OTAPI_Wrap::GetAccountWallet_Name ( opentxs::OTAPI_Wrap::GetAccountWallet_ID (i)));
	}
	return accounts;
}

bool cUseOT::AccountDisplay(const string & account, bool dryrun) {
	_fact("account show account=" << account);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	string stat = mMadeEasy->stat_asset_account(accountID);
	if ( !stat.empty() ) {
			nUtils::DisplayStringEndl(cout, stat);
			return true;
	}
	_erro("Error trying to stat an account: " + accountID);
	return false;
}

bool cUseOT::AccountDisplayAll(bool dryrun) {
	_fact("account ls");
	if(dryrun) return true;
	if(!Init()) return false;

	const int32_t count = opentxs::OTAPI_Wrap::GetAccountCount();

	if (count < 1) {
		cout << zkr::cc::fore::yellow << "no accounts to display" << zkr::cc::console << endl;
		return false;
	}

	bprinter::TablePrinter tp(&std::cout);
	tp.AddColumn("ID", 4);
	tp.AddColumn("Type", 10);
	tp.AddColumn("Account", 55);
	tp.AddColumn("Asset", 55);
	tp.AddColumn("Balance", 12);

	tp.PrintHeader();
	for (int32_t i = 0; i < count; i++) {
		ID accountID = opentxs::OTAPI_Wrap::GetAccountWallet_ID(i);
		int64_t balance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(accountID);
		ID assetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);
		string accountType = opentxs::OTAPI_Wrap::GetAccountWallet_Type(accountID);
		if(accountType=="issuer") tp.SetContentColor(zkr::cc::fore::lightred);
		else if (accountType=="simple") tp.SetContentColor(zkr::cc::fore::lightgreen);

		tp << std::to_string(i) << "(" + accountType + ")" << AccountGetName(accountID) + " (" + accountID + ")"
				<< AssetGetName(assetID) + " (" + assetID + ")" << std::to_string(balance);
	}

	tp.PrintFooter();
	return true;
}

bool cUseOT::AccountSetDefault(const string & account, bool dryrun) {
	_fact("account set-default " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::Account) = AccountGetId(account);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

bool cUseOT::AccountTransfer(const string & accountFrom, const string & accountTo, const int64_t & amount, const string & note, bool dryrun) {
	_fact("account transfer  from " << accountFrom << " to " << accountTo << " amount=" << amount << " note=" << note);
	if(dryrun) return true;
	if(!Init()) return false;

	if( AccountGetAssetID(accountFrom) !=  AccountGetAssetID(accountTo) ) {
		cout << zkr::cc::fore::yellow;
		cout << "Accounts have different assets!" << zkr::cc::fore::console << endl;
		cout << "Sender account (" << accountFrom << ") has asset: " << AccountGetAsset(accountFrom) << endl;
		cout << "Recipient account (" << accountTo << ") has asset: " << AccountGetAsset(accountTo) << endl;
		cout << "Aborting" << endl;
		cout << endl;
		return false;
	}
	cout << "Sending " << amount << " " << AccountGetAsset(accountFrom) << " from " << accountFrom << " to "
			<< accountTo << endl;
	ID accountFromID = AccountGetId(accountFrom);
	ID accountToID = AccountGetId(accountTo);
	ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountFromID);
	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountFromID);

	string response = mMadeEasy->send_transfer(accountServerID, accountNymID, accountFromID, accountToID, amount, note);

	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy->VerifyMessageSuccess(response)) {
		_erro("Failed to send transfer from " << accountFrom << " to " << accountTo);
		return false;
	}
	return true;
}

bool cUseOT::AccountInDisplay(const string & account, bool dryrun) {
	_fact("account-in ls " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);
	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);

	string inbox = opentxs::OTAPI_Wrap::LoadInbox(accountServerID, accountNymID, accountID); // Returns NULL, or an inbox.

	if (inbox.empty()) {
		_info("Unable to load inbox for account " << AccountGetName(accountID)<< "(" << accountID << "). Perhaps it doesn't exist yet?");
		return false;
	}

	int32_t transactionCount = opentxs::OTAPI_Wrap::Ledger_GetCount(accountServerID, accountNymID, accountID, inbox);

	if (transactionCount > 0) {
		cout << zkr::cc::fore::lightblue;
		cout << "  account: " << account << endl;
		cout << "    asset: " << AssetGetName(AccountGetAssetID(account)) << endl;
		cout << zkr::cc::console << endl;

		bprinter::TablePrinter tp(&std::cout);
		tp.AddColumn("ID", 4);
		tp.AddColumn("Amount", 10);
		tp.AddColumn("Type", 12);
		tp.AddColumn("TxN", 7);
		tp.AddColumn("InRef", 7);
		tp.AddColumn("From Nym", 60);
		tp.AddColumn("From Account", 60);

		tp.PrintHeader();

		for (int32_t index = 0; index < transactionCount; ++index) {
			string transaction = opentxs::OTAPI_Wrap::Ledger_GetTransactionByIndex(accountServerID, accountNymID, accountID, inbox, index);
			int64_t transactionID = opentxs::OTAPI_Wrap::Ledger_GetTransactionIDByIndex(accountServerID, accountNymID, accountID, inbox, index);
			int64_t refNum = opentxs::OTAPI_Wrap::Transaction_GetDisplayReferenceToNum(accountServerID, accountNymID, accountID, transaction);
			int64_t amount = opentxs::OTAPI_Wrap::Transaction_GetAmount(accountServerID, accountNymID, accountID, transaction);
			string transactionType = opentxs::OTAPI_Wrap::Transaction_GetType(accountServerID, accountNymID, accountID, transaction);
			string senderNymID = opentxs::OTAPI_Wrap::Transaction_GetSenderNymID(accountServerID, accountNymID, accountID, transaction);
			string senderAcctID = opentxs::OTAPI_Wrap::Transaction_GetSenderAcctID(accountServerID, accountNymID, accountID, transaction);
			string recipientNymID = opentxs::OTAPI_Wrap::Transaction_GetRecipientNymID(accountServerID, accountNymID, accountID, transaction);
			string recipientAcctID = opentxs::OTAPI_Wrap::Transaction_GetRecipientAcctID(accountServerID, accountNymID, accountID, transaction);

			//TODO Check if Transaction information needs to be verified!!!
			// XXX; test this!! Should be recipient or sender?
            tp << ToStr(index) << ToStr(amount) << transactionType << ToStr(transactionID) << ToStr(refNum)
//				 << NymGetName(senderNymID) + "(" + senderNymID + ")" <<  AccountGetName( senderAcctID ) + "(" + senderAcctID + ")";
				<< NymGetName(recipientNymID) + "(" + recipientNymID + ")" <<  AccountGetName( recipientAcctID ) + "(" + recipientNymID + ")";

		}
		tp.PrintFooter();
	  return true;
	} else {
		_info("There is no transactions in inbox for account " << AccountGetName(accountID)<< "(" << accountID << ")");
		cout << zkr::cc::fore::yellow << "no transactions in inbox for account " << account << zkr::cc::console << endl;
		return true;
	}
	return false;
}

bool cUseOT::AccountInAccept(const string & account, const int index, bool all, bool dryrun) { //TODO make it work with --all, multiple indices
	_fact("account-in accept " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);

	int32_t nItemType = 0; // TODO pass it as an argument


	if (all) {
		int32_t transactionsAccepted = 0;

		ID serverID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);
		ID nymID = AccountGetNymID(account);

		mMadeEasy->retrieve_account(serverID, nymID, accountID, true);

		string inbox = opentxs::OTAPI_Wrap::LoadInbox(serverID, nymID, accountID); // Returns NULL, or an inbox.

		if (inbox.empty()) {
			cout << "Unable to load inbox for " << AccountGetName(accountID) << endl;
			_info("Unable to load inbox for account " << AccountGetName(accountID)<< "(" << accountID << "). Perhaps it doesn't exist yet?");
			return false;
		}
		int32_t transactionCount = opentxs::OTAPI_Wrap::Ledger_GetCount(serverID, nymID, accountID, inbox);
		_dbg3("Transaction count in inbox: " << transactionCount);

		if (transactionCount == 0){
			cout << zkr::cc::fore::yellow << "Empty inbox ("<< account << ")" << zkr::cc::console << endl;
			_warn("No transactions in inbox");
			return true;
		}

		for (int32_t index = 0; index < transactionCount; ++index) {
			auto accepted =  mMadeEasy->accept_inbox_items( accountID, nItemType, ToStr(0));
			if(!accepted) { // problem with transtaction accepting, trying once again
				_warn("transaction " << index << " failed, trying again");
				accepted =  mMadeEasy->accept_inbox_items( accountID, nItemType, ToStr(0));
			}

			if (accepted) {
				_info("Successfully accepted inbox transaction number: " << index);
				++transactionsAccepted;
			} else
				_erro("Failed to accept inbox transaction for number: " << index);
		}


		string count = ToStr(transactionsAccepted) + "/" + ToStr(transactionCount);
		if (transactionsAccepted == transactionCount) {
			_info("All transactions were successfully accepted " << count);
			mMadeEasy->retrieve_account(serverID, nymID, accountID, true);
			cout << zkr::cc::fore::lightgreen << "Payments accepted" << zkr::cc::console << endl;
			return true;
		} else if (transactionsAccepted == 0) {
			_erro("Transactions cannot be accepted " << count);
			return false;
		} else {
			_erro("Some transactions cannot be accepted " << count);
			return true;
		}

	}
	else {
		if ( mMadeEasy->accept_inbox_items( accountID, nItemType, ToStr(index) ) ) {
			_info("Successfully accepted inbox transaction number: " << index);
			return true;
		}
		_erro("Failed to accept inbox transaction for number: " << index);
		return false;
	}
	return false;
}

bool cUseOT::AccountOutCancel(const string & account, const int index, bool all, bool dryrun) { //TODO make it work with --all, multiple indices
	_fact("account-out cancel " << account << " transaction=" << index);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	// int32_t nItemType = 0; // TODO pass it as an argument

	if ( mMadeEasy->cancel_outgoing_payments( accountNymID, accountID, ToStr(index) ) ) { //TODO cancel_outgoing_payments is not for account outbox
		_info("Successfully cancelled outbox transaction: " << index);
		return true;
	}
	_erro("Failed to cancel outbox transaction: " << index);
	return false;
}

bool cUseOT::AccountOutDisplay(const string & account, bool dryrun) {
	_fact("account-out ls " << account);
	if (dryrun)
		return true;
	if (!Init())
		return false;
	cout << zkr::cc::fore::lightblue;
	cout << "  account: " << account << endl;
	cout << "    asset: " << AssetGetName(AccountGetAssetID(account)) << endl;
	cout << zkr::cc::console << endl;

	ID accountID = AccountGetId(account);
	ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);
	ID accountNymID = AccountGetNymID(account);

	mMadeEasy->retrieve_account(accountServerID, accountNymID, accountID, true);
	string outbox = opentxs::OTAPI_Wrap::LoadOutbox(accountServerID, accountNymID, accountID); // Returns NULL, or an inbox.

	if (outbox.empty()) {
		_info(
				"Unable to load outbox for account " << AccountGetName(accountID)<< "(" << accountID << "). Perhaps it doesn't exist yet?");
		return false;
	}

	int32_t transactionCount = opentxs::OTAPI_Wrap::Ledger_GetCount(accountServerID, accountNymID, accountID, outbox);

	if (transactionCount > 0) {
		bprinter::TablePrinter tp(&std::cout);
		tp.AddColumn("ID", 4);
		tp.AddColumn("Amount", 10);
		tp.AddColumn("Type", 10);
		tp.AddColumn("TxN", 8);
		tp.AddColumn("InRef", 8);
//		tp.AddColumn("To Nym", 50); // XXX: opentxs::OTAPI_Wrap::Transaction_GetRecipientNymID returns NULL, so printing nym is disabled.
		 tp.AddColumn("To Account", 50);

		tp.PrintHeader();
		for (int32_t index = 0; index < transactionCount; ++index) {
			const string transaction = opentxs::OTAPI_Wrap::Ledger_GetTransactionByIndex(accountServerID, accountNymID,
					accountID, outbox, index);
			int64_t transactionID = opentxs::OTAPI_Wrap::Ledger_GetTransactionIDByIndex(accountServerID, accountNymID,
					accountID, outbox, index);
			int64_t refNum = opentxs::OTAPI_Wrap::Transaction_GetDisplayReferenceToNum(accountServerID, accountNymID,
					accountID, transaction);
			int64_t amount = opentxs::OTAPI_Wrap::Transaction_GetAmount(accountServerID, accountNymID, accountID,
					transaction);
			string transactionType = opentxs::OTAPI_Wrap::Transaction_GetType(accountServerID, accountNymID, accountID,
					transaction);
			string recipientAcctID = opentxs::OTAPI_Wrap::Transaction_GetRecipientAcctID(accountServerID, accountNymID,
					accountID, transaction);

			//TODO Check if Transaction information needs to be verified!!!
			tp << ToStr(index) << ToStr(amount) << transactionType << ToStr(transactionID)
					<< ToStr(refNum)
					// << "BUG - working on it" << "BUG - working on it" ;
//					<< NymGetName(recipientNymID) + "(" + recipientNymID + ")";
					<< AccountGetName(recipientAcctID) + "(" + recipientAcctID + ")";
		}
		tp.PrintFooter();
		return true;
	} else {
		_info("There is no transactions in outbox for account " << AccountGetName(accountID)<< "(" << accountID << ")");
		return true;
	}
	return false;
}

bool cUseOT::AccountSetName(const string & accountID, const string & newAccountName, bool dryrun) { //TODO: passing to function: const string & nymName, const string & signerNymName,
	_fact("account set-name " << accountID << " " << newAccountName);
	if(dryrun) return true;
	if(!Init()) return false;

	if ( !opentxs::OTAPI_Wrap::SetAccountWallet_Name (accountID, mDefaultIDs.at(nUtils::eSubjectType::User), newAccountName) ) {
		return reportError("Failed trying to name new account: " + accountID);
	}
	_info("Set account " << accountID << "name to " << newAccountName);
	cout << "Set account " << accountID << "name to " << newAccountName << endl;
	return true;
}
bool cUseOT::AddressBookAdd(const string & nym, const string & newNym, const ID & newNymID, bool dryrun) {
	_fact("addressbook add " << nym << " " << newNym << " " << newNymID);
	if(dryrun) return true;
	if(!Init()) return false;

	if(CheckIfExists(nUtils::eSubjectType::User, newNym)) {
		cout << "This nym (" << newNym << ") is in wallet, aborting" << endl;
		return false;
	}
	auto addressbook = AddressBookStorage::Get(NymGetId(nym));
	auto added = addressbook->add(newNym, newNymID);

	return added;
}
bool cUseOT::AddressBookDisplay(const string & nym, bool dryrun) {
	_fact("addressbook ls " << nym);
	if(dryrun) return true;
	if(!Init()) return false;

	cout << "owner nym: " << nym << endl;
	auto addressbook = AddressBookStorage::Get(NymGetId(nym));
	addressbook->display();
	return true;
}

bool cUseOT::AddressBookRemove(const string & ownerNym, const ID & toRemoveNymID, bool dryrun) {
	_fact("addressbook remove " << ownerNym << " " << toRemoveNymID);
	if(dryrun) return true;
	if(!Init()) return false;

	auto addressbook = AddressBookStorage::Get(NymGetId(ownerNym));
	auto removed = addressbook->remove(toRemoveNymID);
	return removed;
}

EXEC bool cUseOT::AddressBookImport(const string & ownerNym, const string & filename, bool dryrun) {
	_fact("addressbook import " << ownerNym << " " << filename);
	if(dryrun) return true;
	if(!Init()) return false;

	auto addressbook = AddressBookStorage::Get(NymGetId(ownerNym));

	return addressbook->nymImport(filename);
}

EXEC bool cUseOT::AddressBookExport(const string & ownerNym, const string & exportNym, const string & filename, bool dryrun) {
	_fact("addressbook export " << ownerNym << " " << exportNym << " " << filename);
	if(dryrun) return true;
	if(!Init()) return false;

	auto addressbook = AddressBookStorage::Get(NymGetId(ownerNym));

	const ID exportNymID = NymGetToNymId(exportNym, NymGetId(ownerNym));
	return  addressbook->nymExport(exportNym, exportNymID, filename);
}

vector<string> cUseOT::AssetGetAllNames() {
	if(!Init())
	return vector<string> {};

	vector<string> assets;
	for(int32_t i = 0 ; i < opentxs::OTAPI_Wrap::GetAssetTypeCount ();i++) {
		assets.push_back(opentxs::OTAPI_Wrap::GetAssetType_Name ( opentxs::OTAPI_Wrap::GetAssetType_ID (i)));
	}
	return assets;
}

string cUseOT::AssetGetName(const ID & assetID) {
	if(!Init())
		return "";
	if(assetID.empty())
		return "";

	auto asset = opentxs::OTAPI_Wrap::GetAssetType_Name(assetID);
	return (asset.empty())? "" : asset;
}

bool cUseOT::AssetAdd(const string & filename, bool dryrun) {
	_fact("asset add");
	if(dryrun) return true;
	if(!Init()) return false;

	string contract = "";
	nUtils::cEnvUtils envUtils;

	try {
		contract = nUse::GetInput(filename);
	} catch (const string & message) {
		return nUtils::reportError("", message, "Provided contract was empty");
	}
	auto result = opentxs::OTAPI_Wrap::AddAssetContract(contract);

	if (result != 1) {
		cout << zkr::cc::fore::lightred << "You must input a currency contract, in order to add it to your wallet"
				<< zkr::cc::console << endl;
		_erro("adding asset failed, result= " << result);
		return false;
	}

	cout << zkr::cc::fore::lightgreen << "Success adding asset contract to your wallet" << zkr::cc::console << endl;

	try {
		auto defaultAsset = AssetGetDefault();
	} catch(...) {
		return AssetSetDefault();
	}

	return true;
}

bool cUseOT::AssetDisplayAll(bool dryrun) {
	_fact("asset ls");
	if(dryrun) return true;
	if(!Init()) return false;

	_dbg3("Retrieving all asset names");
	for(std::int32_t i = 0 ; i < opentxs::OTAPI_Wrap::GetAssetTypeCount();i++) {
		ID assetID = opentxs::OTAPI_Wrap::GetAssetType_ID(i);
		nUtils::DisplayStringEndl(nUtils::stringToColor(assetID) + assetID + " " + AssetGetName( assetID ) );
	}
	return true;
}

string cUseOT::AssetGetId(const string & assetName) {
	if(!Init())
		return "";
	if(assetName.empty()) return "";
	if ( nUtils::checkPrefix(assetName) )
		return assetName.substr(1);
	else {
		for(std::int32_t i = 0 ; i < opentxs::OTAPI_Wrap::GetAssetTypeCount ();i++) {
			if(opentxs::OTAPI_Wrap::GetAssetType_Name ( opentxs::OTAPI_Wrap::GetAssetType_ID (i))==assetName)
				return opentxs::OTAPI_Wrap::GetAssetType_ID (i);
		}
	}
	return "";
}

string cUseOT::AssetGetContract(const string & asset){
	if(!Init()) return "";
	string strContract = opentxs::OTAPI_Wrap::GetAssetType_Contract( AssetGetId(asset) );
	return strContract;
}

ID cUseOT::AssetGetDefault(){
	if(!Init()) return "";
	auto defaultAsset = mDefaultIDs.at(nUtils::eSubjectType::Asset);
	if(defaultAsset.empty() || defaultAsset == "-") throw string("No default asset!");
	return defaultAsset;
}


bool cUseOT::AssetIssue(const string & server, const string & nym, const string & filename, bool dryrun) { // Issue new asset type
	_fact("asset ls " << server << " " << nym << " " << filename);
	if(dryrun) return true;
	if(!Init()) return false;

	const ID serverID = ServerGetId(server);
	const ID nymID = NymGetId(nym);

	if(!opentxs::OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID))
		return reportError("Nym " + nym + " isn't register at server!");

	string signedContract = GetInput(filename);

	string strResponse = mMadeEasy->issue_asset_type(serverID, nymID, signedContract);

	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy->VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to issue asset at Server.");
		return false;
	}
	return true;
}

bool cUseOT::AssetNew(const string & nym, const string & filename, bool dryrun) {
	_fact("asset new for nym=" << nym);
	if(dryrun) return true;
	if(!Init()) return false;
	string xmlContents;

	try {
		xmlContents = nUse::GetInput(filename);
	} catch (const string & message) {
		return nUtils::reportError("", message, "Provided contract was empty");
	}

	nUtils::DisplayStringEndl(cout, opentxs::OTAPI_Wrap::CreateAssetContract(NymGetId(nym), xmlContents) );

	try {
		auto defaultAsset = AssetGetDefault();
	} catch(...) {
		return AssetSetDefault();
	}

	return true;
}

bool cUseOT::AssetRemove(const string & asset, bool dryrun) {
	_fact("asset rm " << asset);
	if(dryrun) return true;
	if(!Init()) return false;

	string assetID = AssetGetId(asset);
	if ( opentxs::OTAPI_Wrap::Wallet_CanRemoveAssetType(assetID) ) {
		if ( opentxs::OTAPI_Wrap::Wallet_RemoveAssetType(assetID) ) {
			_info("Asset was deleted successfully");
			mDefaultIDs.at(nUtils::eSubjectType::Asset) = "-";
			return true;
		}
	}
	_warn("Asset cannot be removed");
	return false;
}
bool cUseOT::AssetSetDefault() {
	ID assetID = opentxs::OTAPI_Wrap::GetAssetType_ID(0);
	if(assetID.empty()) return false;
	_note("Setting asset" << AssetGetName(assetID) << " as default");
	return AssetSetDefault(AssetGetName(assetID), false);
}

bool cUseOT::AssetSetDefault(const std::string & asset, bool dryrun){
	_fact("asset set-default " << asset);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::Asset) = AssetGetId(asset);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

bool cUseOT::AssetShowContract(const string & asset, const string & filename, bool dryrun) {
	_fact("asset show-contract " << asset);
	if(dryrun) return true;
	if(!Init()) return false;

	const ID assetID = AssetGetId(asset);
	const auto contract = opentxs::OTAPI_Wrap::GetAssetType_Contract(assetID);

	if(!filename.empty()) {
		try {
			nUtils::cEnvUtils envUtils;
			envUtils.WriteToFile(filename, contract);
			return true;
		} catch(...) {
			_warn("can't save to file");
		}
	}

	cout << zkr::cc::fore::lightblue << asset << zkr::cc::fore::blue << " (" << assetID << ")" << zkr::cc::console << endl << endl;
	cout << contract << endl;

	return true;
}

// testing
bool cUseOT::BasketNew(const string & nym, const string & server, const string & assets, bool dryrun) {
	_fact("ot basket new " << nym << " " << server << " " << assets);
	if(dryrun) return true;
	if(!Init()) return false;

	// TODO: basket
	/*
	int32_t basketCount = 2;
	int64_t amount = 100;
	auto fromNym = "Trader Bob";

	string basket = opentxs::OTAPI_Wrap::GenerateBasketCreation(NymGetId(fromNym), amount);

	_dbg2("basket: " << basket);

	const auto asset1 = "US Dollars";
	const auto asset2 = "Bitcoins";

	const auto asset1ID = AssetGetId(asset1);
	const auto asset2ID = AssetGetId(asset2);

	_dbg3(opentxs::OTAPI_Wrap::GetAssetType_Contract(asset1ID));
	_dbg3(opentxs::OTAPI_Wrap::GetAssetType_Contract(asset2ID));

	auto tmpBasket = opentxs::OTAPI_Wrap::AddBasketCreationItem(NymGetId(fromNym), basket, asset2ID, amount);

	_dbg2("tmpBasket: " << tmpBasket);

	basket = tmpBasket;

	auto response = mMadeEasy->issue_basket_currency(ServerGetDefault(), NymGetId(fromNym), basket);
*/
	return true;
}

bool cUseOT::CashExportWrap(const ID & nymSender, const ID & nymRecipient, const string & account, bool passwordProtected, bool dryrun) {
	// TODO get indices
	_fact("cash export from " << nymSender << " to " << nymRecipient << " account " << account );
	if(dryrun) return true;
	if(!Init()) return false;

	ID nymSenderID = NymGetId(nymSender);
	ID nymRecipientID = NymGetToNymId(nymRecipient, nymSenderID);

	string indices = "";
	string retained_copy = "";

	string exportedCash = CashExport( nymSenderID, nymRecipientID, account, indices, passwordProtected, retained_copy);

	if (exportedCash.empty()) {
		_erro("Exported string is empty");
		return false;
	}

	DisplayStringEndl(cout, exportedCash);
	return true;
}

string cUseOT::CashExport(const ID & nymSenderID, const ID & nymRecipientID, const string & account, const string & indices, const bool passwordProtected, string & retained_copy) {
	_fact("cash export from " << nymSenderID << " to " << nymRecipientID << " account " << account << " indices: " << indices << "passwordProtected: " << passwordProtected);

	ID accountID = AccountGetId(account);
	ID accountAssetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);
	ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);

	string contract = mMadeEasy->load_or_retrieve_contract(accountServerID, nymSenderID, accountAssetID);

	string exportedCash = mMadeEasy->export_cash(accountServerID, nymSenderID, accountAssetID, nymRecipientID, indices, passwordProtected, retained_copy);
	_info("Cash was exported");
	return exportedCash;
}

bool cUseOT::CashImport(const string & nym, bool dryrun) {
                               //optional?
	//TODO add input from file support
	_fact("cash import ");
	if (dryrun) return false;
	if (!Init()) return false;

	ID nymID = NymGetId(nym);

	_dbg3("Open text editor for user to paste payment instrument");
	string instrument = GetText();

	string instrumentType = opentxs::OTAPI_Wrap::Instrmnt_GetType(instrument);

	if (instrumentType.empty()) {
		opentxs::OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine instrument type. Expected (cash) PURSE.\n");
		return false;
	}

	string serverID = opentxs::OTAPI_Wrap::Instrmnt_GetNotaryID(instrument);

	if (serverID.empty()) {
			opentxs::OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine server ID from purse.\n");
			return false;
	}

	if ("PURSE" == instrumentType) {
	}
	/** Todo: case "TOKEN"
	 *
	 * NOTE: This is commented out because since it is guessing the NymID as MyNym,
	 * then it will just create a purse for MyNym and import it into that purse, and
	 * then later when doing a deposit, THAT's when it tries to DECRYPT that token
	 * and re-encrypt it to the SERVER's nym... and that's when we might find out that
	 * it never was encrypted to MyNym in the first place -- we had just assumed it
	 * was here, when we did the import. Until I can look at that in more detail, it
	 * will remain commented out.
	 */
	//else {
			//            // This version supports cash tokens (instead of purse...)
			//            bool bImportedToken = importCashPurse(strServerID, MyNym, strAssetID, userInput, isPurse)
			//
			//            if (bImportedToken)
			//            {
			//                opentxs::OTAPI_Wrap::Output(0, "\n\n Success importing cash token!\nServer: "+strServerID+"\nAsset Type: "+strAssetID+"\nNym: "+MyNym+"\n\n");
			//                return 1;
			//; }

			//opentxs::OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine instrument type. Expected (cash) PURSE.\n");
			//return false;
	//}

	// This tells us if the purse is password-protected. (Versus being owned
	// by a Nym.)
	bool hasPassword = opentxs::OTAPI_Wrap::Purse_HasPassword(serverID, instrument);

	/**
	 * Even if the Purse is owned by a Nym, that Nym's ID may not necessarily
	 * be present on the purse itself (it's optional to list it there.)
	 * opentxs::OTAPI_Wrap::Instrmnt_GetRecipientNymID tells us WHAT the recipient User ID
	 * is, IF it's on the purse. (But does NOT tell us WHETHER there is a
	 * recipient. The above function is for that.)
	 */
	ID purseOwner = "";

	if (!hasPassword) {
			purseOwner = opentxs::OTAPI_Wrap::Instrmnt_GetRecipientNymID(instrument); // TRY and get the Nym ID (it may have been left blank.)
	}
	/**
	 * Whether the purse was password-protected (and thus had no Nym ID)
	 * or whether it does have a Nym ID (but it wasn't listed on the purse)
	 * Then either way, in those cases strPurseOwner will still be NULL.
	 *
	 * (The third case is that the purse is Nym protected and the ID WAS available,
	 * in which case we'll skip this block, since we already have it.)
	 *
	 * But even in the case where there's no Nym at all (password protected)
	 * we STILL need to pass a Signer Nym ID into opentxs::OTAPI_Wrap::Wallet_ImportPurse.
	 * So if it's still NULL here, then we use --mynym to make the call.
	 * And also, even in the case where there IS a Nym but it's not listed,
	 * we must assume the USER knows the appropriate NymID, even if it's not
	 * listed on the purse itself. And in that case as well, the user can
	 * simply specify the Nym using --mynym.
	 *
	 * Bottom line: by this point, if it's still not set, then we just use
	 * MyNym, and if THAT's not set, then we return failure.
	 */
	if (purseOwner.empty()) {
		opentxs::OTAPI_Wrap::Output(0,
				"\n\n The NymID isn't evident from the purse itself... (listing it is optional.)\n"
				"The purse may have no Nym at all--it may instead be password-protected.) "
				"Either way, a signer nym is still necessary, even for password-protected purses.\n\n "
				"Trying MyNym...\n");
			purseOwner = nymID;
	}

	string assetID = opentxs::OTAPI_Wrap::Instrmnt_GetInstrumentDefinitionID(instrument);

	if (assetID.empty()) {
			opentxs::OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine asset type ID from purse.\n");
			return false;
	}

	bool imported = opentxs::OTAPI_Wrap::Wallet_ImportPurse(serverID, assetID, purseOwner, instrument);

	if (imported) {
			opentxs::OTAPI_Wrap::Output(0, "\n\n Success importing purse!\nServer: " + serverID + "\nAsset Type: " + assetID + "\nNym: " + purseOwner + "\n\n");
			return true;
	}

	return false;
}

bool cUseOT::CashDeposit(const string & accountID, const string & nymFromID, const string & serverID, const string & instrument) {
	if(!Init()) return false;

	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);

	string purseValue = instrument;

	if (instrument.empty()) {
		// LOAD PURSE
		_dbg3("Loading purse");
		purseValue = opentxs::OTAPI_Wrap::LoadPurse(serverID, accountAssetID, nymFromID); // returns NULL, or a purse.

		if (purseValue.empty()) {
			opentxs::OTAPI_Wrap::Output(0, " Unable to load purse from local storage. Does it even exist?\n");
			return false;
		}
	}

	_dbg3("Processing cash deposit to account");
	int32_t nResult = mMadeEasy->deposit_cash(serverID, accountNymID, accountID, purseValue); // TODO pass reciever nym if exists in purse
	if (nResult < 1) {
		DisplayStringEndl(cout, "Unable to deposit purse");
		return false;
	}
	return true;
}

bool cUseOT::CashDepositWrap(const string & account, bool dryrun) {
	//TODO accept indices as arguments
	//FIXME cleaning arguments
	_fact("Deposit purse to account: " << account);
	if (dryrun) return false;
	if (!Init()) return false;

	ID accountID = AccountGetId(account);

	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);
	ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);

	_dbg3("Open text editor for user to paste payment instrument");
	string instrument = GetText();

	return CashDeposit(accountID, accountNymID, accountServerID, instrument);
}

bool cUseOT::CashSend(const string & nymSender, const string & nymRecipient, const string & account, int64_t amount, bool dryrun) { // TODO make it work with longer version: asset, server, nym
	_fact("cash send from " << nymSender << " to " << nymRecipient << " account " << account );
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);
	ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);

	ID nymSenderID = NymGetId(nymSender);
	ID nymRecipientID = NymGetToNymId(nymRecipient, nymSenderID);

	_info("Withdrawing cash from account: " << account << " amount: " << amount);
	bool withdrawalSuccess = CashWithdraw(account, amount, false);
	if (!withdrawalSuccess) {
		_erro("Withdrawal failed");
		DisplayStringEndl(cout, "Withdrawal from account: " + account + "failed");
		return false;
	}
	string retainedCopy = "";
	string indices = "";
	bool passwordProtected = false; // TODO check if password protected

	string exportedCashPurse = CashExport(nymSenderID, nymRecipientID, account, indices, passwordProtected, retainedCopy);
	if (!exportedCashPurse.empty()) {
		string response = mMadeEasy->send_user_cash(accountServerID, nymSenderID, nymRecipientID, exportedCashPurse, retainedCopy);

		int32_t returnVal = mMadeEasy->VerifyMessageSuccess(response);

		if (1 != returnVal) {
			// It failed sending the cash to the recipient Nym.
			// Re-import strRetainedCopy back into the sender's cash purse.
			//
			bool bImported = opentxs::OTAPI_Wrap::Wallet_ImportPurse(accountServerID, accountAssetID, nymSenderID, retainedCopy);

			if (bImported) {
				DisplayStringEndl(cout, "Failed sending cash, but at least: success re-importing purse.\nServer: " + accountServerID + "\nAsset Type: " + accountServerID + "\nNym: " + nymSender + "\n\n");
			}
			else {
				DisplayStringEndl(cout, " Failed sending cash AND failed re-importing purse.\nServer: " + accountServerID + "\nAsset Type: " + accountServerID + "\nNym: " + nymSender + "\n\nPurse (SAVE THIS SOMEWHERE!):\n\n" + retainedCopy);
			}
		}
		else {
			DisplayStringEndl(cout, "Success in sending cash from " + nymSender + " to " + nymRecipient + " account " + account );
		}
	}

	return true;
}

bool cUseOT::CashShow(const string & account, bool dryrun) { // TODO make it work with longer version: asset, server, nym
	_fact("cash show for purse with account: " << account);
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);
	ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);

	int alignCenter = 15;

	cout << zkr::cc::fore::lightyellow << std::setw(alignCenter) <<"Server: " << zkr::cc::fore::green << ServerGetName(accountServerID) << endl
			<< zkr::cc::fore::lightyellow << std::setw(alignCenter) << "Asset: " << zkr::cc::fore::green << AssetGetName(accountAssetID) << endl
			<< zkr::cc::fore::lightyellow << std::setw(alignCenter) << "Nym: " << zkr::cc::fore::green << NymGetName(accountNymID) << endl;

	string purseValue = opentxs::OTAPI_Wrap::LoadPurse(accountServerID, accountAssetID, accountNymID); // returns NULL, or a purse

  if (purseValue.empty()) {
		 _erro("Unable to load purse. Does it even exist?");
		 DisplayStringEndl(cout, "Unable to load purse. Does it even exist?");
		 return false;
	}

  int64_t amount = opentxs::OTAPI_Wrap::Purse_GetTotalValue(accountServerID, accountAssetID, purseValue);
  cout << zkr::cc::fore::lightyellow << std::setw(alignCenter) << "Total value: " << zkr::cc::fore::green << opentxs::OTAPI_Wrap::FormatAmount(accountAssetID, amount) << zkr::cc::fore::console << endl;

	int32_t count = opentxs::OTAPI_Wrap::Purse_Count(accountServerID, accountAssetID, purseValue);
	if (count < 0) { // TODO check if integer?
		DisplayStringEndl(cout, "Error: Unexpected bad value returned from OT_API_Purse_Count.");
		_erro("Unexpected bad value returned from OT_API_Purse_Count.");
		return false;
	}

	if (count > 0) {

		cout << zkr::cc::fore::lightyellow << std::setw(alignCenter) << "Token count: " << zkr::cc::fore::green << count << endl;

		bprinter::TablePrinter tp(&std::cout);
		tp.AddColumn("ID", 4);
		tp.AddColumn("Value", 10);
		tp.AddColumn("Series", 10);
		tp.AddColumn("Valid From", 20);
		tp.AddColumn("Valid To", 20);
		tp.AddColumn("Status", 20);
		tp.PrintHeader();

		int32_t index = -1;
		while (count > 0) { // Loop through purse contents and display tokens
			--count;
			++index;  // on first iteration, this is now 0.

			string token = opentxs::OTAPI_Wrap::Purse_Peek(accountServerID, accountAssetID, accountNymID, purseValue);
			if (token.empty()) {
				_erro("OT_API_Purse_Peek unexpectedly returned NULL instead of token.");
				return false;
			}

			string newPurse = opentxs::OTAPI_Wrap::Purse_Pop(accountServerID, accountAssetID, accountNymID, purseValue);

			if (newPurse.empty()) {
				_erro("OT_API_Purse_Pop unexpectedly returned NULL instead of updated purse.\n");
				return false;
			}

			purseValue = newPurse;

			int64_t denomination = opentxs::OTAPI_Wrap::Token_GetDenomination(accountServerID, accountAssetID, token);
			int32_t series = opentxs::OTAPI_Wrap::Token_GetSeries(accountServerID, accountAssetID, token);
			time64_t validFrom = opentxs::OTAPI_Wrap::Token_GetValidFrom(accountServerID, accountAssetID, token);
			time64_t validTo = opentxs::OTAPI_Wrap::Token_GetValidTo(accountServerID, accountAssetID, token);
			time64_t time = opentxs::OTAPI_Wrap::GetTime();

			if (denomination < 0){
				_erro( "Error while showing purse: bad denomination");
				return false;
			}
			if (series < 0) {
				_erro("Error while showing purse: bad series");
				return false;
			}
			if (validFrom < 0) {
				_erro("Error while showing purse: bad validFrom");
				return false;
			}
			if (validTo < 0) {
				_erro("Error while showing purse: bad validTo");
				return false;
			}
			if (OT_TIME_ZERO > time) {
				_erro("Error while showing purse: bad time");
				return false;
			}

			string status = (time > validTo) ? "expired" : "valid";

			// Display token
			tp << ToStr(index) << ToStr(denomination) << ToStr(series) << ToStr(validFrom) << ToStr(validTo) << status;

		} // while
		tp.PrintFooter();
	} // if count > 0
	cout << zkr::cc::console << endl;
	return true;
}

bool cUseOT::CashWithdraw(const string & account, int64_t amount, bool dryrun) {
	_fact("cash withdraw " << account);
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);

	// Make sure the appropriate asset contract is available.
	string assetContract = opentxs::OTAPI_Wrap::LoadAssetContract(accountAssetID);

	if (assetContract.empty()) {
		string strResponse = mMadeEasy->retrieve_contract(mDefaultIDs.at(nUtils::eSubjectType::Server), accountNymID, accountAssetID);

		if (1 != mMadeEasy->VerifyMessageSuccess(strResponse)) {
			_erro( "Unable to retreive asset contract for nym " << accountNymID << " and server " << mDefaultIDs.at(nUtils::eSubjectType::Server) );
			DisplayStringEndl(cout, "Unable to retreive asset contract for nym " + accountNymID + " and server " + mDefaultIDs.at(nUtils::eSubjectType::Server) );
			return false;
		}

		assetContract = opentxs::OTAPI_Wrap::LoadAssetContract(accountAssetID);

		if (assetContract.empty()) {
			_erro("Failure: Unable to load Asset contract even after retrieving it.");
			DisplayStringEndl(cout, "Failure: Unable to load Asset contract even after retrieving it.");
			return false;
		}
	}

	// Make sure the unexpired mint file is available.
	string mint = mMadeEasy->load_or_retrieve_mint(mDefaultIDs.at(nUtils::eSubjectType::Server), accountNymID, accountAssetID);

	if (mint.empty()) {
		_erro("Failure: Unable to load or retrieve necessary mint file for withdrawal.");
		return false;
	}

	// Send withdrawal request
	string response = mMadeEasy->withdraw_cash ( mDefaultIDs.at(nUtils::eSubjectType::Server), accountNymID, accountID, amount);//TODO pass server as an argument

	// Check server response
	if (1 != mMadeEasy->VerifyMessageSuccess(response) ) {
		_erro("Failed trying to withdraw cash from account: " << AccountGetName(accountID) );
		return false;
	}
	_info("Successfully withdraw cash from account: " << AccountGetName(accountID));
	DisplayStringEndl(cout, "Successfully withdraw cash from account: " + AccountGetName(accountID));
	return true;
}

bool cUseOT::ChequeCreate(const string &fromAcc, const string & fromNym, const string &toNym, int64_t amount, const string &srv, const string &memo, bool dryrun) {
	_fact("cheque new \"" << fromAcc << "\" \"" << toNym << "\" " << srv);
	if (dryrun) return false;
	if (!Init()) return false;

	const ID fromAccID = AccountGetId(fromAcc);

	const ID fromNymID = NymGetId(fromNym);
	const ID toNymID = NymGetToNymId(toNym, fromNymID);
	const ID srvID = ServerGetId(srv);

	// TODO: Cheque is valid 6 months, time as param?
	auto now = OTTimeGetCurrentTime();
	const time64_t validFrom = now;
	const time64_t validTo = now + OT_TIME_SIX_MONTHS_IN_SECONDS;

	if (!mMadeEasy->retrieve_nym(srvID, fromNymID, true))
		return nUtils::reportError("Can't retrieve nym");

	if (!mMadeEasy->make_sure_enough_trans_nums(1, srvID, fromNymID))
		return nUtils::reportError("", "not enough transaction number", "Not enough transaction number!");

	const auto cheque = opentxs::OTAPI_Wrap::WriteCheque(srvID, amount, validFrom, validTo, fromAccID, fromNymID, memo,
			toNymID);

	// OTAPI_Wrap::WriteCheque should drop a notice
	// into the payments outbox, the same as it does when you "sendcheque" (after all, the same
	// resolution would be expected once it is cashed.)

	const auto status = mMadeEasy->VerifyMessageSuccess(cheque);
	if (status < 0) {
		_erro("status: " << status << " for cheque: " << cheque);
		return nUtils::reportError(ToStr(status), "status", "Creating cheque failed!");
	}

	auto ok = mMadeEasy->retrieve_account(srvID, fromNymID, fromAccID, true);
	PrintInstrumentInfo(cheque);
	return ok;
}

bool cUseOT::ChequeDiscard(const string & acc, const string & nym, const int32_t & index, bool dryrun) {
	_fact("cheque discard " << acc << " " << index);
	if (dryrun) return true;
	if (!Init()) return false;

	const auto accID = AccountGetId(acc);
	const auto nymID = NymGetId(nym);

	string cheque = "";
	if (index == -1) { // gets cheque from editor
		_dbg2("getting cheque from editor");
		cheque = GetText();

	} else { // gets cheque from outpayments
		const auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(nymID);
		if (count == 0)
			return false;
		cheque = opentxs::OTAPI_Wrap::GetNym_OutpaymentsContentsByIndex(nymID, index);
	}
	const auto srvID = opentxs::OTAPI_Wrap::Instrmnt_GetNotaryID(cheque);

	bool discard = opentxs::OTAPI_Wrap::DiscardCheque(srvID, nymID, accID, cheque);
	_info(discard);
	if(!discard) return nUtils::reportError("Error while discarding cheque");

	auto retrive = mMadeEasy->retrieve_account(srvID, nymID, accID, true);
	if(!retrive)
		cout << "Can't refresh account!" << endl;

	cout << zkr::cc::fore::lightgreen << "cheque discarded" << zkr::cc::console << endl;

	return true;
}

string cUseOT::ContractSign(const std::string & nymID, const std::string & contract){
	// FIXME can't sign contract with this (assetNew() functionality)
	if(!Init())
		return "";
	return opentxs::OTAPI_Wrap::AddSignature(nymID, contract);
}

bool cUseOT::ContractSign(const string & nym, const string & filename, const string & outfilename, bool dryrun) {
	_fact("contract sign " << nym << " " << filename << " " << outfilename);
	if (dryrun) return true;
	if (!Init()) return false;
	/**
	 * NOTE: The actual OT functionality (Use Cases) NEVER requires you to sign via this function.
     * Why not? because, anytime a signature is needed on something, the relevant OT API call will
     * require you to pass in the Nym, and the API already signs internally wherever it deems appropriate.
     * Thus, this function is only for advanced uses, for OT-Scripts, server operators, etc.
	*/

	string contract = GetInput(filename);
	auto signedContract = opentxs::OTAPI_Wrap::SignContract(NymGetId(nym), contract);

	try {
		nUtils::cEnvUtils envUtils;
		envUtils.WriteToFile(filename, signedContract);
	} catch(...) {
		return reportError("Can't sign contract");
	}

	return true;
}

bool cUseOT::MarketList(const string & srvName, const string & nymName, bool dryrun) {
	_fact("market ls " << srvName << ", " << nymName);
	if (dryrun) return true;
	if (!Init()) return false;

	ID serverID = ServerGetId(srvName);
	ID nymID = NymGetId(nymName);

	if (!opentxs::OTDB::Exists("markets", serverID, "market_data.bin")) {
		auto &nocol = zkr::cc::fore::console;
		auto &red = zkr::cc::fore::lightred;
		std::cout << red << "No markets available for " << srvName << " server" << nocol << std::endl;
		_erro("Can't open file: .ot/client_data/markets/" << serverID << "/market_data.bin");
		return false;
	}

	string marketList = mMadeEasy->get_market_list(serverID, nymID);
	_dbg2("market list:" << marketList);
	nUtils::DisplayStringEndl(cout, marketList);
	return true;
}

bool cUseOT::MintShow(const string & srvName, const string & nymName, const string & assetName, bool dryrun) {
	_fact("mint ls " << srvName << " " << nymName << " " << assetName);
	if(dryrun) return true;
	if(!Init())	return false;

	cout << "    Nym: " << nymName << endl;
	cout << "  Asset: " << assetName << endl;
	cout << " Server: " << srvName << endl;

	ID srvID = ServerGetId(srvName);
	ID nymID = NymGetId(nymName);
	ID assetID = AssetGetId(assetName);

	const string mint = mMadeEasy->load_or_retrieve_mint(srvID, nymID, assetID);

	auto &nocol = zkr::cc::fore::console;
	auto &blue = zkr::cc::fore::blue;

	cout << blue << mint << nocol << endl;
	return true;
}

vector<string> cUseOT::MsgGetAll() { ///< Get all messages from all Nyms. FIXME unused
	if(!Init())
	return vector<string> {};

	for(int i = 0 ; i < opentxs::OTAPI_Wrap::GetNymCount ();i++) {
		MsgDisplayForNym( NymGetName( opentxs::OTAPI_Wrap::GetNym_ID(i) ), false );
	}
	return vector<string> {};
}

bool cUseOT::MsgDisplayForNym(const string & nymName, bool dryrun) { ///< Get all messages from Nym.
	_fact("msg ls " << nymName);
	if (dryrun)
		return false;
	if (!Init())
		return false;
	string nymID = NymGetId(nymName);

	bprinter::TablePrinter tpIn(&std::cout);
	tpIn.AddColumn("ID", 4);
	tpIn.AddColumn("From", 20);
	tpIn.AddColumn("Content", 50);

	nUtils::DisplayStringEndl(cout,
			zkr::cc::fore::lightgreen + NymGetName(nymID) + zkr::cc::fore::console + "(" + nymID + ")");
	nUtils::DisplayStringEndl(cout, "INBOX");
	tpIn.PrintHeader();

	for (int i = 0; i < opentxs::OTAPI_Wrap::GetNym_MailCount(nymID); i++) {
		tpIn << i << NymGetName(opentxs::OTAPI_Wrap::GetNym_MailSenderIDByIndex(nymID, i))
				<< opentxs::OTAPI_Wrap::GetNym_MailContentsByIndex(nymID, i);
	}
	tpIn.PrintFooter();

	bprinter::TablePrinter tpOut(&std::cout);
	tpOut.AddColumn("ID", 4);
	tpOut.AddColumn("To", 20);
	tpOut.AddColumn("Content", 50);

	nUtils::DisplayStringEndl(cout, "OUTBOX");
	tpOut.PrintHeader();

	for (int i = 0; i < opentxs::OTAPI_Wrap::GetNym_OutmailCount(nymID); i++) {
		tpOut << i << NymGetRecipientName(opentxs::OTAPI_Wrap::GetNym_OutmailRecipientIDByIndex(nymID, i))
				<< opentxs::OTAPI_Wrap::GetNym_OutmailContentsByIndex(nymID, i);
	}
	tpOut.PrintFooter();
	return true;
}

bool cUseOT::MsgDisplayForNymInbox(const string & nymName, int msg_index, bool dryrun) {
	return cUseOT::MsgDisplayForNymBox( eBoxType::Inbox , nymName, msg_index, dryrun);
}

bool cUseOT::MsgDisplayForNymOutbox(const string & nymName, int msg_index, bool dryrun) {
	return cUseOT::MsgDisplayForNymBox( eBoxType::Outbox , nymName, msg_index, dryrun);
}

bool cUseOT::MsgDisplayForNymBox(eBoxType boxType, const string & nymName, int msg_index, bool dryrun) {
	_fact("msg ls box " << nymName << " " << msg_index);
	using namespace zkr;

	if (dryrun) return false;
	if (!Init()) return false;

	string nymID = NymGetId(nymName);
	string data_msg;
	auto &col1 = cc::fore::lightblue;
	auto &col2 = cc::fore::lightyellow;
	auto &nocol = cc::fore::console;
	auto &col4 = cc::fore::lightgreen;

	auto errMessage = [](int index, string type)->void {
		cout << cc::fore::red << "No message with index " << index << ". in " << type << cc::fore::console << endl;
		_mark("Lambda: Can't get message with index: " << index << " from " << type);
	};

	cout << col4;
	nUtils::DisplayStringEndl(cout, NymGetName(nymID) + "(" + nymID + ")");
	cout << col1;
	if (boxType == eBoxType::Inbox) {
		nUtils::DisplayStringEndl(cout, "INBOX");

		data_msg = opentxs::OTAPI_Wrap::GetNym_MailContentsByIndex(nymID, msg_index);

		if (data_msg.empty()) {
			errMessage(msg_index,"inbox");
			return false;
		}

		const string& data_from = NymGetRecipientName(opentxs::OTAPI_Wrap::GetNym_MailSenderIDByIndex(nymID, msg_index));
		const string& data_server = ServerGetName(opentxs::OTAPI_Wrap::GetNym_MailNotaryIDByIndex(nymID, msg_index));

		cout << col1 << "          To: " << col2 << nymName << endl;
		cout << col1 << "        From: " << col2 << data_from << endl;
		cout << col1 << " Data server: " << col2 << data_server << endl;

	} else if (boxType == eBoxType::Outbox) {
		nUtils::DisplayStringEndl(cout, "OUTBOX");
		data_msg = opentxs::OTAPI_Wrap::GetNym_OutmailContentsByIndex(nymID, msg_index);

		if (data_msg.empty() ) {
			errMessage(msg_index,"outbox");
			return false;
		}

		const string& data_to = NymGetName(opentxs::OTAPI_Wrap::GetNym_OutmailRecipientIDByIndex(nymID, msg_index));
		const string& data_server = ServerGetName(opentxs::OTAPI_Wrap::GetNym_OutmailNotaryIDByIndex(nymID, msg_index));

		// printing
		cout << col1 << "          To: " << col2 << nymName << endl;
		cout << col1 << "        From: " << col2 << data_to << endl;
		cout << col1 << " Data server: " << col2 << data_server << endl;

	}

	cout << col1 << "\n     Message: " << col2 << data_msg << endl;
	cout << col1 << "--- end of message ---" << nocol << endl;

	return true;
}

bool cUseOT::MsgSend(const string & nymSender, vector<string> nymRecipient, const string & subject, const string & msg, int prio, const string & filename, bool dryrun) {
	_fact("MsgSend " << nymSender << " to " << DbgVector(nymRecipient) << " msg=" << msg << " subj="<<subject<<" prio="<<prio);
	if(dryrun) return true;
	if(!Init()) return false;

	string outMsg;

	if ( !filename.empty() ) {
		_mark("try to load message from file: " << filename);
		nUtils::cEnvUtils envUtils;
		outMsg = envUtils.ReadFromFile(filename);
		_dbg3("loaded message: " << outMsg);
	}

	if ( msg.empty() && outMsg.empty()) {
		_dbg3("Message is empty, starting text editor");
		nUtils::cEnvUtils envUtils;
		outMsg = envUtils.Compose();
	}
	else if(outMsg.empty())
		outMsg = msg;

	if ( outMsg.empty() ) {
		_warn("Can't send the message: message is empty");
		return false;
	}

	ID senderID = NymGetId(nymSender);
	vector<ID> recipientID;
	for (auto varName : nymRecipient)
		recipientID.push_back( NymGetToNymId(varName, senderID) );

	for (auto varID : recipientID) {
		_dbg1("Sending message from " + senderID + " to " + varID + "using server " + nUtils::SubjectType2String(nUtils::eSubjectType::Server) );

		string strResponse = mMadeEasy->send_user_msg ( mDefaultIDs.at(nUtils::eSubjectType::Server), senderID, varID, outMsg);

		// -1 error, 0 failure, 1 success.
		if (1 != mMadeEasy->VerifyMessageSuccess(strResponse)) {
			_erro("Failed trying to send the message");
			return false;
		}
		_dbg3("Message from " + senderID + " to " + varID + " was sent successfully.");
	}
	_info("All messages were sent successfully.");
	return true;
}

bool cUseOT::MsgInCheckIndex(const string & nymName, const int32_t & index) {
	if(!Init())
			return false;
	if ( index >= 0 && index < opentxs::OTAPI_Wrap::GetNym_MailCount(NymGetId(nymName)) ) {
		return true;
	}
	return false;
}

bool cUseOT::MsgOutCheckIndex(const string & nymName, const int32_t & index) {
	if(!Init())
			return false;
	if ( index >= 0 && index < opentxs::OTAPI_Wrap::GetNym_OutmailCount(NymGetId(nymName)) ) {
		return true;
	}
	return false;
}

bool cUseOT::MsgInRemoveByIndex(const string & nymName, const int32_t & index, bool dryrun) {
	_fact("msg rm " << nymName << " index=" << index);
	if (dryrun) return false;
	if(!Init()) return false;
	if(opentxs::OTAPI_Wrap::Nym_RemoveMailByIndex (NymGetId(nymName), index)){
		_info("Message " << index << " removed successfully from " << nymName << " inbox");
	return true;
	}
	return false;
}

bool cUseOT::MsgOutRemoveByIndex(const string & nymName, const int32_t & index, bool dryrun) {
	_fact("msg rm-out " << nymName << " index=" << index);
	if (dryrun) return false;
	if(!Init()) return false;
	if( opentxs::OTAPI_Wrap::Nym_RemoveOutmailByIndex(NymGetId(nymName), index) ) {
		_info("Message " << index << " removed successfully from " << nymName << " outbox");
		return true;
	}
	return false;
}
bool cUseOT::NymNameExist(const string & nymName) {
	if(CheckIfExists(nUtils::eSubjectType::User, nymName)) return true;
	return AddressBookStorage::NymNameExist(nymName, NymGetAllIDs());
}

bool cUseOT::NymCheck(const string & nymName, bool dryrun) { // wip
	_fact("nym check " << nymName);
	if (dryrun) return false;
	if(!Init()) return false;

	ID nymID = NymGetId(nymName);

	string strResponse = mMadeEasy->check_nym( mDefaultIDs.at(nUtils::eSubjectType::Server), mDefaultIDs.at(nUtils::eSubjectType::User), nymID );
	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy->VerifyMessageSuccess(strResponse)) {
		_erro("Failed trying to download public key for nym: " << nymName << "(" << nymID << ")" );
		return false;
	}
	_info("Successfully downloaded user public key for nym: " << nymName << "(" << nymID << ")" );
	return true;
}

bool cUseOT::NymCreate(const string & nymName, bool registerOnServer, bool dryrun) {
	_fact("nym create " << nymName);
	if (dryrun) return false;
	if(!Init()) return false;

	if ( CheckIfExists(nUtils::eSubjectType::User, nymName) ) {
		_erro("Cannot create new nym: '" << nymName << "'. Nym with that name exists" );
		return false;
	}
	bool ok = true;
	int32_t nKeybits = 1024;
	string NYM_ID_SOURCE = ""; // TODO: check
	string ALT_LOCATION = "";
	string nymID = mMadeEasy->create_nym(nKeybits, NYM_ID_SOURCE, ALT_LOCATION);


	if (nymID.empty()) {
		_erro("Failed trying to create new Nym: " << nymName);
		return false;
	}
	/**
	 * Set the Name of the new Nym.
	 * Signer Nym? When testing, there is only one nym, so you just pass it twice.
	 * But in real production, a user will have a default signing nym, the same way that he might have a default signing key in PGP,
	 * and that must be passed in whenever he changes the name on any of the other nyms in his wallet.
 	 * (In order to properly sign and save the change.)
     */
	if ( !opentxs::OTAPI_Wrap::SetNym_Name(nymID, nymID, nymName) ) {
		_erro("Failed trying to name new Nym: " << nymID);
		return false;
	}
	_info("Nym " << nymName << "(" << nymID << ")" << " created successfully.");
	cout << zkr::cc::fore::lightgreen << "Nym " << nymName << "(" << nymID << ")" << " created successfully."
			<< zkr::cc::console << endl;

	mCache.mNyms.insert( std::make_pair(nymID, nymName) ); // insert nym to nyms cache

	try {
		auto defaultNym = NymGetDefault();
	} catch (...) {
		_info("No default nym, sets " << zkr::cc::bold << nymName << zkr::cc::console << " as default");
		cout << "Setting " << nymName << " as default nym." << endl;
		ok = ok && NymSetDefault(nymName, false);
	}

	if ( registerOnServer ) {
		try {
			ok = ok && NymRegister(nymName, "^" + ServerGetDefault(), false, dryrun);
		} catch(...) {
			ok = ok && nUtils::reportError("No default server, can't register nym");
		}
	}


	return ok;
}

bool cUseOT::NymExport(const string & nymName, const string & filename, bool dryrun) {
	if(dryrun) return true;
	if(!Init()) return false;

	std::string nymID = NymGetId(nymName);
	_fact("nym export: " << nymName << ", id: " << nymID);

	std::string exported = opentxs::OTAPI_Wrap::Wallet_ExportNym(nymID);
	// FIXME Bug in OTAPI? Can't export nym twice

	if(exported.empty()) {
		cout << zkr::cc::fore::lightred << "Can't export nym" << zkr::cc::console << endl;
		return false;
	}

	if(filename.empty())  {
		std::cout << zkr::cc::fore::lightblue << exported << zkr::cc::console << endl;
		return true;
	}
	nUtils::cEnvUtils envUtils;

	try {
		envUtils.WriteToFile(filename, exported);
	} catch(...) {
		return false;
	}

	cout << "Saved" << endl;
	return true;
}

bool cUseOT::NymImport(const string & filename, bool dryrun) {
	if(dryrun) return true;
	if(!Init()) return false;

	std::string toImport;
	nUtils::cEnvUtils envUtils;

	try {
		toImport = nUtils::GetInput(filename);
	} catch(...) {
		return nUtils::reportError("Can't import, empty input");
	}

	cout << zkr::cc::fore::lightred << "Error in opentxs library, can't import nym!" << zkr::cc::console << endl;
	return false; // XXX

	// FIXME: segfault!
	auto nym = opentxs::OTAPI_Wrap::Wallet_ImportNym(toImport);
	//cout << nym << endl;

	return true;
}

void cUseOT::NymGetAll(bool force) {
	if(!Init())
		return;

	int32_t cacheSize = mCache.mNyms.size();
	auto nymCount = opentxs::OTAPI_Wrap::GetNymCount();

	if (force || cacheSize != nymCount) { //TODO optimize?
		mCache.mNyms.clear();
		_dbg3("Reloading nyms cache");
		for(int i = 0 ; i < opentxs::OTAPI_Wrap::GetNymCount();i++) {
			string nym_ID = opentxs::OTAPI_Wrap::GetNym_ID (i);
			string nym_Name = opentxs::OTAPI_Wrap::GetNym_Name (nym_ID);

			mCache.mNyms.insert( std::make_pair(nym_ID, nym_Name) );
		}
	}
}

vector<string> cUseOT::NymGetAllIDs() {
	if(!Init())
		return vector<string> {};
	NymGetAll();
	vector<string> IDs;
	for (auto val : mCache.mNyms) {
		IDs.push_back(val.first);
	}
	return IDs;
}

vector<string> cUseOT::NymGetAllNames() {
	if(!Init())
		return vector<string> {};
	NymGetAll();
	vector<string> names;
	for (auto val : mCache.mNyms) {
		names.push_back(val.second);
	}
	return names;
}

bool cUseOT::NymDisplayAll(bool dryrun) {
	_fact("nym ls ");
	if(dryrun) return true;
	if(!Init()) return false;

	NymGetAll();
	nUtils::DisplayMap(cout, mCache.mNyms);// display Nyms cache

	return true;
}

ID cUseOT::NymGetDefault() {
	if(!Init())
		return "";
	auto defaultNym = mDefaultIDs.at(nUtils::eSubjectType::User);
	if(defaultNym.empty() || defaultNym=="-") throw string("No default nym!");
	return defaultNym;
}

string cUseOT::NymGetId(const string & nymName) { // Gets nym aliases and IDs begins with '^'
	if(!Init())
		return "";
	if(nymName.empty()) return "";

	if ( nUtils::checkPrefix(nymName) ) // nym ID
		return nymName.substr(1);
	else { // look in cache
		string key = nUtils::FindMapValue(mCache.mNyms, nymName);
		if(!key.empty()){
			_dbg3("Found nymID in cache");
			return key;
		}
	}

	for(int i = 0 ; i < opentxs::OTAPI_Wrap::GetNymCount ();i++) {
		string nymID = opentxs::OTAPI_Wrap::GetNym_ID (i);
		string nymName_ = opentxs::OTAPI_Wrap::GetNym_Name (nymID);
		if (nymName_ == nymName)
			return nymID;
	}
	return "";
}

string cUseOT::NymGetToNymId(const string & nym, const string & ownerNymID) {
	ID nymID = NymGetId(nym);
	return (nymID.empty())? AddressBookStorage::Get(ownerNymID)->nymGetID(nym) : nymID;
}

bool cUseOT::NymDisplayInfo(const string & nymName, bool dryrun) {
	_fact("nym info " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	cout << opentxs::OTAPI_Wrap::GetNym_Stats( NymGetId(nymName) );
	return true;
}

string cUseOT::NymGetName(const ID & nymID) {
	if(!Init())
		return "";
	if(nymID.empty()) return "";
	auto vector = NymGetAllIDs();
	if(std::find(vector.begin(), vector.end(), nymID) == vector.end()) { // nym not found, checing in address book
		_dbg1("nym not found, checking in address book");
		return AddressBookStorage::GetNymName(nymID, vector);
	}

	return opentxs::OTAPI_Wrap::GetNym_Name(nymID);
}

string cUseOT::NymGetRecipientName(const ID & nymID) {
	// if can't get nym name, returns id
	// comfortable to printing information about transaction, etc
	// in case situation whet nym WAS in addressBook but it was removed (or sth like that)

	auto name = NymGetName(nymID);
	if(name.empty()) {
		_warn("Can't find nym");
		return nymID;
	}
	return name;
}

bool cUseOT::NymRefresh(const string & nymName, bool all, bool dryrun) { //TODO arguments for server, all servers
	_fact("nym refresh " << nymName << " all?=" << all);
	if(dryrun) return true;
	if(!Init()) return false;

	int32_t serverCount = opentxs::OTAPI_Wrap::GetServerCount();
	if (all) {
		int32_t nymsRetrieved = 0;
		int32_t nymCount = opentxs::OTAPI_Wrap::GetNymCount();
		if (nymCount == 0){
			_warn("No Nyms to retrieve");
			return true;
		}

		for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex) { // FIXME Working for all available servers!
			for (int32_t nymIndex = 0; nymIndex < nymCount; ++nymIndex) {
				ID nymID = opentxs::OTAPI_Wrap::GetNym_ID(nymIndex);
				ID serverID = opentxs::OTAPI_Wrap::GetServer_ID(serverIndex);
				if (opentxs::OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID)) {
					if ( mMadeEasy->retrieve_nym(serverID, nymID, true) ){ // forcing download
						_info("Nym " + NymGetName(nymID) + "(" + nymID +  ")" + " retrieval success from server " + ServerGetName(serverID) + "(" + serverID +  ")");
						++nymsRetrieved;
					} else
					_erro("Nym " + NymGetName(nymID) + "(" + nymID +  ")" + " retrieval failure from server " + ServerGetName(serverID) + "(" + serverID +  ")");
				}
			}
		}
		string count = ToStr(nymsRetrieved) + "/" + ToStr(nymCount);
		if (nymsRetrieved == nymCount) {
			_info("All Nyms were successfully retrieved " << count);
			return true;
		} else if (nymsRetrieved == 0) {
			_erro("Nyms retrieval failure " << count);
			return false;
		} else {
			_erro("Some Nyms cannot be retrieved (not registered?) " << count); //TODO check if nym is regstered on server
			return true;
		}
	}
	else {
		ID nymID = NymGetId(nymName);
		for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex) { // Working for all available servers!
			ID serverID = opentxs::OTAPI_Wrap::GetServer_ID(serverIndex);
			if (opentxs::OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID)) {
				if ( mMadeEasy->retrieve_nym(serverID,nymID, true) ) { // forcing download
					_info("Nym " + nymName + "(" + nymID +  ")" + " retrieval success from server " + ServerGetName(serverID) + "(" + serverID +  ")");
					return true;
				}
				_warn("Nym " + nymName + "(" + nymID +  ")" + " retrieval failure from server " + ServerGetName(serverID) + "(" + serverID +  ")");
				return false;
			}
		}
	}
	return false;
}

bool cUseOT::NymRegister(const string & nymName, const string & serverName, bool force, bool dryrun) {
	_fact("nym register " << nymName << " on server " << serverName << " force=" << force);
	if(dryrun) return true;
	if(!Init()) return false;

	ID nymID = NymGetId(nymName);
	ID serverID = ServerGetId(serverName);

	if (!opentxs::OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID) || force) {
		string response = mMadeEasy->register_nym(serverID, nymID);

		if(mMadeEasy->VerifyMessageSuccess(response) != 1) {
			return reportError(response, "error register nym: " + nymName, "Can't register nym " + nymName);
		}

		nOT::nUtils::DisplayStringEndl(cout, response);
		_info("Nym " << nymName << "(" << nymID << ")" << " was registered successfully on server");
		cout << zkr::cc::fore::lightgreen << "Nym " << nymName << "(" << nymID << ")"
				<< " was registered successfully on server" << zkr::cc::console << endl;
		return true;
	}
	_info("Nym " << nymName << "(" << nymID << ")" << " was already registered" << endl);
	cout << "Nym " << nymName << " was already registered" << endl;
	return true;
}

bool cUseOT::NymRemove(const string & nymName, bool force, bool dryrun) {
	_fact("nym rm " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	string nymID = NymGetId(nymName);
	if ( opentxs::OTAPI_Wrap::Wallet_CanRemoveNym(nymID) || force) {
		if ( opentxs::OTAPI_Wrap::Wallet_RemoveNym(nymID) ) {
			cout << zkr::cc::fore::green << "Nym " << nymName << " was deleted successfully" << zkr::cc::console
					<< endl;
			_info(nymName << " deleted");
			mCache.mNyms.erase(nymID);
			return true;
		}
	}
	_warn("Nym " << nymName  <<  "(" << nymID << ")" << " cannot be removed");
	return false;
}

bool cUseOT::NymSetName(const ID & nymID, const string & newNymName) { //TODO: passing to function: const string & nymName, const string & signerNymName,
	if(!Init()) return false;

	if ( !opentxs::OTAPI_Wrap::SetNym_Name(nymID, nymID, newNymName) ) {
		_erro("Failed trying to set name " << newNymName << " to nym " << nymID);
		return false;
	}
	_info("Set Nym " << nymID << " name to " << newNymName);
	return true;
}

bool cUseOT::NymRename(const string & nym, const string & newNymName, bool dryrun) {
	_fact("nym rename from " << nym << " to " << newNymName);
	if(dryrun) return true;
	if(!Init()) return false;

	ID nymID = NymGetId(nym);

	if( NymSetName(nymID, newNymName) ) {
		_info("Nym " << NymGetName(nymID) << "(" << nymID << ")" << " renamed to " << newNymName);
		mCache.mNyms.insert( std::make_pair(nymID, newNymName) ); // insert nym to nyms cache
		return true;
	}
	_erro("Failed to rename Nym " << NymGetName(nymID) << "(" << nymID << ")" << " to " << newNymName);
	return false;
}

bool cUseOT::NymSetDefault(const string & nymName, bool dryrun) {
	_fact("nym set-default " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::User) = NymGetId(nymName);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

bool cUseOT::NymUnregister(const string &nym, const string & server, bool force, bool dryrun) {
	_fact("nym unregister nym=" << nym << " server=" << server << " force=" << force);
	if(dryrun) return true;
	if(!Init()) return false;

	const ID nymID = NymGetId(nym);
	const ID srvID = ServerGetId(server);

	if(!opentxs::OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, srvID)) {
		cout << zkr::cc::fore::red << "Nym " << nym << " wasn't register at server " << server << zkr::cc::console
				<< endl;
		if(force) cout << "Trying unregister nym" << endl;
		else return false;
	}

	auto unregister = opentxs::OTAPI_Wrap::unregisterNym(srvID, nymID);
	_note("unregister: " << unregister);
	return unregister == 1;

}
bool cUseOT::OutpaymentDiscard(const string & acc, const string & nym, const int32_t index, bool dryrun) {
	if(!Init())	return false;
	if(dryrun) return true;

	const ID nymID = NymGetId(nym);
	const auto outpayment = opentxs::OTAPI_Wrap::GetNym_OutpaymentsContentsByIndex(nymID, index);

	auto type = opentxs::OTAPI_Wrap::Instrmnt_GetType(outpayment);
	_dbg2(type);

	cout << zkr::cc::fore::cyan << "Discarding " << type << " for nym: " << nym << zkr::cc::console << " (" << nymID
			<< ")" << endl;
	cout << zkr::cc::fore::cyan << "Account: " << acc <<  zkr::cc::console << " (" << AccountGetId(acc)	<< ")" << endl;

	if(type == "CHEQUE") {
		return ChequeDiscard(acc, nym, index, false);
	}

	if(type == "VOUCHER") {
		return VoucherCancel(acc, nym, index, false);
	}

	if( type == "INVOICE" || type == "PURSE") {
		return reportError("Sorry, discarding " + type + " is not implemented yet.");
	}

	return reportError("Unknown type (" + type + ")");
}

bool cUseOT::OutpaymentCheckIndex(const string & nymName, const int32_t & index) {
	if(!Init()) return false;
	if(index < 0) return false;
	if(index < opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(NymGetId(nymName)) )
		return true;

	return false;
}

int32_t cUseOT::OutpaymantGetCount(const string & nym) {
	const auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(NymGetId(nym));
	return (count <= 0) ? -1 : count;
}

bool cUseOT::OutpaymentDisplay(const string & nym, bool dryrun) {
	_fact("nym-outpayment ls " << nym);
	if (dryrun) return true;
	if (!Init()) return false;

	ID nymID = NymGetId(nym);

	auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(nymID);

	if(count <= 0) {
		cout << zkr::cc::fore::lightblue << "No outpayments for nym: " << nym << zkr::cc::console << endl;
		return true;
	}

	cout << "Printing outpayments for nym: " << zkr::cc::fore::lightblue << nym << zkr::cc::console << " (" << count << ")" << endl;
	auto color = zkr::cc::fore::lightyellow;
	auto nocolor = zkr::cc::console;
	auto err = zkr::cc::fore::lightred;

	bprinter::TablePrinter table(&std::cout);
	table.SetContentColor(nocolor);

	table.AddColumn("Index", 5);
	table.AddColumn("Recipient", 20);
	table.AddColumn("Type", 10);
	table.AddColumn("Asset", 20);
	table.AddColumn("Amount", 10);
	table.PrintHeader();

	for (int32_t i = 0; i<count; i++) {
		auto instr = opentxs::OTAPI_Wrap::GetNym_OutpaymentsContentsByIndex(nymID,i);
		auto to = NymGetRecipientName(opentxs::OTAPI_Wrap::GetNym_OutpaymentsRecipientIDByIndex(nymID,i));
		auto type = opentxs::OTAPI_Wrap::Instrmnt_GetType(instr);
		auto asset = AssetGetName(opentxs::OTAPI_Wrap::Instrmnt_GetInstrumentDefinitionID(instr));
		auto amount = opentxs::OTAPI_Wrap::Instrmnt_GetAmount(instr);
		auto srvID = opentxs::OTAPI_Wrap::Instrmnt_GetNotaryID(instr);

		if(to == nym) table.SetContentColor(color);
		else table.SetContentColor(nocolor);
		if(!opentxs::OTAPI_Wrap::Nym_VerifyOutpaymentsByIndex(nymID, i))
			table.SetContentColor(err);


		table << i << to << type << asset << amount;
	}
	table.PrintFooter();
	cout << zkr::cc::console << endl;

	return true;
}

bool cUseOT::OutpaymentRemove(const string & nym, const int32_t & index, bool all, bool dryrun) {
	_fact("outpayment remove " << index << " " << nym << " all=" << all);
	if(dryrun) return true;
	if(!Init()) return false;

	const auto nymID = NymGetId(nym);
	const auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(nymID);

	if(count == 0) {
		cout << zkr::cc::fore::yellow << "Can't remove. Outpayment box is empty!" << zkr::cc::console << endl;
		return false;
	}

	bool ok = true;

	if(all) {
		for(int32_t i = count - 1; i >= 0; --i)
			ok = ok && opentxs::OTAPI_Wrap::Nym_RemoveOutpaymentsByIndex(nymID, i);
	}
	else
		ok = opentxs::OTAPI_Wrap::Nym_RemoveOutpaymentsByIndex(nymID, index);

	if(ok) cout << zkr::cc::fore::lightgreen << "Operation successful" << zkr::cc::console << endl;

	return ok;
}

bool cUseOT::OutpaymentSend(const string & senderNym, const string & recipientNym, int32_t index, bool all, bool dryrun) {
	_fact("payment send " << recipientNym << " " << senderNym << " " << index << " " << all);
	if (dryrun)	return true;
	if (!Init()) return false;

	const auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(NymGetId(senderNym));
	if(count <= 0 ) {
		cout << "Empty payment box, aborting" << endl;
		return false;
	}
	bool sent = true;
	if(all) {
		cout << "sending all payments to " << recipientNym << endl;
		bool sent = true;
		for(auto i=count-1; i>= 0; --i)
			sent = sent && OutpaymentSend(senderNym, recipientNym, i, false);
		return sent;
	} else {
		sent = OutpaymentSend(senderNym, recipientNym, index, false);
	}
	return sent;
}

bool cUseOT::OutpaymentSend(const string & senderNym, const string & recipientNym, int32_t index, bool dryrun) {
	if (dryrun)	return true;
	if (!Init()) return false;

	const ID senderNymID = NymGetId(senderNym);
	const ID recNymID = NymGetToNymId(recipientNym, senderNymID);

	const auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(senderNymID);

	if (index < 0 || index >= count) {
		_erro("index: " << index << ", count: " << count);
		cout << zkr::cc::fore::lightred << "Unable to load outpayment for nym " << senderNym << " with index: " << index
				<< zkr::cc::fore::console << endl;
		return false;
	}

	const auto payment = opentxs::OTAPI_Wrap::GetNym_OutpaymentsContentsByIndex(senderNymID, index);

	cout << endl << endl;
	PrintInstrumentInfo(payment);

	const ID srvID = opentxs::OTAPI_Wrap::Instrmnt_GetNotaryID(payment);

	cout << zkr::cc::fore::yellow << "\n\n Sending to nym: " << recipientNym << zkr::cc::fore::console << endl;

	auto send = mMadeEasy->send_user_payment(srvID, senderNymID, recNymID, payment);

	auto refreshSender = mMadeEasy->retrieve_nym(srvID, senderNymID, true);
	auto status = mMadeEasy->VerifyMessageSuccess(send);

	if(status < 0) {
        auto harvest = opentxs::OTAPI_Wrap::Msg_HarvestTransactionNumbers(send, senderNymID, false, false, false, false, false); // XXX
        _dbg1("harvest = " << harvest);
		return nUtils::reportError(ToStr(status), "status", "Can't send this payment");
	}

	return refreshSender;
}

bool cUseOT::OutpaymentShow(const string & nym, int32_t index, bool dryrun) {
	_fact("outpayment show " << index << " " << nym);
	if (dryrun)
		return true;
	if (!Init())
		return false;

	const ID nymID = NymGetId(nym);
	const auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(nymID);

	if (count <= 0) {
		cout << zkr::cc::fore::lightred << "Empty outpayment box!" << zkr::cc::console << endl;
		return false;
	}

	auto outpayment = opentxs::OTAPI_Wrap::GetNym_OutpaymentsContentsByIndex(nymID, index);
	auto srv = opentxs::OTAPI_Wrap::GetNym_OutpaymentsNotaryIDByIndex(nymID, index);
	auto rec = opentxs::OTAPI_Wrap::GetNym_OutpaymentsRecipientIDByIndex(nymID, index);

	cout << zkr::cc::fore::lightblue << outpayment << zkr::cc::console << endl;

	PrintInstrumentInfo(outpayment);

	(opentxs::OTAPI_Wrap::Nym_VerifyOutpaymentsByIndex(nymID, index)) ?
			cout << zkr::cc::fore::green << "\nverification successfull" :
			cout << zkr::cc::fore::lightred << "\nverification failed";

	cout << zkr::cc::console << endl;

	return true;
}

bool cUseOT::PaymentAccept(const string & account, int64_t index, bool all, bool dryrun) {
	_fact("payment accept " << account << " " << index << " " << all);
	if (dryrun) return false;
	if (!Init()) return false;

	auto accID = AccountGetId(account);
	auto srvID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accID);
	auto nymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accID);

	if (all) {
		string paymentInbox = opentxs::OTAPI_Wrap::LoadPaymentInbox(srvID, nymID); // Returns NULL, or an inbox.
		if (paymentInbox.empty())
			return nUtils::reportError("accept_from_paymentbox: OT_API_LoadPaymentInbox Failed.");

		_dbg3("Get size of inbox ledger");

		int32_t nCount = opentxs::OTAPI_Wrap::Ledger_GetCount(srvID, nymID, nymID, paymentInbox);
		bool ok = true;

		for(int32_t i=nCount-1; i>=0; --i) {
			ok = ok && PaymentAccept(account, i, false);
		}
		return ok;
	} else
		return PaymentAccept(account, index, false);
}

bool cUseOT::PaymentAccept(const string & account, int64_t index, bool dryrun) {
	/*
		 case ("CHEQUE")
		 case ("VOUCHER")
		 case ("INVOICE")
		 case ("PURSE")
		 TODO make it work with longer version: asset, server, nym
		 TODO accept all payments
		 TODO accept various instruments types
	 */
	_fact("Accept incoming payment nr " << index << " for account " << account);
	if (dryrun)
		return false;
	if (!Init())
		return false;

	const ID accountID = AccountGetId(account);
	const ID accountNymID = opentxs::OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	const ID accountAssetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accountID);
	const ID accountServerID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accountID);


	_dbg1("nym: " << NymGetName(accountNymID) << ", acc: " << account);

	/*
		 payment instrument and myacct must both have same asset type
		 int32_t nAcceptedPurses = accept_from_paymentbox(strMyAcctID, strIndices, "PURSE");
		 Voucher is already interpreted as a form of cheque, so this is redundant.

		 int32_t nAcceptedVouchers = accept_from_paymentbox(strMyAcctID, strIndices, "VOUCHER")
		 int32_t nAcceptedCheques = accept_from_paymentbox(strMyAcctID, strIndices, "CHEQUE");
	 */

	_dbg3("Loading payment inbox");

	string paymentInbox = opentxs::OTAPI_Wrap::LoadPaymentInbox(accountServerID, accountNymID); // Returns NULL, or an inbox.

	if (paymentInbox.empty())
		return nUtils::reportError("accept_from_paymentbox: OT_API_LoadPaymentInbox Failed.");

	_dbg3("Get size of inbox ledger");

	int32_t nCount = opentxs::OTAPI_Wrap::Ledger_GetCount(accountServerID, accountNymID, accountNymID, paymentInbox);
	if (nCount < 0)
		return nUtils::reportError("Unable to retrieve size of payments inbox ledger. (Failure.)\n");

	if (nCount == 0)
		return nUtils::reportError("Empty payment box");

	if (index == -1)
			index = nCount - 1;
	_info("index = " << index << "nCount = " << nCount);

	ASRT(index >= 0);
/*
	int32_t nIndicesCount = VerifyStringVal(strIndices) ? opentxs::OTAPI_Wrap::NumList_Count(strIndices) : 0;

	 Either we loop through all the instruments and accept them all, or
	 we loop through all the instruments and accept the specified indices.

	 (But either way, we loop through all the instruments.)

	for (int32_t nIndex = (nCount - 1); nIndex >= 0; --nIndex) { // Loop from back to front,NymG so if any are removed, the indices remain accurate subsequently.
		 bool bContinue = false;

		 // - If indices are specified, but the current index is not on
		 //   that list, then continue...
		 //
		 // - If NO indices are specified, accept all the ones matching MyAcct's asset type.
		 //
		 if ((nIndicesCount > 0) && !opentxs::OTAPI_Wrap::NumList_VerifyQuery(strIndices, ToStr(nIndex))) {
				 //          continue  // apparently not supported by the language.
				 bContinue = true;
		 }
		 else if (!bContinue) {
				 int32_t nHandled = handle_payment_index(strMyAcctID, nIndex, strPaymentType, paymentInbox);
		 }
	}
*/
	_dbg3("Get payment instrument");

	// strInbox is optional and avoids having to load it multiple times. This function will just load it itself, if it has to.

	string instrument = mMadeEasy->get_payment_instrument(accountServerID, accountNymID, index, paymentInbox);
	if (instrument.empty())
		return nOT::nUtils::reportError("Unable to get payment instrument based on index: " + ToStr(index));

	_dbg3("Get type of instrument");
	string strType = opentxs::OTAPI_Wrap::Instrmnt_GetType(instrument);

	if (strType.empty())
		return nOT::nUtils::reportError("Unable to determine instrument's type. Expected CHEQUE, VOUCHER, INVOICE, or (cash) PURSE");


	// If there's a payment type,
	// and it's not "ANY", and it's the wrong type,
	// then skip this one.

	// But we need to make sure the invoice is made out to strMyNymID (or to no one.)
	// Because if it IS endorsed to a Nym, and strMyNymID is NOT that nym, then the
	// transaction will fail. So let's check, before we bother sending it...

	_dbg3("Get recipient user id");

	// Not all instruments have a specified recipient. But if they do, let's make
	// sure the Nym matches.

	string recipientNymID = opentxs::OTAPI_Wrap::Instrmnt_GetRecipientNymID(instrument);

	_dbg1("recNym: " << NymGetRecipientName(recipientNymID));
	/*
	 if (!recipientNymID.empty() && !CheckIfExists(eSubjectType::User, recipientNymID)) {
	 _erro("The instrument " + ToStr(index) + " is endorsed to a specific recipient ("
	 + NymGetName(recipientNymID) + ") and that doesn't match the account's owner Nym ("
	 + AccountGetName(accountNymID) + "). (Skipping.)");

	 opentxs::OTAPI_Wrap::Output(0, "The instrument " + ToStr(index) + " is endorsed to a specific recipient (" + recipientNymID + ") and that doesn't match the account's owner Nym (" + accountNymID + "). (Skipping.) \n");
	 return false;
	 }*/
	_dbg3("Get instrument assetID");
	string instrumentAssetType = opentxs::OTAPI_Wrap::Instrmnt_GetInstrumentDefinitionID(instrument);

	if (accountAssetID != instrumentAssetType) {
		return nOT::nUtils::reportError(
				"The instrument at index " + ToStr(index) + " has a different asset type than the selected account");
	}

	_dbg3("Check if instrument is valid");

	time64_t tFrom = opentxs::OTAPI_Wrap::Instrmnt_GetValidFrom(instrument);
	time64_t tTo = opentxs::OTAPI_Wrap::Instrmnt_GetValidTo(instrument);
	time64_t tTime = opentxs::OTAPI_Wrap::GetTime();

	if (tTime < tFrom)
		return nUtils::reportError("The instrument at index " + ToStr(index) + " is not yet within its valid date range");

	if (tTo > OT_TIME_ZERO && tTime > tTo) {
		opentxs::OTAPI_Wrap::Output(0,
				"The instrument at index " + ToStr(index) + " is expired. (Moving it to the record box.)\n");
		nOT::nUtils::reportError("The instrument at index " + ToStr(index) + " is expired. (Moving it to the record box.)");
		// Since this instrument is expired, remove it from the payments inbox, and move to record box.
		_dbg3("Expired instrument - moving into record inbox");
		// Note: this harvests
		if ((index >= 0) && opentxs::OTAPI_Wrap::RecordPayment(accountServerID, accountNymID, true, // bIsInbox = true;
				index, true)) { // bSaveCopy = true. (Since it's expired, it'll go into the expired box.)
			return false;
		}
		return false;
	}

	/**
	 * TODO, IMPORTANT: After the below deposits are completed successfully, the wallet
	 * will receive a "successful deposit" server reply. When that happens, OT (internally)
	 * needs to go and see if the deposited item was a payment in the payments inbox. If so,
	 * it should REMOVE it from that box and move it to the record box.
	 *
	 * That's why you don't see me messing with the payments inbox even when these are successful.
	 * They DO need to be removed from the payments inbox, but just not here in the script. (Rather,
	 * internally by OT itself.)
	 */

	PrintInstrumentInfo(instrument);
	if ("CHEQUE" == strType || "VOUCHER" == strType) {
		const auto deposit = mMadeEasy->deposit_cheque(accountServerID, accountNymID, accountID, instrument);
		const auto status = mMadeEasy->VerifyMessageSuccess(deposit);

		_dbg3(deposit);

		auto refreshRecipient = mMadeEasy->retrieve_account(accountServerID, accountNymID, accountID, true);
		if (!refreshRecipient)
			_warn("Can't refresh recipient account: " << AccountGetName(accountID) << ", owner nym: " << NymGetName(accountNymID));

		if(status < 0)
			return nUtils::reportError("Can't accept this payment!");
		return true;
	}

	if ("INVOICE" == strType) { // TODO: implement this
		return nUtils::reportError("Not implemented yet");
	}

	if ("PURSE" == strType) {
		_dbg3("Type of instrument: " << strType);
		int32_t nDepositPurse = CashDeposit(accountID, accountNymID, accountServerID, instrument);
		// strIndices is left blank in this case
		// if nIndex !=  -1, go ahead and call RecordPayment on the purse at that index, to
		// remove it from payments inbox and move it to the recordbox.
		//
		if ((index != -1) && (1 == nDepositPurse)) {
			auto recorded = opentxs::OTAPI_Wrap::RecordPayment(accountServerID, accountNymID, true, //bIsInbox=true
					index, true); // bSaveCopy=true.
			_dbg3("recorded: " << recorded);
			if(!recorded) _warn("can't record this payment!");
		}
		return true;
	}
	opentxs::OTAPI_Wrap::Output(0, "\nSkipping this instrument: Expected CHEQUE, VOUCHER, INVOICE, or (cash) PURSE.\n");

	return false;
}

bool cUseOT::PaymentShow(const string & nym, const string & server, bool dryrun) { // TODO make it work with longer version: asset, server, nym
	_fact("Show incoming payments inbox for nym: " << nym << " and server: " << server);
	if (dryrun) return false;
	if(!Init()) return false;

	ID nymID = NymGetId(nym);
	ID serverID = ServerGetId(server);

	string paymentInbox = opentxs::OTAPI_Wrap::LoadPaymentInbox(serverID, nymID); // Returns NULL, or an inbox.

	if (paymentInbox.empty()) {
		DisplayStringEndl(cout, "Unable to load the payments inbox (probably doesn't exist yet.)\n(Nym/Server: " + nym + " / " + server + " )");
		return false;
	}

  int32_t count = opentxs::OTAPI_Wrap::Ledger_GetCount(serverID, nymID, nymID, paymentInbox);
	if (count > 0) {
		opentxs::OTAPI_Wrap::Output(0, "Show payments inbox (Nym/Server)\n( " + nym + " / " + server + " )\n");
		bprinter::TablePrinter tp(&std::cout);
		tp.AddColumn("ID", 4);
		tp.AddColumn("Amount", 10);
		tp.AddColumn("Type", 10);
		tp.AddColumn("Txn", 10);
		tp.AddColumn("Asset Type", 60);
		tp.PrintHeader();

		for (int32_t index = 0; index < count; ++index)
		{
			string instrument = opentxs::OTAPI_Wrap::Ledger_GetInstrument(serverID, nymID, nymID, paymentInbox, index);

			if (instrument.empty()) {
				 opentxs::OTAPI_Wrap::Output(0, "Failed trying to get payment instrument from payments box.\n");
				 return false;
			}
			string transaction = opentxs::OTAPI_Wrap::Ledger_GetTransactionByIndex(serverID, nymID, nymID, paymentInbox, index);

			int64_t transNumber = opentxs::OTAPI_Wrap::Ledger_GetTransactionIDByIndex(serverID, nymID, nymID, paymentInbox, index);

			string transactionNumber = ToStr(transNumber);

			// int64_t refNum = opentxs::OTAPI_Wrap::Transaction_GetDisplayReferenceToNum(serverID, nymID, nymID, transaction); // FIXME why we need this?

			int64_t amount = opentxs::OTAPI_Wrap::Instrmnt_GetAmount(instrument);
			string instrumentType = opentxs::OTAPI_Wrap::Instrmnt_GetType(instrument);
			string instrAssetID = opentxs::OTAPI_Wrap::Instrmnt_GetInstrumentDefinitionID(instrument);
			string senderNymID = opentxs::OTAPI_Wrap::Instrmnt_GetSenderNymID(instrument);
			string senderAccountID = opentxs::OTAPI_Wrap::Instrmnt_GetSenderAcctID(instrument);
			string recipientNymID = opentxs::OTAPI_Wrap::Instrmnt_GetRecipientNymID(instrument);
			string recipientAccountID = opentxs::OTAPI_Wrap::Instrmnt_GetRecipientAcctID(instrument);

			string finalNymID = senderNymID.empty() ? senderNymID : recipientNymID;
			string finalAccountID = senderAccountID.empty() ? senderAccountID : recipientAccountID;

			bool hasAmount = amount >= 0;
			bool hasAsset = !instrAssetID.empty();

			string formattedAmount = (hasAmount && hasAsset) ? opentxs::OTAPI_Wrap::FormatAmount(instrAssetID, amount) : "UNKNOWN_AMOUNT";

 			string assetDescr = AssetGetName(instrAssetID) + "(" + instrAssetID + ")";
			string recipientDescr = recipientNymID; // FIXME Is recipient needed in purse?

			tp << ToStr(index) <<  formattedAmount << instrumentType << transactionNumber << assetDescr;
		} // for
		tp.PrintFooter();
	}


	return true;
}

bool cUseOT::PaymentDiscard(const string & nym, const string & index, bool all, bool dryrun) {
	_mark("Discard incoming payment");

	if (dryrun) return false;
	if(!Init()) return false;

	if(nym.empty()) {
		auto nymID = NymGetDefault();
		string paymentInbox = opentxs::OTAPI_Wrap::LoadPaymentInbox(ServerGetDefault(), nymID); // Returns NULL, or an inbox.
		int32_t count = opentxs::OTAPI_Wrap::Ledger_GetCount(ServerGetDefault(), nymID, nymID, paymentInbox);
		_dbg1("Count: " << count);
		count --;
        mMadeEasy->discard_incoming_payments(ServerGetDefault(), nymID, std::to_string(count));
		return true;
	}

	if(index.empty()) {
		auto nymID = NymGetId(nym);
		string paymentInbox = opentxs::OTAPI_Wrap::LoadPaymentInbox(ServerGetDefault(), nymID); // Returns NULL, or an inbox.
		int32_t count = opentxs::OTAPI_Wrap::Ledger_GetCount(ServerGetDefault(), nymID, nymID, paymentInbox);
		_dbg1("Count: " << count);
		count --;
        mMadeEasy->discard_incoming_payments(ServerGetDefault(), nymID, std::to_string(count));
		return true;
	}

	_dbg1("All = " << all);
	_dbg1("Dryrun = " << dryrun);

	ID nymID = NymGetId(nym);
	if(!all) {
		_dbg1("Not all");
		mMadeEasy->discard_incoming_payments(ServerGetDefault(), nymID, index);
		return true;
	}

	_dbg2("string paymentInbox = ");
	string paymentInbox = opentxs::OTAPI_Wrap::LoadPaymentInbox(ServerGetDefault(), nymID); // Returns NULL, or an inbox.
	_dbg2("int32_t count =");
	if (paymentInbox.empty()) {
		 opentxs::OTAPI_Wrap::Output(0, "\n\n accept_from_paymentbox:  OT_API_LoadPaymentInbox Failed.\n\n");
		 return false;
	}
	int32_t count = opentxs::OTAPI_Wrap::Ledger_GetCount(ServerGetDefault(), nymID, nymID, paymentInbox);
	_dbg2(" PaymentDiscard() count =  " << count);
	for (int32_t i = 0; i < count; i++) {
        mMadeEasy->discard_incoming_payments(ServerGetDefault(), nymID, std::to_string(0));
	}

	return true;
}

bool cUseOT::PaymentDiscardAll(bool dryrun) {
	_fact("Discard incoming payment for all nyms");

	if (dryrun) return false;
	if(!Init()) return false;

	vector<string> nymIDs = NymGetAllIDs();
	for(auto nymID : nymIDs) {
		_dbg1("Discard for nym: " << NymGetName(nymID));
		PaymentDiscard(NymGetName(nymID), " ", true, false);
	}

	return true;
}

bool cUseOT::PurseCreate(const string & serverName, const string & asset, const string & ownerName, const string & signerName, bool dryrun) {
	_fact("purse create ");
	if(dryrun) return true;
	if(!Init()) return false;

	string serverID = ServerGetId(serverName);
	string assetTypeID = AssetGetId(asset);
	string ownerID = NymGetId(ownerName);
	string signerID = NymGetId(signerName);

	string purse = opentxs::OTAPI_Wrap::CreatePurse(serverID,assetTypeID,ownerID,signerID);
	nUtils::DisplayStringEndl(cout, purse);

	//	bool opentxs::OTAPI_Wrap::SavePurse	(	const std::string & 	SERVER_ID,
	//	const std::string & 	ASSET_TYPE_ID,
	//	const std::string & 	USER_ID,
	//	const std::string & 	THE_PURSE
	//	)
	bool result = opentxs::OTAPI_Wrap::SavePurse(serverID,assetTypeID,ownerID,purse);
	_info("saving: " << result) ;

	return true;
}


bool cUseOT::PurseDisplay(const string & serverName, const string & asset, const string & nymName, bool dryrun) {
	_fact("purse show= " << serverName << " " << asset << " " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	string serverID = ServerGetId(serverName);
	string assetTypeID = AssetGetId(asset);
	string nymID = NymGetId(nymName);

	string result = opentxs::OTAPI_Wrap::LoadPurse(serverID,assetTypeID,nymID);
	nUtils::DisplayStringEndl(cout, result);
	_info(result);
	//  TODO:
//	std::string opentxs::OTAPI_Wrap::LoadPurse	(	const std::string & 	SERVER_ID,
//	const std::string & 	ASSET_TYPE_ID,
//	const std::string & 	USER_ID
//	)

	return true;
}

bool cUseOT::RecordClear(const string &acc, bool all, bool dryrun) {
	if (dryrun)
		return true;
	if (!Init())
		return false;

	const auto nym = AccountGetNym(acc);
	const auto nymID = NymGetId(nym);
	const auto accID = AccountGetId(acc);
	const auto srvID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accID);
	const auto srv = ServerGetName(srvID);

	const auto recordBox = opentxs::OTAPI_Wrap::LoadRecordBox(srvID, nymID, accID);

	cout << endl;
	cout << "    Nym: " << nym << endl;
	cout << "Account: " << acc << endl;
	cout << " Server: " << srv << endl << endl;


	if (recordBox.empty()) {
		cout << zkr::cc::fore::yellow << "Recordbox is empty" << zkr::cc::console << endl;
		return false;
	}

	bool cleared;

	(all) ? cleared = opentxs::OTAPI_Wrap::ClearRecord(srvID, nymID, accID, 0, true) :
			cleared = opentxs::OTAPI_Wrap::ClearExpired(srvID, nymID, 0, true);

	return cleared;
}

bool cUseOT::RecordDisplay(const string &acc, bool noVerify, bool dryrun) {
	_fact("recordbox ls " << acc);
	if (dryrun) return true;
	if (!Init()) return false;

	const auto nym = AccountGetNym(acc);
	const auto nymID = NymGetId(nym);
	const auto accID = AccountGetId(acc);
	const auto srvID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accID);
	const auto srv = ServerGetName(srvID);

	const auto recordBox =
			(noVerify) ?
					opentxs::OTAPI_Wrap::LoadRecordBoxNoVerify(srvID, nymID, accID) :
					opentxs::OTAPI_Wrap::LoadRecordBox(srvID, nymID, accID);

	_dbg3(recordBox);
	cout << endl;
	cout << "    Nym: " << nym << endl;
	cout << "Account: " << acc << endl;
	cout << " Server: " << srv << endl << endl;


	if(recordBox.empty()) {
		cout << zkr::cc::fore::yellow << "Recordbox is empty" << zkr::cc::console << endl;
		return false;
	}

	const auto count = opentxs::OTAPI_Wrap::Ledger_GetCount(srvID, nymID, accID, recordBox);

	_dbg2(count);

	cout << "  RECORDBOX" << endl;
	bprinter::TablePrinter table(&std::cout);
	table.SetContentColor(zkr::cc::console);

	table.AddColumn("ID", 5);
	table.AddColumn("Type", 20);
	table.AddColumn("Sender", 20);
	table.AddColumn("Recipient", 20);
	table.AddColumn("Amount", 10);
	table.PrintHeader();

	bool ok = true;

	for (int32_t i = 0; i < count; ++i) {
		const auto transaction = opentxs::OTAPI_Wrap::Ledger_GetTransactionByIndex(srvID, nymID, accID, recordBox, i);
		if (transaction.empty()) { // handle error
			ok = false;
			table.SetContentColor(zkr::cc::fore::lightred);
			table << "ERROR" << "ERROR" << "ERROR" << "ERROR" << "ERROR";
		} else {
			const auto id = opentxs::OTAPI_Wrap::Ledger_GetTransactionIDByIndex(srvID, nymID, accID, recordBox, i);
			const auto type = opentxs::OTAPI_Wrap::Transaction_GetType(srvID, nymID, accID, transaction);
			const auto senderNymID = opentxs::OTAPI_Wrap::Transaction_GetSenderNymID(srvID, nymID, accID, transaction);
			const auto recipientNymID = opentxs::OTAPI_Wrap::Transaction_GetRecipientNymID(srvID, nymID, accID,
					transaction);
			const auto amount = opentxs::OTAPI_Wrap::Transaction_GetAmount(srvID, nymID, accID, transaction);

			(opentxs::OTAPI_Wrap::Transaction_IsCanceled(srvID, nymID, accID, transaction)) ?
					table.SetContentColor(zkr::cc::fore::yellow) : table.SetContentColor(zkr::cc::console);

			// if sender or recipient nym is empty, print space
			const auto senderNym = (senderNymID.empty()) ? "???" : NymGetName(senderNymID);
			const auto recipientNym = (recipientNymID.empty()) ? "???" : NymGetRecipientName(recipientNymID);

			table << id << type << senderNym << recipientNym << amount;
		}
	}
	table.PrintFooter();
	return ok;
}

bool cUseOT::ServerAdd(const string &filename, bool dryrun) {
	_fact("server add " << filename);
	if(dryrun) return true;
	if(!Init()) return false;

	string contract = "";
	nUtils::cEnvUtils envUtils;

	try {
		contract = nUse::GetInput(filename);
	} catch (const string & message) {
		return nUtils::reportError("", message,
				"Provided contract was empty");
	}

	if( !opentxs::OTAPI_Wrap::AddServerContract(contract) ) {
		return nUtils::reportError("Failure to add server");
	}

	_info("Server added");
	cout << "Server added" << endl;

	try {
		auto defaultSrv = ServerGetDefault();
		_dbg3(defaultSrv);
	} catch (...) {
		return ServerSetDefault();
	}
	return true;
}

bool cUseOT::ServerCreate(const string & nym, const string & filename, bool dryrun) {
	_fact("server new for nym=" << nym);
	if(dryrun) return true;
	if(!Init()) return false;

	string xmlContents;
	nUtils::cEnvUtils envUtils;

	try {
		xmlContents = nUse::GetInput(filename);
	} catch (const string & message) {
		return nUtils::reportError("Contract file is empty - aborting");
	}

	ID nymID = NymGetId(nym);
	ID serverID = opentxs::OTAPI_Wrap::CreateServerContract(nymID, xmlContents);
	string server = ServerGetName(serverID);
	if(serverID.empty())
		return reportError( "Failure to create contract for nym: " + NymGetName(nymID) + "(" + nymID + ")" );


	_info( "Contract created for Nym: " << NymGetName(nymID) << "(" << nymID << ")" );
	cout << zkr::cc::fore::lightgreen << "Contract created" << zkr::cc::console << endl << endl ;
	cout << server << " " << nUtils::stringToColor(serverID) << serverID << zkr::cc::console << endl << endl;
	cout << "To show contract use command: \"" << zkr::cc::bold << "server show-contract " << server << zkr::cc::console
			<< "\"" << endl;

	try {
		auto defaultSrv = ServerGetDefault();
		_dbg3(defaultSrv);
	} catch(...) {
		_note("Setting " << server << " as default server");
		return ServerSetDefault(server, false);
	}

	return true;

}

void cUseOT::ServerCheck() {
	if(!Init()) return;
	if (opentxs::OTAPI_Wrap::pingNotary(mDefaultIDs.at(nUtils::eSubjectType::Server),
			mDefaultIDs.at(nUtils::eSubjectType::User)) == -1) {
		_erro("No response from server: " + mDefaultIDs.at(nUtils::eSubjectType::Server));
	}

	_info("Server " + mDefaultIDs.at(nUtils::eSubjectType::Server) + " is OK");
}

string cUseOT::ServerGetDefault() {
	if(!Init()) {
		throw string("unknown error");
		return "";
	}
	auto defaultServer = mDefaultIDs.at(nUtils::eSubjectType::Server);
	if(defaultServer.empty() || defaultServer == "-") throw string("No default server!");
	_dbg1("default server: " << defaultServer);
	return defaultServer;
}

string cUseOT::ServerGetId(const string & serverName) { ///< Gets nym aliases and IDs begins with '%'
	if(!Init())
		return "";

	if ( nUtils::checkPrefix(serverName) )
		return serverName.substr(1);
	else {
		for(int i = 0 ; i < opentxs::OTAPI_Wrap::GetServerCount(); i++) {
			string serverID = opentxs::OTAPI_Wrap::GetServer_ID(i);
			string serverName_ = opentxs::OTAPI_Wrap::GetServer_Name(serverID);
			if (serverName_ == serverName)
				return serverID;
		}
	}
	return "";
}

string cUseOT::ServerGetName(const string & serverID){
	if(!Init())
		return "";
	if(serverID.empty())
		return "";

	auto srvName = opentxs::OTAPI_Wrap::GetServer_Name(serverID);
	return (srvName.empty())? "" : srvName;
}

bool cUseOT::ServerRemove(const string & serverName, bool dryrun) {
	_fact("server rm " << serverName);
	if(dryrun) return true;
	if(!Init()) return false;
	string serverID = ServerGetId(serverName);
	if ( opentxs::OTAPI_Wrap::Wallet_CanRemoveServer(serverID) ) {
		if ( opentxs::OTAPI_Wrap::Wallet_RemoveServer(serverID) ) {
			_info("Server " << serverName << " was deleted successfully");
			return true;
		}
		_warn("Failed to remove server " << serverName);
		return false;
	}
	_warn("Server " << serverName << " cannot be removed");
	return false;
}

bool cUseOT::ServerSetDefault() {
	ID serverID = opentxs::OTAPI_Wrap::GetServer_ID(0);
	if(serverID.empty()) return false;
	_note("Setting server " << ServerGetName(serverID) << " as default");
	return ServerSetDefault(ServerGetName(serverID), false);
}

bool cUseOT::ServerSetDefault(const string & serverName, bool dryrun) {
	_fact("server set-default " << serverName);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::Server) = ServerGetId(serverName);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

bool cUseOT::ServerPing(const string & server, const string & nym, bool dryrun) {
	_fact("server ping " << server << " " << nym);
	if(dryrun) return true;
	if(!Init()) return false;

	cout << "Checking connection (" << server << ") for nym: " << nym << endl;

	auto ping = opentxs::OTAPI_Wrap::pingNotary(ServerGetId(server), NymGetId(nym));

	if(ping == -1) {
		_erro("ping= " << ping << ", no message sent");
		cout << zkr::cc::fore::lightred << "Connection failed" << zkr::cc::console << endl;
		return false;
	}

	if(ping == 0) {
		_info("ping= " << ping << ", no errors, but also: no message was sent");
		cout << "No errors reported" << endl;
		return true;
	}

	else {
		_note("ping= " << ping << " OK");
		return true;
	}

	return false;
}

bool cUseOT::ServerShowContract(const string & serverName, const string &filename, bool dryrun) {
	_fact("server show-contract " << serverName << " " << filename);
	if(dryrun) return true;
	if(!Init()) return false;

	string serverID = ServerGetId(serverName);
	nUtils::DisplayStringEndl(cout, zkr::cc::fore::lightblue + serverName + zkr::cc::fore::console);
	nUtils::DisplayStringEndl(cout, "ID: " + serverID);

	const auto contract = opentxs::OTAPI_Wrap::GetServer_Contract(serverID);
	if(!filename.empty()) {
		try {
			nUtils::cEnvUtils envUtils;
			envUtils.WriteToFile(filename, contract);
			return true;
		} catch(...) {
			nUtils::DisplayStringEndl(cout, contract);
			return false;
		}
	}
	nUtils::DisplayStringEndl(cout, contract);


	return true;

}
vector<string> cUseOT::ServerGetAllNames() { ///< Gets all servers name
	if(!Init())
	return vector<string> {};

	vector<string> servers;
	for(int i = 0 ; i < opentxs::OTAPI_Wrap::GetServerCount ();i++) {
		string servID = opentxs::OTAPI_Wrap::GetServer_ID(i);
		string servName = opentxs::OTAPI_Wrap::GetServer_Name(servID);
		servers.push_back(servName);
	}
	return servers;
}

bool cUseOT::ServerDisplayAll(bool dryrun) {
	_fact("server ls");
	if(dryrun) return true;
	if(!Init()) return false;

	for(std::int32_t i = 0 ; i < opentxs::OTAPI_Wrap::GetServerCount();i++) {
		ID serverID = opentxs::OTAPI_Wrap::GetServer_ID(i);

		nUtils::DisplayStringEndl(nUtils::stringToColor(serverID) + ServerGetName(serverID) + "\t" + serverID);
	}
	return true;
}

bool cUseOT::TextEncode(const string & plainText, const string & fromFile, const string & toFile, bool dryrun) {
	_fact("text encode");
	if(dryrun) return true;
	if(!Init()) return false;

	string plainTextIn;
	nUtils::cEnvUtils envUtils;

	try {
		plainTextIn = nUse::GetInput(fromFile, plainText);
	} catch (const string & message) {
		return nUtils::reportError("", message, "Aborting");
	}

	ASRT(!plainTextIn.empty());

	bool bLineBreaks = true; // FIXME? opentxs::OTAPI_Wrap - bLineBreaks should usually be set to true
	string encodedText;
	encodedText = opentxs::OTAPI_Wrap::Encode(plainTextIn, bLineBreaks);

	if(encodedText.empty()) return nUtils::reportError("empty encoded text");
	if (!toFile.empty()) {
		try {
			nUtils::DisplayStringEndl(cout, encodedText);
			envUtils.WriteToFile(toFile, encodedText);
			cout << "saved" << endl;
			return true;
		} catch (...) {
			return false;
		}
	}

	nUtils::DisplayStringEndl(cout, encodedText);

	return true;
}

bool cUseOT::TextEncrypt(const string & recipientNymName, const string & plainText, bool dryrun) {
	_fact("text encrypt to " << recipientNymName);
	if(dryrun) return true;
	if(!Init()) return false;

	string encryptedText;
	encryptedText = opentxs::OTAPI_Wrap::Encrypt(NymGetToNymId(recipientNymName, NymGetDefault()), plainText);
	nUtils::DisplayStringEndl(cout, encryptedText);
	return true;
}

bool cUseOT::TextDecode(const string & encodedText, const string & fromFile, const string & toFile, bool dryrun) {
	_fact("text decode");
	if(dryrun) return true;
	if(!Init()) return false;

	nUtils::cEnvUtils envUtils;
	string encodedTextIn;

	try {
		encodedTextIn = nUse::GetInput(fromFile, encodedText);
	} catch (const string & message) {
		return nUtils::reportError("", message, "Aborting");
	}

	ASRT(!encodedTextIn.empty());

	bool bLineBreaks = true; // FIXME? opentxs::OTAPI_Wrap - bLineBreaks should usually be set to true
	string plainText;
	plainText = opentxs::OTAPI_Wrap::Decode(encodedTextIn, bLineBreaks);

	if(plainText.empty()) return nUtils::reportError("empty decoded text");

	if (!toFile.empty()) {
		try {
			envUtils.WriteToFile(toFile, plainText);
			nUtils::DisplayStringEndl(cout, plainText);
			cout << "saved" << endl;
			return true;
		} catch (...) {
			return false;
		}
	}

	nUtils::DisplayStringEndl(cout, plainText);
	return true;
}

bool cUseOT::TextDecrypt(const string & recipientNymName, const string & encryptedText, bool dryrun) {
	_fact("text decrypt for " << recipientNymName );
	if(dryrun) return true;
	if(!Init()) return false;

	string plainText;
	plainText = opentxs::OTAPI_Wrap::Decrypt(NymGetId(recipientNymName), encryptedText);
	nUtils::DisplayStringEndl(cout, plainText);
	return true;
}

bool cUseOT::VoucherCancel(const string & acc, const string & nym, const int32_t & index, bool dryrun) {
	_fact("voucher cancel " << " " << acc << " " << nym << " " << index);
	if(dryrun) return true;
	if(!Init()) return false;

	ID nymID = NymGetId(nym);
	string voucher = "" ;

	if(index == -1) {
		_dbg2("getting voucher from editor");

		voucher = GetText();
	} else {
		_dbg2("getting voucher from outpayments");
		auto count = opentxs::OTAPI_Wrap::GetNym_OutpaymentsCount(nymID);
		_dbg3("outpayment count: " << count);
		if(count == 0) {
			cout << zkr::cc::fore::lightred << "Empty outpayments for nym: " << NymGetName(nymID) << zkr::cc::console << endl;
			return false;
		}
		voucher = opentxs::OTAPI_Wrap::GetNym_OutpaymentsContentsByIndex(nymID, index);
		if(voucher.empty()) return nUtils::reportError("Empty voucher!");
	}
	ID accID = AccountGetId(acc);
	ID srvID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(accID);
	ID assetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(accID);

	if(opentxs::OTAPI_Wrap::Instrmnt_GetType(voucher) != "VOUCHER") {
		cout << zkr::cc::fore::lightred << "Not a voucher!" << zkr::cc::console << endl;
		return false;
	}

	ID vAssetID = opentxs::OTAPI_Wrap::Instrmnt_GetInstrumentDefinitionID(voucher);

	auto valid = opentxs::OTAPI_Wrap::Instrmnt_GetValidTo(voucher);
	_mark(valid);
	if(assetID != vAssetID) {
		cout << zkr::cc::fore::yellow << "Assets are different" << zkr::cc::console << endl;
		return false;
	}

	auto dep = mMadeEasy->deposit_cheque(srvID, nymID, accID, voucher);
	auto status = mMadeEasy->VerifyMessageSuccess(dep);

	if(status < 0)
		return nUtils::reportError(ToStr(status), "status", "Can't cancel voucher ");
	OutpaymentRemove(nym, index, false, false);

	auto ok = mMadeEasy->retrieve_account(srvID, nymID, accID, true);
	return ok;
}


bool cUseOT::VoucherWithdraw(const string & fromAcc, const string &fromNym, const string &toNym, int64_t amount,
		string memo, bool dryrun) {
	_fact("voucher new " << fromAcc << " " << toNym << " " << amount << " " << memo);
	if (dryrun)
		return true;
	if (!Init())
		return false;

	const ID fromAccID = AccountGetId(fromAcc);
	const ID fromNymID = NymGetId(fromNym);
	const ID toNymID = NymGetToNymId(toNym, fromNymID);

	const ID assetID = opentxs::OTAPI_Wrap::GetAccountWallet_InstrumentDefinitionID(fromAccID);
	const ID srvID = opentxs::OTAPI_Wrap::GetAccountWallet_NotaryID(fromAccID);

	_mark(toNym << " id: " << toNymID );

	if(!mMadeEasy->make_sure_enough_trans_nums(1, srvID, fromNymID)) {
		return nUtils::reportError("", "not enough transaction number", "Not enough transaction number!");
	}
	// amount validating
	if (amount < 1)
		return nUtils::reportError(ToStr(amount), "Amount < 1", "Amount must be greater then zero!");

	int64_t balance = opentxs::OTAPI_Wrap::GetAccountWallet_Balance(fromAccID);
	string accType = opentxs::OTAPI_Wrap::GetAccountWallet_Type(fromAccID);

	if (amount > balance && accType == "simple") { // TODO: issuer
		string mess = "Balance [" + fromAcc + "] is " + ToStr(balance);
		return nUtils::reportError(ToStr(balance), "Not enough money", mess);
	}

	if (memo.empty())
		memo = "(no memo)";
	_info("memo: " << memo);

	string attempt = "withdraw_voucher";

	auto response = mMadeEasy->withdraw_voucher(srvID, fromNymID, fromAccID, toNymID, memo, amount);
	// connection with server
	auto reply = mMadeEasy->InterpretTransactionMsgReply(srvID, fromNymID, fromAccID, attempt, response);
	if (reply != 1)
		return nUtils::reportError(ToStr(reply), "withdraw voucher (made easy) failed!", "Error from server!");

	auto ledger = opentxs::OTAPI_Wrap::Message_GetLedger(response);
	if (ledger.empty())
		return nUtils::reportError(ledger, "Some error with ledger", "Server error");

	auto transactionReply = opentxs::OTAPI_Wrap::Ledger_GetTransactionByIndex(srvID, fromNymID, fromAccID, ledger, 0);
	if (transactionReply == "")
		return nUtils::reportError(transactionReply, "some error with transaction reply", "Server error");

	_mark(opentxs::OTAPI_Wrap::Transaction_GetSuccess(srvID, fromNymID, fromAccID, transactionReply));

	auto voucher = opentxs::OTAPI_Wrap::Transaction_GetVoucher(srvID, fromNymID, fromAccID, transactionReply);
	if (voucher.empty())
		return nUtils::reportError(voucher, "Error with getting voucher", "Server error");

	cout << voucher << endl;

	_mark(opentxs::OTAPI_Wrap::Transaction_GetSuccess(srvID, fromNymID, fromAccID, voucher));

	auto send = mMadeEasy->send_user_payment(srvID, fromNymID, fromNymID, voucher);
	_dbg1(send);
	// sending voucher to yourself - saving voucher in my outpayments
	// after sending this voucher, this copy will be removed automatically

	bool srvAcc = mMadeEasy->retrieve_account(srvID, fromNymID, fromAccID, true);
	_dbg3("srvAcc retrv: " << srvAcc);

	if (!srvAcc)
		return nUtils::reportError(ToStr(srvAcc), "Retrieving account failed! Used force download", "Retriving account failed!");
	else
		cout << zkr::cc::fore::lightgreen << "Operation successful" << zkr::cc::console << endl; // all ok

	return true;
}


bool cUseOT::OTAPI_loaded = false;
bool cUseOT::OTAPI_error = false;

} // nUse
} // namespace OT


