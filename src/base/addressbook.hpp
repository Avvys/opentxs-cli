#ifndef ADDRESSBOOK_HPP_
#define ADDRESSBOOK_HPP_

#include "lib_common2.hpp"

namespace nOT {
class AddressBook {

public:
public:
	AddressBook(const string & nymID); ///< constructor, should be called only from Load() function @see Load()
	static shared_ptr<AddressBook> Load(const string &nymID); ///< creates AddressBook object and returns shared pointer to it, this function should be used only from AddressBookSorade
	size_t getCount() { return contacts.size(); }; ///< get numbers of contact

	bool add(const string & nymName, const string & nymID); ///< adding new nym to address book
	bool nymExist(const string & nymID) const; ///< check nym exists (by ID)
	string nymGetID(const string & nymName) const; ///< get nyn exist
	bool nymNameExist(const string & nymName) const; ///< check nym exists (by name)
	string nymGetName(const string & id) const; ///< get nym name
	bool nymExport(const string & nymName, const string &nymID, const string & filename);
	bool nymImport(const string & filename);


	bool remove(const string & nymID); ///< removes nym
	void removeAll(); ///< removes all nyms from address book

	vector<string> getAllNames(); ///< returns vector of all nyms names from address book

	void display();
	virtual ~AddressBook();

private:
	class Entry {
		string nymName;
	public:
		Entry(const string & name) :
				nymName(name) {
		};
		string toString() { return nymName; };
		static Entry fromString(string strEntry);
	};

	void createDirectory();
	bool loadFromFile();

	void saveContacts();
	void saveContacts(map<string, string> contacts);

	const string ownerNymID;
	string path;
	map<string, string> contacts;
};

class AddressBookStorage {
public:
	static shared_ptr<AddressBook> Get(const string & nymID); ///< returns pointer to address book for specific nym
	static void ForceClear(); ///< remove pointers to addressBook
	static string GetNymName(const string & nymID, const vector<string> & allNymsID); ///< search name in address books all given nyms
	static vector <string> GetAllNames(const vector<string> & allNymsID); ///< all names from all address books, used to completition
	static void Reload();
	static bool NymNameExist(const string & nymName, const vector<string> & allNymsID);
private:
	static void Load(const vector<string> & allNymsID);
	static map <string, shared_ptr<AddressBook>> saved; ///< map with <id nyms, pointers to address book>
	static vector<string> names; ///< map with all nym names
	static bool init;
};


} // nOT



#endif
