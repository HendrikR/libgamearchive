/**
 * @file   fmt-dat-bash.cpp
 * @brief  Implementation of Monster Bash .DAT file reader/writer.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/DAT_Format_%28Monster_Bash%29
 *
 * Copyright (C) 2010-2011 Adam Nielsen <malvineous@shikadi.net>
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

#include <camoto/iostream_helpers.hpp>
#include <camoto/util.hpp>

#include "fmt-dat-bash.hpp"

#define DAT_FIRST_FILE_OFFSET    0
#define DAT_MAX_FILENAME_LEN     30
#define DAT_FILENAME_FIELD_LEN   31

// Length of embedded-FAT entry
#define DAT_EFAT_ENTRY_LEN       37  // filename + offsets

#define DAT_FILETYPE_OFFSET(e)   ((e)->iOffset + 0)
#define DAT_FILESIZE_OFFSET(e)   ((e)->iOffset + 2)
#define DAT_FILENAME_OFFSET(e)   ((e)->iOffset + 4)
#define DAT_DECOMP_OFFSET(e)     ((e)->iOffset + 35)

namespace camoto {
namespace gamearchive {

DAT_BashType::DAT_BashType()
	throw ()
{
}

DAT_BashType::~DAT_BashType()
	throw ()
{
}

std::string DAT_BashType::getArchiveCode() const
	throw ()
{
	return "dat-bash";
}

std::string DAT_BashType::getFriendlyName() const
	throw ()
{
	return "Monster Bash DAT File";
}

std::vector<std::string> DAT_BashType::getFileExtensions() const
	throw ()
{
	std::vector<std::string> vcExtensions;
	vcExtensions.push_back("dat");
	return vcExtensions;
}

std::vector<std::string> DAT_BashType::getGameList() const
	throw ()
{
	std::vector<std::string> vcGames;
	vcGames.push_back("Monster Bash");
	return vcGames;
}

ArchiveType::Certainty DAT_BashType::isInstance(stream::inout_sptr psArchive) const
	throw (stream::error)
{
	stream::pos lenArchive = psArchive->size();
	// TESTED BY: fmt_dat_bash_isinstance_c02
	//if (lenArchive < DAT_FAT_OFFSET) return DefinitelyNo; // too short

	psArchive->seekg(0, stream::start);

	// Check each FAT entry
	char fn[DAT_FILENAME_FIELD_LEN];
	stream::pos pos = 0;
	uint16_t type, lenEntry;
	while (pos < lenArchive) {
		psArchive
			>> u16le(type)
			>> u16le(lenEntry);
		/*switch (type) {
			case 0: //?
			case 1: //?
			case 3:
			case 4:
			case 5:
			case 32:
			case 64:
				break;
			default:
				std::cout << "Unknown filetype number " << type << std::endl;
				return DefinitelyNo;
		}*/
		psArchive->read(fn, DAT_FILENAME_FIELD_LEN);
		// Make sure there aren't any invalid characters in the filename
		for (int j = 0; j < DAT_MAX_FILENAME_LEN; j++) {
			if (!fn[j]) break; // stop on terminating null

			// Fail on control characters in the filename
			if (fn[j] < 32) return DefinitelyNo; // TESTED BY: fmt_dat_bash_isinstance_c01
		}

		pos += lenEntry + DAT_EFAT_ENTRY_LEN;

		// If a file entry points past the end of the archive then it's an invalid
		// format.
		// TESTED BY: fmt_dat_bash_isinstance_c03
		if (pos > lenArchive) return DefinitelyNo;

		psArchive->seekg(pos, stream::start);
	}

	// If we've made it this far, this is almost certainly a DAT file.

	// TESTED BY: fmt_dat_bash_isinstance_c00, c02
	return DefinitelyYes;
}

ArchivePtr DAT_BashType::newArchive(stream::inout_sptr psArchive, SuppData& suppData) const
	throw (stream::error)
{
	return ArchivePtr(new DAT_BashArchive(psArchive));
}

ArchivePtr DAT_BashType::open(stream::inout_sptr psArchive, SuppData& suppData) const
	throw (stream::error)
{
	return ArchivePtr(new DAT_BashArchive(psArchive));
}

SuppFilenames DAT_BashType::getRequiredSupps(const std::string& filenameArchive) const
	throw ()
{
	// No supplemental types/empty list
	return SuppFilenames();
}


DAT_BashArchive::DAT_BashArchive(stream::inout_sptr psArchive)
	throw (stream::error) :
		FATArchive(psArchive, DAT_FIRST_FILE_OFFSET, DAT_MAX_FILENAME_LEN)
{
	stream::pos lenArchive = this->psArchive->size();

	this->psArchive->seekg(0, stream::start);

	stream::pos pos = 0;
	uint16_t type;
	int numFiles = 0;
	while (pos < lenArchive) {
		FATEntry *fatEntry = new FATEntry();
		EntryPtr ep(fatEntry);

		fatEntry->iIndex = numFiles;
		fatEntry->iOffset = pos;
		fatEntry->lenHeader = DAT_EFAT_ENTRY_LEN;
		fatEntry->fAttr = 0;
		fatEntry->bValid = true;

		// Read the data in from the FAT entry in the file
		this->psArchive
			>> u16le(type)
			>> u16le(fatEntry->iSize)
			>> nullPadded(fatEntry->strName, DAT_FILENAME_FIELD_LEN)
			>> u16le(fatEntry->iPrefilteredSize);

		if (fatEntry->iPrefilteredSize) {
			fatEntry->fAttr |= EA_COMPRESSED;
			fatEntry->filter = "lzw-bash"; // decompression algorithm
		}

		switch (type) {
			case 0:
				fatEntry->strName += ".mif";
				fatEntry->type = "map/bash-info";
				break;
			case 1:
				fatEntry->strName += ".mbg";
				fatEntry->type = "map/bash-bg";
				break;
			case 2:
				fatEntry->strName += ".mfg";
				fatEntry->type = "map/bash-fg";
				break;
			case 3:
				fatEntry->strName += ".tbg";
				fatEntry->type = "image/bash-tiles-bg";
				break;
			case 4:
				fatEntry->strName += ".tfg";
				fatEntry->type = "image/bash-tiles-fg";
				break;
			case 5:
				fatEntry->strName += ".tbn";
				fatEntry->type = "image/bash-tiles-fg";
				break;
			case 7:
				fatEntry->strName += ".msp";
				fatEntry->type = "map/bash-sprites";
				break;
			case 8:
				// Already has ".snd" extension
				fatEntry->type = "sound/bash";
				break;
			case 64:
				fatEntry->strName += ".spr";
				fatEntry->type = "image/bash-sprite";
				break;
			case 32:
				fatEntry->type = FILETYPE_GENERIC;
				break;
			default:
				fatEntry->strName += createString("." << type);
				fatEntry->type = createString("unknown/bash-" << type);
				break;
		}
		this->vcFAT.push_back(ep);

		this->psArchive->seekg(fatEntry->iSize, stream::cur);
		pos += DAT_EFAT_ENTRY_LEN + fatEntry->iSize;
		numFiles++;
	}

}

DAT_BashArchive::~DAT_BashArchive()
	throw ()
{
}

int DAT_BashArchive::getSupportedAttributes() const
	throw ()
{
	return EA_COMPRESSED;
}

void DAT_BashArchive::updateFileName(const FATEntry *pid, const std::string& strNewName)
	throw (stream::error)
{
	int typeNum;
	int newLen = strNewName.length();
	std::string ext = strNewName.substr(newLen - 4);
	if (boost::iequals(ext, ".mif")) typeNum = 0;
	else if (boost::iequals(ext, ".mbg")) typeNum = 1;
	else if (boost::iequals(ext, ".mfg")) typeNum = 2;
	else if (boost::iequals(ext, ".tbg")) typeNum = 3;
	else if (boost::iequals(ext, ".tfg")) typeNum = 4;
	else if (boost::iequals(ext, ".tbn")) typeNum = 5;
	else if (boost::iequals(ext, ".msp")) typeNum = 7;
	else if (boost::iequals(ext, ".spr")) typeNum = 64;
	else typeNum = 32;

	std::string strNativeName; // name to write to .dat file
	if (typeNum != 32) {
		// Custom file, chop off extension
		strNativeName = strNewName.substr(0, newLen - 4);
		newLen -= 4;
	} else {
		strNativeName = strNewName;
	}

	// TESTED BY: fmt_dat_bash_rename
	assert(newLen <= DAT_MAX_FILENAME_LEN);

	this->psArchive->seekp(DAT_FILETYPE_OFFSET(pid), stream::start);
	this->psArchive << u16le(typeNum);

	this->psArchive->seekp(DAT_FILENAME_OFFSET(pid), stream::start);
	this->psArchive << nullPadded(strNativeName, DAT_FILENAME_FIELD_LEN);
	return;
}

void DAT_BashArchive::updateFileOffset(const FATEntry *pid,
	stream::delta offDelta
)
	throw (stream::error)
{
	return;
}

void DAT_BashArchive::updateFileSize(const FATEntry *pid,
	stream::delta sizeDelta
)
	throw (stream::error)
{
	// TESTED BY: fmt_dat_bash_insert*
	// TESTED BY: fmt_dat_bash_resize*
	this->psArchive->seekp(DAT_FILESIZE_OFFSET(pid), stream::start);
	this->psArchive << u16le(pid->iSize);

	// Write out the decompressed size too
	this->psArchive->seekp(DAT_FILENAME_FIELD_LEN, stream::cur);
	if (pid->fAttr & EA_COMPRESSED) {
		this->psArchive << u16le(pid->iPrefilteredSize);
	} else {
		this->psArchive << u16le(0);
	}
	return;
}

FATArchive::FATEntry *DAT_BashArchive::preInsertFile(
	const FATEntry *idBeforeThis, FATEntry *pNewEntry
)
	throw (stream::error)
{
	// See if file extension is known and set type appropriately
	int newLen = pNewEntry->strName.length();
	std::string ext = pNewEntry->strName.substr(newLen - 4);
	if (
		boost::iequals(ext, ".mif") ||
		boost::iequals(ext, ".mbg") ||
		boost::iequals(ext, ".mfg") ||
		boost::iequals(ext, ".tbg") ||
		boost::iequals(ext, ".tfg") ||
		boost::iequals(ext, ".tbn") ||
		boost::iequals(ext, ".msp") ||
		boost::iequals(ext, ".spr")
	) {
		newLen -= 4; // don't count the fake extension in the length limit
	}

	// TESTED BY: fmt_dat_bash_insert*
	assert(newLen <= DAT_MAX_FILENAME_LEN);

	// Set the format-specific variables
	pNewEntry->lenHeader = DAT_EFAT_ENTRY_LEN;

	// Because the new entry isn't in the vector yet we need to shift it manually
	//pNewEntry->iOffset += DAT_EFAT_ENTRY_LEN;

	this->psArchive->seekp(pNewEntry->iOffset, stream::start);
	this->psArchive->insert(DAT_EFAT_ENTRY_LEN);
	boost::to_upper(pNewEntry->strName);

	if (pNewEntry->fAttr & EA_COMPRESSED) {
		pNewEntry->filter = "lzw-bash";
	}

	// Since we've inserted some data for the embedded header, we need to update
	// the other file offsets accordingly.  This call updates the offset of the
	// files, then calls updateFileOffset() on them, using the *new* offset, so
	// we need to do this after the insert() call above to make sure the extra
	// data has been inserted.  Then when updateFileOffset() writes data out it
	// will go into the correct spot.
	this->shiftFiles(NULL, pNewEntry->iOffset, pNewEntry->lenHeader, 0);

	return pNewEntry;
}

void DAT_BashArchive::postInsertFile(FATEntry *pNewEntry)
	throw (stream::error)
{
	this->psArchive->seekp(pNewEntry->iOffset, stream::start);

	int typeNum;

	int newLen = pNewEntry->strName.length();
	std::string ext = pNewEntry->strName.substr(newLen - 4);
	if (boost::iequals(ext, ".mif")) typeNum = 0;
	else if (boost::iequals(ext, ".mbg")) typeNum = 1;
	else if (boost::iequals(ext, ".mfg")) typeNum = 2;
	else if (boost::iequals(ext, ".tbg")) typeNum = 3;
	else if (boost::iequals(ext, ".tfg")) typeNum = 4;
	else if (boost::iequals(ext, ".tbn")) typeNum = 5;
	else if (boost::iequals(ext, ".msp")) typeNum = 7;
	else if (boost::iequals(ext, ".spr")) typeNum = 64;
	else typeNum = 32;

	if (typeNum != 32) {
		// Custom file, chop off extension
		pNewEntry->strName = pNewEntry->strName.substr(0, newLen - 4);
	}

	uint16_t expandedSize;
	if (pNewEntry->fAttr & EA_COMPRESSED) {
		expandedSize = pNewEntry->iPrefilteredSize;
	} else {
		expandedSize = 0;
	}

	// Write out the entry
	this->psArchive
		<< u16le(typeNum)
		<< u16le(pNewEntry->iSize)
		<< nullPadded(pNewEntry->strName, DAT_FILENAME_FIELD_LEN)
		<< u16le(expandedSize)
	;
	return;
}

} // namespace gamearchive
} // namespace camoto
