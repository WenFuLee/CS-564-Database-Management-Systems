/**
 * Wen-Fu Lee         wlee256@wisc.edu    9077987502
 * Shang-Yen Yeh      syeh6@wisc.edu      9079956299
 * Yahn-Chung Chen    chen666@wisc.edu    9075858895
 *
 * This file includes all the implementations of Badger buffer manager.
 *
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
		try
		{
            // Flushes out all dirty pages
			if (this->bufDescTable[i].dirty) {
				this->bufDescTable[i].file->writePage(this->bufPool[i]);
				this->bufDescTable[i].dirty = false;
			}
		}
		catch (...)
		{
		}
	}
    // Deallocates the buffer pool, the BufDesc table, and hashTable
	delete [] bufDescTable;
    delete [] bufPool;
	delete hashTable;
}

void BufMgr::advanceClock()
{
    // Advance clock to next frame in the buffer pool
	this->clockHand++;
	this->clockHand %= this->numBufs; 
}

void BufMgr::allocBuf(FrameId & frame) 
{
	int count = 0; /* Used to detect all buffer frames are pinned */
    // Allocates a free frame using the clock algorithm
	while (true) {
		this->advanceClock();
		frame = this->clockHand;
		if (!this->bufDescTable[frame].valid) {
			return;
		} else {
			if (this->bufDescTable[frame].refbit) {
				this->bufDescTable[frame].refbit = false;
				continue;
			}
			if (this->bufDescTable[frame].pinCnt > 0) {
				++count;
				if (count == numBufs * 2) {
                    // Throws BufferExceededException if all buffer frames are pinned
					throw BufferExceededException();
				}
				continue;
			}
            // Writing a dirty page back to disk
			if (this->bufDescTable[frame].dirty) {
				this->bufDescTable[frame].file->writePage(this->bufPool[frame]);
			}
            // If the buffer frame allocated has a valid page in it, remove the entry from the hash table
			this->hashTable->remove(this->bufDescTable[frame].file, this->bufDescTable[frame].pageNo);
			this->bufDescTable[frame].Clear();
			return;
		}
	}
}
	
void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
{
	FrameId frameNo;
	try {
        // Check whether the page is already in the buffer pool
		this->hashTable->lookup(file, pageNo, frameNo);
        
        // Page is in the buffer pool
		page = &this->bufPool[frameNo];
		this->bufDescTable[frameNo].refbit = true;
        // Increment the pinCnt for the page
		this->bufDescTable[frameNo].pinCnt++;
	}
	catch (HashNotFoundException)
	{
        // Page is not in the buffer pool
		std::cout << "HashNotFoundException" << std::endl;
        
        //Read the page from disk into the buffer pool frame
		Page diskPage = file->readPage(pageNo);
        
        // Allocate a buffer frame
		this->allocBuf(frameNo);
		this->hashTable->insert(file, pageNo, frameNo);
		this->bufDescTable[frameNo].Set(file, pageNo);
		this->bufPool[frameNo] = diskPage;
		page = &this->bufPool[frameNo];
	}
	catch (...)
	{
		std::cout << "Unknown exception!" << std::endl;
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
		}
		catch (HashNotFoundException)
		{
			std::cout << "Page not in buffer!" << std::endl;
		}
		catch (BadBufferException) {
			std::cout << "An invalid page belonging to the file is encountered!" << std::endl;
		}
    }
}

void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page) 
{
    // Allocate an empty page in the specified file
	Page diskPage = file->allocatePage();
	pageNo = diskPage.page_number();
    
	// Obtain a frame from buffer pool and insert the entry(page)
    FrameId frameNo;
	this->allocBuf(frameNo);
	this->hashTable->insert(file, pageNo, frameNo);
	this->bufDescTable[frameNo].Set(file, pageNo);
	this->bufPool[frameNo] = diskPage;
	page = &this->bufPool[frameNo];
}

void BufMgr::disposePage(File* file, const PageId PageNo)
{
	try {
		FrameId frameNo;
		// Check if the page to be deleted is allocated a frame in the buffer pool
        this->hashTable->lookup(file, PageNo, frameNo);
		
        // Wirte back if the page is dirty, free corresponding table
        if (this->bufDescTable[frameNo].dirty) {
			this->bufDescTable[frameNo].file->writePage(this->bufPool[frameNo]);
		}
		this->bufDescTable[frameNo].Clear();
		this->hashTable->remove(file, PageNo);
	}
	catch (HashNotFoundException)
	{
		std::cout << "Page not in buffer!" << std::endl;
	}
	catch (...)
	{
		std::cout << "Unknown exception!" << std::endl;
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
