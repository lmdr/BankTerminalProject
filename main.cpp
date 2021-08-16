/* 
* Louis Duller
* CSE 4701-001
* 2020-12-02
* Project 2 Part 2
* 
* NOTES:
* Program does not validate user supplied account number.
* Entering anything other than a number at the menu crashes the program.
* Name on account limited to one word. No multi-word names.
* Modify lines 38-40 if necessary to connect to MySQL DBMS.
* LINE 38:	HOST
*			PORT
*			USERNAME
*			PASSWORD
* LINE 39:	DATABASE NAME
* LINE 40:	TABLE NAME
*/



// includes
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <mysqlx/xdevapi.h>



// class: utilize RAII for automatic session lifetime management
class Connection
{
public:
	// default constructor
	Connection() :
		client("localhost", 33060, "root", ""),
		database(client.getSchema("CSE4701F20_PROJECT2")),
		table(database.getTable("ACCOUNT"))
	{}

	// destructor
	~Connection()
	{
		client.close();
	}

	// member function: get the connected session
	// required for transactions
	mysqlx::Session& get_session()
	{
		return client;
	}

	// member function: get the ACCOUNT table
	mysqlx::Table& get_table()
	{
		return table;
	}

private:
	mysqlx::Session client;			// connection
	mysqlx::Schema database;		// database
	mysqlx::Table table;			// ACCOUNT table
};



// function: prompt user for input
template<typename T>
void prompt(std::string dialog, T& input)
{
	std::cout << dialog;
	std::cin >> input;
}

// function: get account information
mysqlx::Row get_account_information(mysqlx::Table& table, uint64_t account_number)
{
	return table.select("*")
		.where("account_no = :acc_no")
		.limit(1)
		.bind("acc_no", account_number)
		.lockExclusive()
		.execute()
		.fetchOne();
}

// function: display account information
void display_account_information(mysqlx::Row& record)
{
	std::cout << "=== Account Information ===\n"
		<< "Account number: " << record[0]
		<< "\nName on account: " << record[1]
		<< "\nBalance: " << record[2]
		<< "\nAccount opened: " << record[3]
		<< "\nAccount status: " << record[4]
		<< "\n\n";
}

// function: display main menu
int32_t display_menu()
{
	// display options
	std::cout << "Main Menu\n"
		<< "0 - Quit\n"
		<< "1 - Open Account\n"
		<< "2 - Check Balance\n"
		<< "3 - Deposit\n"
		<< "4 - Withdraw\n"
		<< "5 - Transfer\n"
		<< "6 - Close Account\n";

	// prompt user for menu choice
	int32_t choice = 0;
	prompt("Enter your choice: ", choice);
	std::cout << "\n";
	return choice;
}

// function: open a new account
void open_account(mysqlx::Table& table)
{
	// prompt user
	std::cout << "********************************************************************************\n\n"
		<< "Opening account\n";
	std::string name;
	prompt("Name on account: ", name);
	float balance = 0.0;
	prompt("Initial balance: ", balance);
	std::cout << "\n";

	// insert record
	auto result = table.insert("name_on_account", "balance")
		.values(name, balance)
		.execute()
		.getAutoIncrementValue();

	// display result
	mysqlx::Row record = get_account_information(table, result);
	display_account_information(record);
	std::cout << "Account opened\n\n"
		<< "********************************************************************************\n\n";
}

// function: check an account balance
void check_balance(mysqlx::Table& table)
{
	// prompt user
	std::cout << "********************************************************************************\n\n"
		<< "Checking account balance\n";
	uint64_t account_number = 0;
	prompt("Enter account number: ", account_number);
	std::cout << "\n";

	// retrieve record
	mysqlx::Row record = get_account_information(table, account_number);

	// display result
	display_account_information(record);
	std::cout << "Account balance checked\n\n"
		<< "********************************************************************************\n\n";
}

// function: deposit funds into an account
void deposit(mysqlx::Session& session, mysqlx::Table& table)
{
	std::cout << "********************************************************************************\n\n"
		<< "Depositing account funds\n";

	// prompt user for account number
	uint64_t account_number = 0;
	prompt("Enter account number: ", account_number);
	std::cout << "\n";
	
	// start transaction
	session.startTransaction();

	try
	{
		// retrieve and lock record
		mysqlx::Row record = get_account_information(table, account_number);
		display_account_information(record);

		// check if account closed
		if (static_cast<std::string>(record[4]) != "open")
			throw "Account is not open.\n";

		// prompt user for deposit amount
		float deposit_amount = 0.0;
		prompt("Enter deposit amount: ", deposit_amount);
		std::cout << "\n";

		// modify record
		table.update()
			.set("balance", static_cast<float>(record[2]) + deposit_amount)
			.where("account_no = :acc_no")
			.bind("acc_no", account_number)
			.execute();

		// commit modifications
		session.commit();
	}
	catch (const mysqlx::Error& error)
	{
		// rollback to before start of transaction
		session.rollback();

		// display error message
		std::cout << "Could not complete transaction... Please try again.\n" << error
			<< "\n********************************************************************************\n\n";

		// return from function
		return;
	}
	catch (const char* error)
	{
		// rollback to before start of transaction
		session.rollback();

		// display error message
		std::cout << "Could not complete transaction... Please try again.\n" << error
			<< "\n********************************************************************************\n\n";

		// return from function
		return;
	}

	// retrieve record
	mysqlx::Row record = get_account_information(table, account_number);

	// display result
	display_account_information(record);
	std::cout << "Account funds deposited\n\n"
		<< "********************************************************************************\n\n";
}

// function: withdraw funds from an account
void withdraw(mysqlx::Session& session, mysqlx::Table& table)
{
	std::cout << "********************************************************************************\n\n"
		<< "Withdrawing account funds\n";

	// prompt user for account number
	uint64_t account_number = 0;
	prompt("Enter account number: ", account_number);
	std::cout << "\n";

	// start transaction
	session.startTransaction();

	try
	{
		// retrieve and lock record
		mysqlx::Row record = get_account_information(table, account_number);
		display_account_information(record);

		// check if account closed
		if (static_cast<std::string>(record[4]) != "open")
			throw "Account is not open.\n";

		// prompt user for withdrawal amount
		float withdraw_amount = 0.0;
		prompt("Enter withdrawal amount: ", withdraw_amount);
		std::cout << "\n";

		// validate withdrawal amount
		if (withdraw_amount > static_cast<float>(record[2]))
			throw "Withdraw amount greater than balance.\n";

		// modify record
		table.update()
			.set("balance", static_cast<float>(record[2]) - withdraw_amount)
			.where("account_no = :acc_no")
			.bind("acc_no", account_number)
			.execute();

		// commit modifications
		session.commit();
	}
	catch (const mysqlx::Error error)
	{
		// rollback to before start of transaction
		session.rollback();

		// display error message
		std::cout << "Could not complete transaction... Please try again.\n" << error
			<< "\n********************************************************************************\n\n";

		// return from function
		return;
	}
	catch (const char *error)
	{
		// rollback to before start of transaction
		session.rollback();

		// display error message
		std::cout << "Could not complete transaction... Please try again.\n" << error
			<< "\n********************************************************************************\n\n";

		// return from function
		return;
	}
	
	// retrieve record
	mysqlx::Row record = get_account_information(table, account_number);

	// display result
	display_account_information(record);
	std::cout << "Account funds withdrawn\n\n"
		<< "********************************************************************************\n\n";
}

// function: transfer funds from one accound to another
void transfer(mysqlx::Session& session, mysqlx::Table& table)
{
	std::cout << "********************************************************************************\n\n"
		<< "Transferring account funds\n";

	// prompt user for source account number
	uint64_t source_account_number = 0;
	prompt("Enter source account number: ", source_account_number);
	std::cout << "\n";

	// start transaction
	session.startTransaction();

	try
	{
		// retrieve and lock source account
		mysqlx::Row source_record = get_account_information(table, source_account_number);
		display_account_information(source_record);

		// check if source account closed
		if (static_cast<std::string>(source_record[4]) != "open")
			throw "Source account is not open.\n";

		// prompt user for target account number
		uint64_t target_account_number = 0;
		prompt("Enter target account number: ", target_account_number);
		std::cout << "\n";

		// retrieve and lock target account
		mysqlx::Row target_record = get_account_information(table, target_account_number);
		display_account_information(target_record);

		// check if target account closed
		if (static_cast<std::string>(target_record[4]) != "open")
			throw "Target account is not open.\n";

		// prompt user for transfer amount
		float transfer_amount = 0.0;
		prompt("Enter transfer amount: ", transfer_amount);
		std::cout << "\n";

		// validate transfer amount
		if (transfer_amount > static_cast<float>(source_record[2]))
			throw "Withdraw amount greater than source balance.\n";

		// withdraw from source account
		std::cout << "Transaction processing: withdrawing funds from source account. Please wait...\n";
		table.update()
			.set("balance", static_cast<float>(source_record[2]) - transfer_amount)
			.where("account_no = :acc_no")
			.bind("acc_no", source_account_number)
			.execute();

		// sleep for 10 seconds
		std::this_thread::sleep_for(std::chrono::seconds(10));

		// deposit into target account
		std::cout << "Transaction processing: depositing funds into target account. Please wait...\n\n";
		table.update()
			.set("balance", static_cast<float>(target_record[2]) + transfer_amount)
			.where("account_no = :acc_no")
			.bind("acc_no", target_account_number)
			.execute();

		// sleep for 10 seconds
		std::this_thread::sleep_for(std::chrono::seconds(10));

		// commit modifications
		session.commit();
	}
	catch (const mysqlx::Error error)
	{
		// rollback to before start of transaction
		session.rollback();

		// display error message
		std::cout << "Could not complete transaction... Please try again.\n" << error
			<< "\n********************************************************************************\n\n";

		// return from function
		return;
	}
	catch (const char* error)
	{
		// rollback to before start of transaction
		session.rollback();

		// display error message
		std::cout << "Could not complete transaction... Please try again.\n" << error
			<< "\n********************************************************************************\n\n";

		// return from function
		return;
	}

	// retrieve record
	mysqlx::Row source_record = get_account_information(table, source_account_number);

	// display result
	display_account_information(source_record);
	std::cout << "Account funds transferred\n\n"
		<< "********************************************************************************\n\n";
}

// function: close an account
void close_account(mysqlx::Table& table)
{
	// prompt user
	std::cout << "********************************************************************************\n\n"
		<< "Closing account\n";
	uint64_t account_number = 0;
	prompt("Enter account number: ", account_number);
	std::cout << "\n";

	// modify record
	table.update()
		.set("account_status", "closed")
		.where("account_no = :acc_no")
		.bind("acc_no", account_number)
		.execute();

	// display result
	mysqlx::Row record = get_account_information(table, account_number);
	display_account_information(record);
	std::cout << "Account closed\n\n"
		<< "********************************************************************************\n\n";
}



// function: program entry point
int main()
{
	Connection session;

	// handle user menu choice
	int8_t state = 1;
	while(state)
	{
		// get user menu choice
		state = display_menu();

		switch (state)
		{
		case 0:		// quit
			break;
		case 1:		// open account
			open_account(session.get_table());
			break;
		case 2:		// check balance
			check_balance(session.get_table());
			break;
		case 3:		// deposit funds
			deposit(session.get_session(), session.get_table());
			break;
		case 4:		// withdraw funds
			withdraw(session.get_session(), session.get_table());
			break;
		case 5:		// transfer funds
			transfer(session.get_session(), session.get_table());
			break;
		case 6:		// close account
			close_account(session.get_table());
			break;
		default:	// invalid user choice
			std::cout << "Invalid option. Please try again.\n\n";
			break;
		}
	}

	// exit
	// MySQL session automatically closes when Connection object is destroyed
	return EXIT_SUCCESS;
}