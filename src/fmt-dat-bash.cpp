/**
 * @file  fmt-dat-bash.cpp
 * @brief Implementation of Monster Bash .DAT file reader/writer.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/DAT_Format_%28Monster_Bash%29
 *
 * Copyright (C) 2010-2016 Adam Nielsen <malvineous@shikadi.net>
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

ArchiveType_DAT_Bash::ArchiveType_DAT_Bash()
{
}

ArchiveType_DAT_Bash::~ArchiveType_DAT_Bash()
{
}

std::string ArchiveType_DAT_Bash::code() const
{
	return "dat-bash";
}

std::string ArchiveType_DAT_Bash::friendlyName() const
{
	return "Monster Bash DAT File";
}

std::vector<std::string> ArchiveType_DAT_Bash::fileExtensions() const
{
	return {
		"dat",
	};
}

std::vector<std::string> ArchiveType_DAT_Bash::games() const
{
	return {
		"Monster Bash",
	};
}

ArchiveType::Certainty ArchiveType_DAT_Bash::isInstance(
	stream::input& content) const
{
	stream::pos lenArchive = content.size();
	// TESTED BY: fmt_dat_bash_isinstance_c02
	//if (lenArchive < DAT_FAT_OFFSET) return Certainty::DefinitelyNo; // too short

	content.seekg(0, stream::start);

	// Check each FAT entry
	char fn[DAT_FILENAME_FIELD_LEN];
	stream::pos pos = 0;
	uint16_t type, lenEntry;
	while (pos < lenArchive) {
		if (pos + DAT_EFAT_ENTRY_LEN > lenArchive) {
			// File ends on an incomplete FAT entry
			// TESTED BY: fmt_dat_bash_isinstance_c04
			return Certainty::DefinitelyNo;
		}
		content
			>> u16le(type)
			>> u16le(lenEntry)
		;
		content.read(fn, DAT_FILENAME_FIELD_LEN);
		// Make sure there aren't any invalid characters in the filename
		for (int j = 0; j < DAT_MAX_FILENAME_LEN; j++) {
			if (!fn[j]) break; // stop on terminating null

			// Fail on control characters in the filename
			if (fn[j] < 32) return Certainty::DefinitelyNo; // TESTED BY: fmt_dat_bash_isinstance_c01
		}

		pos += lenEntry + DAT_EFAT_ENTRY_LEN;

		// If a file entry points past the end of the archive then it's an invalid
		// format.
		// TESTED BY: fmt_dat_bash_isinstance_c03
		if (pos > lenArchive) return Certainty::DefinitelyNo;

		content.seekg(pos, stream::start);
	}

	// If we've made it this far, this is almost certainly a DAT file.

	// TESTED BY: fmt_dat_bash_isinstance_c00, c02
	return Certainty::DefinitelyYes;
}

std::shared_ptr<Archive> ArchiveType_DAT_Bash::create(
	std::unique_ptr<stream::inout> content, SuppData& suppData) const
{
	return std::make_shared<Archive_DAT_Bash>(std::move(content));
}

std::shared_ptr<Archive> ArchiveType_DAT_Bash::open(
	std::unique_ptr<stream::inout> content, SuppData& suppData) const
{
	return std::make_shared<Archive_DAT_Bash>(std::move(content));
}

SuppFilenames ArchiveType_DAT_Bash::getRequiredSupps(stream::input& content,
	const std::string& filename) const
{
	return {};
}


Archive_DAT_Bash::Archive_DAT_Bash(std::unique_ptr<stream::inout> content)
	:	Archive_FAT(std::move(content), DAT_FIRST_FILE_OFFSET, DAT_MAX_FILENAME_LEN)
{
	stream::pos lenArchive = this->content->size();

	this->content->seekg(0, stream::start);

	stream::pos pos = 0;
	uint16_t type;
	int numFiles = 0;
	while (pos < lenArchive) {
		auto f = this->createNewFATEntry();

		f->iIndex = numFiles;
		f->iOffset = pos;
		f->lenHeader = DAT_EFAT_ENTRY_LEN;
		f->fAttr = File::Attribute::Default;
		f->bValid = true;

		// Read the data in from the FAT entry in the file
		*this->content
			>> u16le(type)
			>> u16le(f->storedSize)
			>> nullPadded(f->strName, DAT_FILENAME_FIELD_LEN)
			>> u16le(f->realSize);

		if (f->realSize) {
			f->fAttr |= File::Attribute::Compressed;
			f->filter = "lzw-bash"; // decompression algorithm
		} else {
			f->realSize = f->storedSize;
		}

		switch (type) {
			case 0:
				f->strName += ".mif";
				f->type = "map/bash-info";
				break;
			case 1:
				f->strName += ".mbg";
				f->type = "map/bash-bg";
				break;
			case 2:
				f->strName += ".mfg";
				f->type = "map/bash-fg";
				break;
			case 3:
				f->strName += ".tbg";
				f->type = "image/bash-tiles-bg";
				break;
			case 4:
				f->strName += ".tfg";
				f->type = "image/bash-tiles-fg";
				break;
			case 5:
				f->strName += ".tbn";
				f->type = "image/bash-tiles-fg";
				break;
			case 6:
				f->strName += ".sgl";
				f->type = "data/bash-sprite-graphics-list";
				break;
			case 7:
				f->strName += ".msp";
				f->type = "map/bash-sprites";
				break;
			case 8:
				// Already has ".snd" extension
				f->type = "sound/bash";
				break;
			case 12:
				f->strName += ".pbg";
				f->type = "data/bash-tile-properties";
				break;
			case 13:
				f->strName += ".pfg";
				f->type = "data/bash-tile-properties";
				break;
			case 14:
				f->strName += ".pal";
				f->type = "image/pal-ega";
				break;
			case 16:
				f->strName += ".pbn";
				f->type = "data/bash-tile-properties";
				break;
			case 64:
				f->strName += ".spr";
				f->type = "image/bash-sprite";
				break;
			case 32:
				f->type = FILETYPE_GENERIC;
				break;
			default:
				f->strName += createString("." << type);
				f->type = createString("unknown/bash-" << type);
				break;
		}
		this->content->seekg(f->storedSize, stream::cur);
		pos += DAT_EFAT_ENTRY_LEN + f->storedSize;

		this->vcFAT.push_back(std::move(f));
		numFiles++;
	}
}

Archive_DAT_Bash::~Archive_DAT_Bash()
{
}

Archive::File::Attribute Archive_DAT_Bash::getSupportedAttributes() const
{
	return File::Attribute::Compressed;
}

void Archive_DAT_Bash::updateFileName(const FATEntry *pid, const std::string& strNewName)
{
	int typeNum;
	int newLen = strNewName.length();
	std::string ext = strNewName.substr(newLen - 4);
	if (camoto::icasecmp(ext, ".mif")) typeNum = 0;
	else if (camoto::icasecmp(ext, ".mbg")) typeNum = 1;
	else if (camoto::icasecmp(ext, ".mfg")) typeNum = 2;
	else if (camoto::icasecmp(ext, ".tbg")) typeNum = 3;
	else if (camoto::icasecmp(ext, ".tfg")) typeNum = 4;
	else if (camoto::icasecmp(ext, ".tbn")) typeNum = 5;
	else if (camoto::icasecmp(ext, ".sgl")) typeNum = 6;
	else if (camoto::icasecmp(ext, ".msp")) typeNum = 7;
	else if (camoto::icasecmp(ext, ".snd")) typeNum = 8;
	else if (camoto::icasecmp(ext, ".pbg")) typeNum = 12;
	else if (camoto::icasecmp(ext, ".pfg")) typeNum = 13;
	else if (camoto::icasecmp(ext, ".pal")) typeNum = 14;
	else if (camoto::icasecmp(ext, ".pbn")) typeNum = 16;
	else if (camoto::icasecmp(ext, ".spr")) typeNum = 64;
	else typeNum = 32;

	std::string strNativeName; // name to write to .dat file
	if ((typeNum != 32) && (typeNum != 8)) {
		// Custom file, chop off extension
		strNativeName = strNewName.substr(0, newLen - 4);
		newLen -= 4;
	} else {
		strNativeName = strNewName;
	}

	// TESTED BY: fmt_dat_bash_rename
	assert(newLen <= DAT_MAX_FILENAME_LEN);

	this->content->seekp(DAT_FILETYPE_OFFSET(pid), stream::start);
	*this->content << u16le(typeNum);

	this->content->seekp(DAT_FILENAME_OFFSET(pid), stream::start);
	*this->content << nullPadded(strNativeName, DAT_FILENAME_FIELD_LEN);
	return;
}

void Archive_DAT_Bash::updateFileOffset(const FATEntry *pid,
	stream::delta offDelta)
{
	return;
}

void Archive_DAT_Bash::updateFileSize(const FATEntry *pid,
	stream::delta sizeDelta)
{
	if (pid->storedSize > 65535) {
		throw stream::error(createString("The file \"" << pid->strName
			<< "\" cannot be expanded to the requested size of " << pid->storedSize
			<< " bytes, as the Monster Bash .DAT file cannot store files larger than "
			"65535 bytes."));
	}
	if (pid->realSize > 65535) {
		throw stream::error(createString("The file \"" << pid->strName
			<< "\" cannot have its decompressed size set to " << pid->storedSize
			<< " bytes, as the Monster Bash .DAT file cannot store files that are "
			"larger than 65535 bytes, before or after decompression."));
	}

	// TESTED BY: fmt_dat_bash_insert*
	// TESTED BY: fmt_dat_bash_resize*
	this->content->seekp(DAT_FILESIZE_OFFSET(pid), stream::start);
	*this->content << u16le(pid->storedSize);

	// Write out the decompressed size too
	this->content->seekp(DAT_FILENAME_FIELD_LEN, stream::cur);
	if (pid->fAttr & File::Attribute::Compressed) {
		*this->content << u16le(pid->realSize);
	} else {
		*this->content << u16le(0);
	}
	return;
}

void Archive_DAT_Bash::preInsertFile(const FATEntry *idBeforeThis,
	FATEntry *pNewEntry)
{
	// See if file extension is known and set type appropriately
	int newLen = pNewEntry->strName.length();
	std::string ext = pNewEntry->strName.substr(newLen - 4);
	if (
		camoto::icasecmp(ext, ".mif") ||
		camoto::icasecmp(ext, ".mbg") ||
		camoto::icasecmp(ext, ".mfg") ||
		camoto::icasecmp(ext, ".tbg") ||
		camoto::icasecmp(ext, ".tfg") ||
		camoto::icasecmp(ext, ".tbn") ||
		camoto::icasecmp(ext, ".sgl") ||
		camoto::icasecmp(ext, ".msp") ||
		camoto::icasecmp(ext, ".pbg") ||
		camoto::icasecmp(ext, ".pfg") ||
		camoto::icasecmp(ext, ".pal") ||
		camoto::icasecmp(ext, ".pbn") ||
		camoto::icasecmp(ext, ".spr")
	) {
		newLen -= 4; // don't count the fake extension in the length limit
	}

	// TESTED BY: fmt_dat_bash_insert*
	assert(newLen <= DAT_MAX_FILENAME_LEN);

	// Set the format-specific variables
	pNewEntry->lenHeader = DAT_EFAT_ENTRY_LEN;

	// Because the new entry isn't in the vector yet we need to shift it manually
	//pNewEntry->iOffset += DAT_EFAT_ENTRY_LEN;

	this->content->seekp(pNewEntry->iOffset, stream::start);
	this->content->insert(DAT_EFAT_ENTRY_LEN);

	if (pNewEntry->fAttr & File::Attribute::Compressed) {
		pNewEntry->filter = "lzw-bash";
	}

	// Since we've inserted some data for the embedded header, we need to update
	// the other file offsets accordingly.  This call updates the offset of the
	// files, then calls updateFileOffset() on them, using the *new* offset, so
	// we need to do this after the insert() call above to make sure the extra
	// data has been inserted.  Then when updateFileOffset() writes data out it
	// will go into the correct spot.
	this->shiftFiles(NULL, pNewEntry->iOffset, pNewEntry->lenHeader, 0);

	return;
}

void Archive_DAT_Bash::postInsertFile(FATEntry *pNewEntry)
{
	this->content->seekp(pNewEntry->iOffset, stream::start);

	int typeNum;

	int newLen = pNewEntry->strName.length();
	std::string ext = pNewEntry->strName.substr(newLen - 4);
	if (camoto::icasecmp(ext, ".mif")) typeNum = 0;
	else if (camoto::icasecmp(ext, ".mbg")) typeNum = 1;
	else if (camoto::icasecmp(ext, ".mfg")) typeNum = 2;
	else if (camoto::icasecmp(ext, ".tbg")) typeNum = 3;
	else if (camoto::icasecmp(ext, ".tfg")) typeNum = 4;
	else if (camoto::icasecmp(ext, ".tbn")) typeNum = 5;
	else if (camoto::icasecmp(ext, ".sgl")) typeNum = 6;
	else if (camoto::icasecmp(ext, ".msp")) typeNum = 7;
	else if (camoto::icasecmp(ext, ".snd")) typeNum = 8;
	else if (camoto::icasecmp(ext, ".pbg")) typeNum = 12;
	else if (camoto::icasecmp(ext, ".pfg")) typeNum = 13;
	else if (camoto::icasecmp(ext, ".pal")) typeNum = 14;
	else if (camoto::icasecmp(ext, ".pbn")) typeNum = 16;
	else if (camoto::icasecmp(ext, ".spr")) typeNum = 64;
	else typeNum = 32;

	if ((typeNum != 32) && (typeNum != 8)) {
		// Custom file, chop off extension
		pNewEntry->strName = pNewEntry->strName.substr(0, newLen - 4);
	}

	uint16_t expandedSize;
	if (pNewEntry->fAttr & File::Attribute::Compressed) {
		expandedSize = pNewEntry->realSize;
	} else {
		expandedSize = 0;
	}

	// Write out the entry
	*this->content
		<< u16le(typeNum)
		<< u16le(pNewEntry->storedSize)
		<< nullPadded(pNewEntry->strName, DAT_FILENAME_FIELD_LEN)
		<< u16le(expandedSize)
	;
	return;
}

} // namespace gamearchive
} // namespace camoto
