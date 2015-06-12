/* See other files here for the LICENCE that applies here. */
/*
Template for new files, replace word "template" and later delete this line here.
*/

#ifndef INCLUDE_OT_NEWCLI_USEOT
#define INCLUDE_OT_NEWCLI_USEOT

#include "lib_common2.hpp"
//#include "OTStorage.hpp"
#include "addressbook.hpp"

namespace opentxs{
class OT_ME;
};

// Use this to mark methods
#define	EXEC
#define	HINT
#define	VALID

namespace nOT {
namespace nUse {


	INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2 // <=== namespaces

	using ID = string;
	using name = string;

	class cUseCache { // TODO optimize/share memory? or convert on usage
		friend class cUseOT;
	public:
		cUseCache();
	protected:
		map<ID, name> mNyms;
		map<ID, name> mAccounts;
		map<ID, name> mAssets;
		map<ID, name> mServers;
		bool mNymsLoaded;
		bool mAccountsLoaded;
		bool mAssetsLoaded;
		bool mServersLoaded;
	private:
	};

	class cUseOT {
	public:

		static bool OTAPI_loaded;
		static bool OTAPI_error;

	private:

        cUseOT(const cUseOT &) {
            throw std::exception();
        }

		string mDbgName;

		opentxs::OT_ME * mMadeEasy;

		cUseCache mCache;

		map<nUtils::eSubjectType, ID> mDefaultIDs; ///< Default IDs are saved to file after changing any default ID
		const string mDataFolder;
		const string mDefaultIDsFile;

		typedef ID ( cUseOT::*FPTR ) (const string &);

		map<nUtils::eSubjectType, FPTR> subjectGetIDFunc; ///< Map to store pointers to GetID functions
		map<nUtils::eSubjectType, FPTR> subjectGetNameFunc; ///< Map to store pointers to GetName functions

	private:

		void LoadDefaults(); ///< Defaults are loaded when initializing OTAPI

	protected:

		enum class eBoxType { Inbox, Outbox };
		EXEC bool MsgDisplayForNymBox( eBoxType boxType, const string & nymName, int msg_index, bool dryrun);

	public:

		cUseOT(const string &mDbgName);
		~cUseOT();

		string DbgName() const NOEXCEPT;

		bool Init();
		void CloseApi();

		VALID bool CheckIfExists(const nUtils::eSubjectType type, const string & subject);
		VALID bool CheckIfExists(const nUtils::eSubjectType type, const string & subject, const string & without);
		EXEC bool DisplayDefaultSubject(const nUtils::eSubjectType type, bool dryrun);
		EXEC bool DefaultsExport(const string & filename, bool dryrun);
		bool DisplayAllDefaults(bool dryrun);
		EXEC bool DisplayHistory(bool dryrun);
		string SubjectGetDescr(const nUtils::eSubjectType type, const string & subject);
		bool Refresh(bool dryrun);
		bool PrintInstrumentInfo(const string &instrument); ///< shows details about instrument

		//================= account =================

		vector<ID> AccountGetAllIds();
		int64_t AccountGetBalance(const string & accountName);
		string AccountGetDefault();
		ID AccountGetId(const string & account); ///< Gets account ID both from name and ID with prefix
		string AccountGetName(const ID & accountID);
		ID AccountGetAssetID(const string & account);
		string AccountGetAsset(const string & account);

		ID AccountGetNymID(const string & account);
		HINT string AccountGetNym(const string & account); ///< get name of owner nym
		VALID bool AccountIsOwnerNym(const string & account, const string & nym);

		HINT vector<string> AccountGetAllNames();

		EXEC bool AccountCreate(const string & nym, const string & asset, const string & newAccountName, const string & server, bool dryrun);
		EXEC bool AccountDisplay(const string & account, bool dryrun);
		EXEC bool AccountDisplayAll(bool dryrun);
		EXEC bool AccountRefresh(const string & accountName, bool all, bool dryrun);
		EXEC bool AccountRemove(const string & account, bool dryrun) ;
		EXEC bool AccountRename(const string & account, const string & newAccountName, bool dryrun);
		EXEC bool AccountSetDefault(const string & account, bool dryrun);
		EXEC bool AccountSetName(const string & accountID, const string & NewAccountName, bool dryrun);
		EXEC bool AccountTransfer(const string & accountFrom, const string & accountTo, const int64_t & amount, const string & note, bool dryrun);

		//================= account-in =================

		EXEC bool AccountInDisplay(const string & account, bool dryrun);
		EXEC bool AccountInAccept(const string & account, const int index, bool all, bool dryrun);

		//================= account-out =================

		EXEC bool AccountOutCancel(const string & account, const int index, bool all, bool dryrun);
		EXEC bool AccountOutDisplay(const string & account, bool dryrun);

		//================= addressbook =================

		EXEC bool AddressBookAdd(const string & nym, const string & newNym, const ID & newNymID, bool dryrun); ///< adds new nym to adress book, TODO: some validation of new nym ID
		EXEC bool AddressBookDisplay(const string & nym, bool dryrun); ///< displaying address book for specific nym
		EXEC bool AddressBookRemove(const string & ownerNym, const ID & toRemoveNymID, bool dryrun); ///< removes entry from address book by nym ID
		EXEC bool AddressBookImport(const string & ownerNym, const string & filename, bool dryrun);
		EXEC bool AddressBookExport(const string & ownerNym, const string & exportNym, const string & filename, bool dryrun);

		//================= asset =================

		ID AssetGetId(const string & asset); ///< Gets asset ID both from name and ID with prefix
		string AssetGetName(const ID & accountID);
		string AssetGetContract(const string & asset);
		ID AssetGetDefault(); ///< Get default asset, also known as purse

		HINT vector<string> AssetGetAllNames();

		EXEC bool AssetAdd(const string & filename, bool dryrun); ///< adds asset by contract
		EXEC bool AssetSetDefault(); ///< Set default asset, also known as purse
		EXEC bool AssetSetDefault(const std::string & asset, bool dryrun); ///< Set default asset, also known as purse
		EXEC bool AssetDisplayAll(bool dryrun);
		EXEC bool AssetIssue(const string & server, const string & nym, const string & filename, bool dryrun) ;
		EXEC bool AssetNew(const string & nym, const string & filename, bool dryrun);
		EXEC bool AssetRemove(const string & asset, bool dryrun);
		EXEC bool AssetShowContract(const string & asset, const string & filename, bool dryrun); ///< shows asset contract or saves it to file

		//================= basket =================

//TODO:	EXEC bool BasketDisplay();
		EXEC bool BasketNew(const string & nym, const string & server, const string & assets, bool dryrun);


		//================= cash =================	// FIXME: segfault!

		string CashExport(const string & nymSender, const string & nymRecipient, const string & account, const string & indices, const bool passwordProtected, string & retained_copy);  ///< Used by CashSend and CashExportWrap to export cash purse
		bool CashDeposit(const string & account, const string & strFromNymID, const string & strServerID,  const string & strInstrument); ///< Used by CashDepositWrap and PaymentAccept to deposit cash purse on account

		EXEC bool CashDepositWrap(const string & accountID, bool dryrun); ///< deposit cash purse to account
		EXEC bool CashExportWrap(const ID & nymSender, const ID & nymRecipient, const string & account, bool passwordProtected, bool dryrun); ///< Export cash purse and display it to screen
		EXEC bool CashImport(const string & nym, bool dryrun); ///< Import cash from file or by pasting to editor
		EXEC bool CashSend(const string & nymSender, const string & nymRecipient, const string & account, int64_t amount, bool dryrun); ///< Send amount of cash from purse connected with account to Recipient, withdraw if necessary.
		EXEC bool CashShow(const string & account, bool dryrun); ///< Show purse connected with account
		EXEC bool CashWithdraw(const string & account, int64_t amount, bool dryrun); ///< withdraw cash from account on server into local purse

		//================= cheque =================

		EXEC bool ChequeCreate(const string &fromAcc, const string & fromNym, const string &toNym, int64_t amount, const string &srv, const string &memo, bool dryrun);
		EXEC bool ChequeDiscard(const string &acc, const string &nym, const int32_t & index, bool dryrun);

		//================= ?contract =================

		string ContractSign(const std::string & nymID, const std::string & contract);
		EXEC bool ContractSign(const string & nym, const string & filename, const string & outfilename, bool dryrun);

		//================= market =================
		EXEC bool MarketList(const string & srvName, const string & nymName, bool dryrun); ///< displaying available markets


		//================= mint =================
		EXEC bool MintShow(const string & srvName, const string & nymName, const string & assetName, bool dryrun);

		//================= msg =================

		vector<string> MsgGetAll();
		bool MsgSend(const string & nymSender, const string & nymRecipient, const string & msg);

		VALID bool MsgInCheckIndex(const string & nymName, const int32_t & nIndex);
		VALID bool MsgOutCheckIndex(const string & nymName, const int32_t & nIndex);

		EXEC bool MsgDisplayForNym(const string & nymName, bool dryrun);

		EXEC bool MsgDisplayForNymInbox(const string & nymName, int msg_index, bool dryrun);
		EXEC bool MsgDisplayForNymOutbox(const string & nymName, int msg_index, bool dryrun);

		EXEC bool MsgSend(const string & nymSender, vector<string> nymRecipient, const string & subject, const string & msg, int prio, const string & filename, bool dryrun);
		EXEC bool MsgInRemoveByIndex(const string & nymName, const int32_t & nIndex, bool dryrun);
		EXEC bool MsgOutRemoveByIndex(const string & nymName, const int32_t & nIndex, bool dryrun);

		//================= nym =================

		void NymGetAll(bool force=false);
		vector<string> NymGetAllIDs();
		ID NymGetDefault();
		ID NymGetId(const string & nym); ///< Gets Nym ID both from name and ID with prefix
		ID NymGetToNymId(const string & nym, const string & ownerNymID); ///< like @see NymGetID() but allows nyms from address book, should be use to get recipient nym ID
		string NymGetName(const ID & nymID);
		string NymGetRecipientName(const ID & nymID); ///< returns nym name or ID (if name not found), should be used to displaying payments, etc. works, when recipient nym was removed from address book and for now is unknown
		bool NymSetName(const ID & nymID, const string & newNymName);
		VALID bool NymNameExist(const string & nymName);

		HINT vector<string> NymGetAllNames();

		EXEC bool NymCheck(const string & nymName, bool dryrun);
		EXEC bool NymCreate(const string & nymName, bool registerOnServer, bool dryrun);
		EXEC bool NymDisplayAll(bool dryrun);
		EXEC bool NymDisplayInfo(const string & nymName, bool dryrun);
		EXEC bool NymExport(const string & nymName, const string & filename, bool dryrun);
		EXEC bool NymImport(const string & filename, bool dryrun);
		EXEC bool NymRefresh(const string & nymName, bool all, bool dryrun);
		EXEC bool NymRegister(const string & nymName, const string & serverName, bool force, bool dryrun);
		EXEC bool NymRemove(const string & nymName, bool force, bool dryrun);
		EXEC bool NymRename(const string & oldNymName, const string & newNymName, bool dryrun);
		EXEC bool NymSetDefault(const string & nymName, bool dryrun);
		EXEC bool NymUnregister(const string &nym, const string & server, bool force, bool dryrun);

		//================= outpayments ==========

		VALID bool OutpaymentCheckIndex(const string & nymName, const int32_t & index);
		HINT int32_t OutpaymantGetCount(const string & nym);

		EXEC bool OutpaymentDiscard(const string & acc, const string & nym, const int32_t index, bool dryrun); ///< discard outgoing payment
		EXEC bool OutpaymentDisplay(const string & nym, bool dryrun);
		EXEC bool OutpaymentSend(const string & senderNym, const string & recipientNym, int32_t index, bool all, bool dryrun);
		bool OutpaymentSend(const string & senderNym, const string & recipientNym, int32_t index,  bool dryrun); ///< main sending function
		EXEC bool OutpaymentShow(const string & nym, int32_t index, bool dryrun);
		EXEC bool OutpaymentRemove(const string & nym, const int32_t & index, bool all, bool dryrun);

		//================= payment ==================

		EXEC bool PaymentShow(const string & nym, const string & server, bool dryrun); ///< show payments inbox
		EXEC bool PaymentAccept(const string & account, int64_t index, bool all, bool dryrun); ///< accept specified payment from payment inbox
		bool PaymentAccept(const string & account, int64_t index, bool dryrun); ///< main accepting function
		EXEC bool PaymentDiscard(const string & nym, const string & index, bool all, bool dryrun);
		EXEC bool PaymentDiscardAll(bool dryrun);

		//================= purse ==================

		EXEC bool PurseCreate(const string & serverName, const string & asset, const string & ownerName, const string & signerName, bool dryrun);
		EXEC bool PurseDisplay(const string & serverName, const string & asset, const string & nymName, bool dryrun);

		//================= recordbox ==================

		EXEC bool RecordClear(const string &acc, bool all, bool dryrun);
		EXEC bool RecordDisplay(const string &acc, bool noVerify, bool dryrun); ///< prints list of recorded payments
//TODO:	EXEC bool RecordRemove(const string &acc, const string & srv, int32_t index, bool dryrun);
		EXEC bool RecordShow(const string &acc, const string & srv, int32_t index, bool dryrun);

		//================= server =================

		void ServerCheck(); ///< Check server availability (ping server)
		ID ServerGetDefault(); ///< Gets ID of default server
		ID ServerGetId(const string & server); ///< Gets server ID both from name and ID with prefix
		string ServerGetName(const string & serverID); ///< Gets Name of default server
		bool ServerSetDefault();

		HINT vector<string> ServerGetAllNames();

		EXEC bool ServerAdd(const string &filename, bool dryrun); ///< Adds new server contract (from editor or file)
		EXEC bool ServerCreate(const string & nymName, const string & filename, bool dryrun); ///< Creates new server contract
		EXEC bool ServerRemove(const string & serverName, bool dryrun);
		EXEC bool ServerSetDefault(const string & serverName, bool dryrun);
		EXEC bool ServerShowContract(const string & serverName, const string &filename, bool dryrun); ///< shows server contract or saves it to file
		EXEC bool ServerDisplayAll(bool dryrun);
		EXEC bool ServerPing(const string & server, const string & nym, bool dryrun); ///< checking server connection FIXME: timeout

		//================= text =================

		EXEC bool TextEncode(const string & plainText, const string & fromFile, const string & toFile, bool dryrun);
		EXEC bool TextEncrypt(const string & recipientNymName, const string & plainText, bool dryrun);
		EXEC bool TextDecode(const string & encodedText, const string & fromFile, const string & toFile, bool dryrun);
		EXEC bool TextDecrypt(const string & recipientNymName, const string & encryptedText, bool dryrun);


		//================= voucher ==============

		EXEC bool VoucherCancel(const string & acc, const string & nym, const int32_t & index, bool dryrun); ///< Cancels a voucher (from outpayments or run editor), usable only when voucher wasn't sent
		EXEC bool VoucherWithdraw(const string & fromAcc, const string &fromNym, const string &toNym, int64_t amount,
				string memo, bool dryrun); ///< creating new voucher

	};

} // nUse
} // namespace nOT

#endif

