import web
import sys

reload(sys)
sys.setdefaultencoding('utf-8')

db = web.database(dbn='sqlite',
        db='AuctionBase.db' #TODO: add your SQLite database filename
    )

######################BEGIN HELPER METHODS######################

# Enforce foreign key constraints
# WARNING: DO NOT REMOVE THIS!
def enforceForeignKey():
    db.query('PRAGMA foreign_keys = ON')

# initiates a transaction on the database
def transaction():
    return db.transaction()
# Sample usage (in auctionbase.py):
#
# t = sqlitedb.transaction()
# try:
#     sqlitedb.query('[FIRST QUERY STATEMENT]')
#     sqlitedb.query('[SECOND QUERY STATEMENT]')
# except Exception as e:
#     t.rollback()
#     print str(e)
# else:
#     t.commit()
#
# check out http://webpy.org/cookbook/transactions for examples

# returns the current time from your database
def getTime():
    # TODO: update the query string to match
    # the correct column and table name in your database
    query_string = 'select Time from CurrentTime'
    results = query(query_string)
    # alternatively: return results[0]['currenttime']
    return results[0].Time # TODO: update this as well to match the
                                  # column name

# returns a single item specified by the Item's ID in the database
# Note: if the `result' list is empty (i.e. there are no items for a
# a given ID), this will throw an Exception!
def getItemById(item_id):
    # TODO: rewrite this method to catch the Exception in case `result' is empty
    try:
        query_string = 'select * from Items where ItemID = $itemID'
        result = query(query_string, {'itemID': item_id})
        return result[0]
    except Exception as e:
        return {}

def setCurrentTime(new_current_time):
    try:
        current_time = getTime()
        db.update('CurrentTime', where='Time=$current_time', Time=new_current_time, vars=locals())
        return True, ""
    except Exception as e:
        retMessage = e.message
        if (retMessage == 'Trigger1_Failed') :
            retMessage = "The current time can only advance forward."
        return False, retMessage

def searchItem(itemID, userID, category, itemDescription, minPrice, maxPrice):
    condition = ''
    table = 'Items'
    
    if (itemID != ''):
        condition = condition + 'ItemID = ' + itemID
    
    if (minPrice != ''):
        if (condition != ''):
            condition = condition + ' and Currently >= ' + minPrice
        else:
            condition = condition + 'Currently >= ' + minPrice
    
    if (maxPrice != ''):
        if (condition != ''):
            condition = condition + ' and Currently <= ' + maxPrice
        else:
            condition = condition + 'Currently <= ' + maxPrice

    if (itemDescription != ''):
        if (condition != ''):
            condition = condition + ' and Description LIKE \'%{}%\''.format(itemDescription)
        else:
            condition = condition + 'Description LIKE \'%{}%\''.format(itemDescription)

    if (category != ''):
        table = table + ', Categories'
        if (condition != ''):
            condition = condition + ' and Items.ItemID = Categories.ItemID and Categories.Category = \'{}\''.format(category)
        else:
            condition = condition + 'Items.ItemID = Categories.ItemID and Categories.Category = \'{}\''.format(category)

    print(table)
    print(condition)
    query_string = 'select * from {} where {}'.format(table, condition)
    result = query(query_string)
    return result

def searchItemDetails(itemID):
    itemResult = getItemById(itemID)

    queryCategories = 'select distinct(Category) from {} where {}'.format('Categories', 'ItemID = ' + itemID)
    categoriesResult = query(queryCategories)

    queryBids = 'select * from {} where {}'.format('Bids', 'ItemID = ' + itemID)
    bidsResult = query(queryBids)
    
    print categoriesResult
    print "_______________________________"
    print bidsResult

    return itemResult, categoriesResult, bidsResult

def insertBid(itemID, userID, price, current_time):
    # TODO: 
    try:
        itemData = getItemById(itemID)
        if (itemData) :
            print itemData['Buy_Price'] 
            if ((itemData['Buy_Price'] != None) and (itemData['Currently'] >= itemData['Buy_Price'])) :
                return False, "The Auction has already closed."
        else:
            return False, "Item ID is invalid."
        db.insert('Bids', ItemID=itemID, UserID=userID, Amount=price, Time=current_time)
        # incrementation for number of bids is handled by trigger4

        return True, ("Successfully added new bid: " + str(itemID) + ", " + str(userID) + ", " + str(price) + ", " + str(current_time))
    except Exception as e:
        retMessage = e.message
        if (retMessage == 'FOREIGN KEY constraint failed') :
            retMessage = "User ID is invalid."
        elif (retMessage == 'Trigger5_Failed') :
            retMessage = "No auction may have a bid before its start time."
        elif (retMessage == 'Trigger6_Failed') :
            retMessage = "No auction may have a bid after its end time."
        elif (retMessage == 'Trigger3_Failed') :
            retMessage = "You need to offer higher price."
        elif (retMessage == 'Trigger7_Failed') :
            retMessage = "A user may not bid on an item he or she is also selling."   
        elif (retMessage == 'UNIQUE constraint failed: Bids.ItemID, Bids.Time') :
            retMessage = "No auction may have two bids at the exact same time."
 
        return False, retMessage

# wrapper method around web.py's db.query method
# check out http://webpy.org/cookbook/query for more info
def query(query_string, vars = {}):
    return list(db.query(query_string, vars))


#####################END HELPER METHODS#####################
#TODO: additional methods to interact with your database,
# e.g. to update the current time

