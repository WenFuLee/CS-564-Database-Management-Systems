/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"
#include "file_iterator.h"

namespace badgerdb { 

BufMgr::BufMgr(std::uint32_t bufs)
	: numBufs(bufs) {
	bufDescTable = new BufDesc[bufs];

  for (FrameId i = 0; i < bufs; i++) 
  {
  	bufDescTable[i].frameNo = i;
  	bufDescTable[i].valid = false;
  }

  bufPool = new Page[bufs];

	int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
  hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

  clockHand = bufs - 1;
}


BufMgr::~BufMgr() {
	for (FrameId i = 0; i < numBufs; i++) {
		//if dirty -> flush
		bufDescTable[i].frameNo = i;
		bufDescTable[i].valid = false;
	}
}

void BufMgr::advanceClock()
{
	this->clockHand++;
	this->clockHand %= this->numBufs; 
}

void BufMgr::allocBuf(FrameId & frame) 
{
	// clock algorithm
}
	
void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
{
	FrameId frameNo;
	try {
		this->hashTable->lookup(file, pageNo, frameNo);
		*page = bufPool[frameNo];
		this->bufDescTable[frameNo].refbit = true; // check whether it should be true or false later.
		this->bufDescTable[frameNo].pinCnt++;
	} 
	catch (HashNotFoundException)
	{
		std::cout << "HashNotFoundException" << std::endl;
		
		Page diskPage = file->readPage(pageNo);
		this->allocBuf(frameNo);
		this->hashTable->insert(file, pageNo, frameNo);
		this->bufDescTable[frameNo].Set(file, pageNo);
		this->bufPool[frameNo] = diskPage;
		*page = bufPool[frameNo];
	}
}

void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty) 
{
	try
	{
		FrameId frameNo;
		this->hashTable->lookup(file, pageNo, frameNo);
		if (this->bufDescTable[frameNo].pinCnt <= 0) {
			throw PageNotPinnedException(file->filename(), pageNo, frameNo);
		}
		this->bufDescTable[frameNo].pinCnt--;
		if (dirty) {
			this->bufDescTable[frameNo].dirty = true;
		}
	}
	catch(PageNotPinnedException)
	{
		std::cout << "PageNotPinnedException" << std::endl;
	}
	catch(HashNotFoundException)
	{
		std::cout << "HashNotFoundException" << std::endl;
	}
}

void BufMgr::flushFile(const File* file) 
{
	FrameId frameNo;
	File temp = *file;
	 // Iterate through all pages in the file.
    for (FileIterator iter = temp.begin(); iter != temp.end(); ++iter) {
		try
		{
			PageId pageNo = (*iter).page_number();
			this->hashTable->lookup(file, pageNo, frameNo);
			if (this->bufDescTable[frameNo].pinCnt > 0) {
				throw PagePinnedException(file->filename(), pageNo, frameNo);
			}

			if (!this->bufDescTable[frameNo].valid) {
				throw BadBufferException(frameNo,
										this->bufDescTable[frameNo].dirty,
										this->bufDescTable[frameNo].valid,
										this->bufDescTable[frameNo].refbit);
			}

			if (this->bufDescTable[frameNo].dirty) {
				this->bufDescTable[frameNo].file->writePage(this->bufPool[frameNo]);
				this->bufDescTable[frameNo].dirty = false;
			}
			this->hashTable->remove(file, pageNo);
			this->bufDescTable[frameNo].Clear();
			this->bufPool[frameNo] = Page();
		}
		catch (HashNotFoundException)
		{
			std::cout << "Page not in buffer!" << std::endl;
		}
		catch (PagePinnedException) {
			std::cout << "Page of the file is pinned!" << std::endl;
		}
		catch (BadBufferException) {
			std::cout << "An invalid page belonging to the file is encountered!" << std::endl;
		}
    }
}

void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page) 
{
	Page diskPage = file->allocatePage();
	pageNo = diskPage.page_number();
	FrameId frameNo;
	this->allocBuf(frameNo);
	this->hashTable->insert(file, pageNo, frameNo);
	this->bufDescTable[frameNo].Set(file, pageNo);
	this->bufPool[frameNo] = diskPage;
	*page = bufPool[frameNo];
}

void BufMgr::disposePage(File* file, const PageId PageNo)
{
	try {
		FrameId frameNo;
		this->hashTable->lookup(file, PageNo, frameNo);
		if (this->bufDescTable[frameNo].dirty) {
			//file->writePage(this->bufPool[frameNo]);
			this->bufDescTable[frameNo].file->writePage(this->bufPool[frameNo]);
		}
		this->bufDescTable[frameNo].Clear();
		this->hashTable->remove(file, PageNo);
		this->bufPool[frameNo] = Page();
	} 
	catch (HashNotFoundException)
	{
		std::cout << "Page not in buffer!" << std::endl;
	}

	file->deletePage(PageNo);
}

void BufMgr::printSelf(void) 
{
  BufDesc* tmpbuf;
	int validFrames = 0;
  
  for (std::uint32_t i = 0; i < numBufs; i++)
	{
  	tmpbuf = &(bufDescTable[i]);
		std::cout << "FrameNo:" << i << " ";
		tmpbuf->Print();

  	if (tmpbuf->valid == true)
    	validFrames++;
  }

	std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
}

}
