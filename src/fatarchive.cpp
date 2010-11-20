/**
 * @file   fatarchive.cpp
 * @brief  Implementation of a FAT-style archive format.
 *
 * Copyright (C) 2010 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/algorithm/string.hpp>

#include "fatarchive.hpp"
#include <camoto/debug.hpp>

namespace camoto {
namespace gamearchive {

FATArchive::FATEntry::FATEntry()
{
}
FATArchive::FATEntry::~FATEntry()
{
}
std::string FATArchive::FATEntry::getContent() const
{
	std::ostringstream ss;
	ss << this->FileEntry::getContent() << ";offset=" << iOffset;
	return ss.str();
}

Archive::EntryPtr getFileAt(const Archive::VC_ENTRYPTR& files, int index)
{
	for (Archive::VC_ENTRYPTR::const_iterator i = files.begin(); i != files.end(); i++) {
		FATArchive::FATEntry *pEntry = dynamic_cast<FATArchive::FATEntry *>(i->get());
		if (pEntry->iIndex == index) return *i;
	}
	return Archive::EntryPtr();
}

FATArchive::FATArchive(iostream_sptr psArchive, io::stream_offset offFirstFile)
	throw (std::ios::failure) :
		psArchive(new segmented_stream(psArchive)),
		offFirstFile(offFirstFile)
{
}

FATArchive::~FATArchive()
	throw ()
{
	// Can't flush here as it could throw std::ios::failure and we have no way
	// of handling it.
	//this->flush(); // make sure it saves on close just in case
}

const FATArchive::VC_ENTRYPTR& FATArchive::getFileList()
	throw ()
{
	return this->vcFAT;
}

FATArchive::EntryPtr FATArchive::find(const std::string& strFilename)
	throw ()
{
	// TESTED BY: fmt_grp_duke3d_*
	for (VC_ENTRYPTR::iterator i = this->vcFAT.begin(); i != this->vcFAT.end(); i++) {
		const FATEntry *pFAT = dynamic_cast<const FATEntry *>(i->get());
		if (boost::iequals(pFAT->strName, strFilename)) {
			return *i;  // *i is the original shared_ptr
		}
	}
	return EntryPtr();
}

bool FATArchive::isValid(const EntryPtr& id)
	throw ()
{
	const FATEntry *id2 = dynamic_cast<const FATEntry *>(id.get());
	return ((id2) && (id2->bValid));
}

iostream_sptr FATArchive::open(const EntryPtr& id)
	throw ()
{
	// TESTED BY: fmt_grp_duke3d_open

	// Make sure we're not trying to open a folder as a file
	//assert((id->fAttr & EA_FOLDER) == 0);
	// We can't do this because some folder formats have their FAT and
	// everything stored as a "file" in the parent archive, so the subfolder
	// code opens this file (even though it's flagged as a folder) and then
	// passes the data to the Archive.

	const FATEntry *pFAT = dynamic_cast<const FATEntry *>(id.get());
	substream_sptr psSub(
		new substream(
			this->psArchive,
			pFAT->iOffset + pFAT->lenHeader,
			pFAT->iSize
		)
	);
	this->vcSubStream.push_back(psSub);
	return psSub;
}

FATArchive::EntryPtr FATArchive::insert(const EntryPtr& idBeforeThis,
	const std::string& strFilename, offset_t iSize, std::string type, int attr
)
	throw (std::ios::failure)
{
	// TESTED BY: fmt_grp_duke3d_insert2
	// TESTED BY: fmt_grp_duke3d_remove_insert
	// TESTED BY: fmt_grp_duke3d_insert_remove

	FATEntry *pNewFile = this->createNewFATEntry();
	EntryPtr ep(pNewFile);

	pNewFile->strName = strFilename;
	pNewFile->iSize = iSize;
	pNewFile->type = type;
	pNewFile->fAttr = attr;
	pNewFile->bValid = true;

	// Figure out where the new file is going to go
	const FATEntry *pFATBeforeThis = NULL;
	if (this->isValid(idBeforeThis)) {
		// Insert at idBeforeThis
		// TESTED BY: fmt_grp_duke3d_insert_mid
		pFATBeforeThis = dynamic_cast<const FATEntry *>(idBeforeThis.get());
		assert(pFATBeforeThis);
		pNewFile->iOffset = pFATBeforeThis->iOffset;
		pNewFile->iIndex = pFATBeforeThis->iIndex;
	} else {
		// Append to end of archive
		// TESTED BY: fmt_grp_duke3d_insert_end
		if (this->vcFAT.size()) {
			const FATEntry *pFATAfterThis = dynamic_cast<const FATEntry *>(this->vcFAT.back().get());
			assert(pFATAfterThis);
			pNewFile->iOffset = pFATAfterThis->iOffset
				+ pFATAfterThis->lenHeader + pFATAfterThis->iSize;
			pNewFile->iIndex = pFATAfterThis->iIndex + 1;
		} else {
			// There are no files in the archive
			pNewFile->iOffset = this->offFirstFile;
			pNewFile->iIndex = 0;
		}
	}

	// Add the file's entry from the FAT.  May throw (e.g. filename too long),
	// archive should be left untouched in this case.
	FATEntry *returned = this->preInsertFile(pFATBeforeThis, pNewFile);
	if (returned != pNewFile) {
		ep.reset(returned);
		pNewFile = returned;
	}

	if (this->isValid(idBeforeThis)) {
		// Update the offsets of any files located after this one (since they will
		// all have been shifted forward to make room for the insert.)
		this->shiftFiles(
			pNewFile,
			pNewFile->iOffset + pNewFile->lenHeader,
			pNewFile->iSize,
			1
		);

		// Add the new file to the vector now all the existing offsets have been
		// updated.
		// TESTED BY: fmt_grp_duke3d_insert_mid
		VC_ENTRYPTR::iterator itBeforeThis = std::find(this->vcFAT.begin(),
			this->vcFAT.end(), idBeforeThis);
		assert(itBeforeThis != this->vcFAT.end());
		this->vcFAT.insert(itBeforeThis, ep);
	} else {
		// TESTED BY: fmt_grp_duke3d_insert_end
		this->vcFAT.push_back(ep);
	}

	// Insert space for the file's data into the archive.  If there is a header
	// (e.g. embedded FAT) then preInsertFile() will have inserted space for
	// this and written the data, so our insert should start just after the
	// header.
	this->psArchive->seekp(pNewFile->iOffset + pNewFile->lenHeader);
	this->psArchive->insert(pNewFile->iSize);

	this->postInsertFile(pNewFile);

	return ep;
}

void FATArchive::remove(EntryPtr& id)
	throw (std::ios::failure)
{
	// TESTED BY: fmt_grp_duke3d_remove
	// TESTED BY: fmt_grp_duke3d_remove2
	// TESTED BY: fmt_grp_duke3d_remove_insert
	// TESTED BY: fmt_grp_duke3d_insert_remove

	// Make sure the caller doesn't try to remove something that doesn't exist!
	assert(this->isValid(id));

	FATEntry *pFATDel = dynamic_cast<FATEntry *>(id.get());
	assert(pFATDel);

	// Remove the file's entry from the FAT
	this->preRemoveFile(pFATDel);

	// Remove the entry from the vector
	VC_ENTRYPTR::iterator itErase = std::find(this->vcFAT.begin(), this->vcFAT.end(), id);
	assert(itErase != this->vcFAT.end());
	this->vcFAT.erase(itErase);

	// Update the offsets of any files located after this one (since they will
	// all have been shifted back to fill the gap made by the removal.)
	this->shiftFiles(
		pFATDel,
		pFATDel->iOffset,
		-(pFATDel->iSize + pFATDel->lenHeader),
		-1
	);

	// Remove the file's data from the archive
	this->psArchive->seekp(pFATDel->iOffset);
	this->psArchive->remove(pFATDel->iSize + pFATDel->lenHeader);

	// Mark it as invalid in case some other code is still holding on to it.
	pFATDel->bValid = false;

	this->postRemoveFile(pFATDel);

	return;
}

// Enlarge or shrink an existing file entry.
// Postconditions: Existing EntryPtrs and open files remain valid.
void FATArchive::resize(EntryPtr& id, size_t iNewSize)
	throw (std::ios::failure)
{
	assert(this->isValid(id));
	std::streamsize iDelta = iNewSize - id->iSize;
	FATEntry *pFAT = dynamic_cast<FATEntry *>(id.get());

	// Add or remove the data in the underlying stream
	io::stream_offset iStart;
	if (iDelta > 0) { // inserting data
		// TESTED BY: fmt_grp_duke3d_resize_larger
		iStart = pFAT->iOffset + pFAT->lenHeader + pFAT->iSize;
		this->psArchive->seekp(iStart);
		this->psArchive->insert(iDelta);
	} else if (iDelta < 0) { // removing data
		// TESTED BY: fmt_grp_duke3d_resize_smaller
		iStart = pFAT->iOffset + pFAT->lenHeader + iNewSize;
		this->psArchive->seekp(iStart);
		this->psArchive->remove(-iDelta);
	} else {
		return; // no change
	}

	pFAT->iSize = iNewSize;

	// Update the FAT with the file's new size
	this->updateFileSize(pFAT, iDelta);

	// Adjust the in-memory offsets etc. of the rest of the files in the archive,
	// including any open streams.
	this->shiftFiles(pFAT, iStart, iDelta, 0);

	// Resize any open substreams
	for (substream_vc::iterator i = this->vcSubStream.begin();
		i != this->vcSubStream.end();
		i++
	) {
		if (substream_sptr sub = i->lock()) {
			if (sub->getOffset() == pFAT->iOffset) {
				sub->setSize(iNewSize);
				// no break, could be multiple opens for same entry
			}
		}
	}

	return;
}

void FATArchive::flush()
	throw (std::ios::failure)
{
	// Write out to the underlying stream
	assert(this->fnTruncate);
	this->psArchive->commit(this->fnTruncate);

	return;
}

FATArchive::EntryPtr FATArchive::entryPtrFromStream(const iostream_sptr openFile)
	throw ()
{
	substream *d = static_cast<substream *>(openFile.get());
	io::stream_offset offStart = d->getOffset();

	// Find an EntryPtr with the same offset
	for (VC_ENTRYPTR::iterator i = this->vcFAT.begin(); i != this->vcFAT.end(); i++) {
		FATEntry *pFAT = dynamic_cast<FATEntry *>(i->get());
		if (pFAT->iOffset + pFAT->lenHeader >= offStart) {
			return *i;
		}
	}
	return EntryPtr();
}

void FATArchive::shiftFiles(const FATEntry *fatSkip, io::stream_offset offStart,
	std::streamsize deltaOffset, int deltaIndex)
	throw (std::ios::failure)
{
	for (VC_ENTRYPTR::iterator i = this->vcFAT.begin(); i != this->vcFAT.end(); i++) {
		FATEntry *pFAT = dynamic_cast<FATEntry *>(i->get());
		if (pFAT == fatSkip) continue; // don't alter this file
		if (pFAT->iOffset >= offStart) {
			// This file is located after the one we're deleting, so tweak its offset
			pFAT->iOffset += deltaOffset;

			// We have to update the index first, as this is used when inserting
			// files, and it's called *after* the FAT has been updated on-disk.  So
			// the index needs to be adjusted before any further on-disk updates to
			// ensure the right place in the file gets changed.
			pFAT->iIndex += deltaIndex;

			this->updateFileOffset(pFAT, deltaOffset);
		}
	}

	// Relocate any open substreams
	bool clean = false;
	for (substream_vc::iterator i = this->vcSubStream.begin();
		i != this->vcSubStream.end();
		i++
	) {
		if (substream_sptr sub = i->lock()) {
			if (sub->getOffset() >= offStart) {
				sub->relocate(deltaOffset);
			}
		} else clean = true;
	}

	// If one substream has closed, clean up
	if (clean) this->cleanOpenSubstreams();

	return;
}

void FATArchive::postInsertFile(FATEntry *pNewEntry)
	throw (std::ios_base::failure)
{
	// No-op default
}

void FATArchive::postRemoveFile(const FATEntry *pid)
	throw (std::ios_base::failure)
{
	// No-op default
}

FATArchive::FATEntry *FATArchive::createNewFATEntry()
	throw ()
{
	return new FATEntry();
}

void FATArchive::cleanOpenSubstreams()
	throw ()
{
	bool clean;
	do {
		clean = true;
		// Run through the list looking for the first expired item
		for (substream_vc::iterator i = this->vcSubStream.begin();
			i != this->vcSubStream.end();
			i++
		) {
			if (i->expired()) {
				// Found one so remove it and restart the search (since removing an
				// item invalidates the iterator.)
				this->vcSubStream.erase(i);
				clean = false;
				break;
			};
		}
	} while (!clean);

	return;
}

} // namespace gamearchive
} // namespace camoto
