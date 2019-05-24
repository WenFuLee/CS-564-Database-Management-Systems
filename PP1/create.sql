-- Create User table
drop table if exists User;
create table User(
	UserID TEXT PRIMARY KEY, 
	Rating INTEGER, 
	Location TEXT, 
	Country TEXT);

-- Create Item table
drop table if exists Item;
create table Item(
	ItemID INTEGER PRIMARY KEY, 
	SellerID TEXT,
	Number_of_Bids INTEGER, 
	First_Bid FLOAT,
	Buy_Price FLOAT,
	Currently FLOAT,
	Name TEXT,
	Started TEXT,
	Ends TEXT,
	Description TEXT);

-- Create Bid table
drop table if exists Bid;
create table Bid(
	ItemID INTEGER, 
	UserID TEXT,
	Time TEXT,
	Amount FLOAT,
	PRIMARY KEY(ItemID, UserID, Time)
	FOREIGN KEY(ItemID)
		REFERENCES Item(ItemID),
	FOREIGN KEY(UserID)
		REFERENCES User(UserID));

-- Create Categorize table
drop table if exists Categorize;
create table Categorize(
	ItemID INTEGER, 
	Type TEXT,
	FOREIGN KEY(ItemID)
		REFERENCES Item(ItemID),
	FOREIGN KEY(Type)
		REFERENCES Category(Type));

-- Create Category table
drop table if exists Category;
create table Category(
	Type TEXT PRIMARY KEY);