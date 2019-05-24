-- Create User table
drop table if exists User;
create table User(
	UserID VARCHAR(30) PRIMARY KEY, 
	Rating INTEGER, 
	Location VARCHAR(30), 
	Country VARCHAR(30));

-- Create Item table
drop table if exists Item;
create table Item(
	ItemID INTEGER PRIMARY KEY, 
	SellerID VARCHAR(30), 
	Number_of_Bids INTEGER, 
	First_Bid VARCHAR(30),
	Buy_Price VARCHAR(30),
	Currently VARCHAR(30),
	Name VARCHAR(30),
	Started VARCHAR(30),
	Ends VARCHAR(30));

-- Create Bid table
drop table if exists Bid;
create table Bid(
	ItemID INTEGER, 
	UserID VARCHAR(30),
	Time VARCHAR(30),
	Amount VARCHAR(30),
	PRIMARY KEY(ItemID, UserID, Time)
	FOREIGN KEY(ItemID)
		REFERENCES Item(ItemID),
	FOREIGN KEY(UserID)
		REFERENCES User(UserID));

-- Create Categorize table
drop table if exists Categorize;
create table Categorize(
	ItemID INTEGER, 
	Type VARCHAR(30),
	FOREIGN KEY(ItemID)
		REFERENCES Item(ItemID),
	FOREIGN KEY(Type)
		REFERENCES Category(Type));

-- Create Category table
drop table if exists Category;
create table Category(
	Type VARCHAR(30) PRIMARY KEY);