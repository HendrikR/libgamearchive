/**
 * @file  fmt-wad-doom.cpp
 * @brief Implementation of Doom .WAD file reader/writer.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/WAD_Format
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

#include <cassert>
#include <camoto/iostream_helpers.hpp>
#include <camoto/util.hpp>
#include "fmt-wad-doom.hpp"

#define WAD_FILECOUNT_OFFSET    4
#define WAD_HEADER_LEN          12
#define WAD_FAT_OFFSET          WAD_HEADER_LEN // assuming no extra data after header
#define WAD_FILENAME_FIELD_LEN  8
#define WAD_MAX_FILENAME_LEN    WAD_FILENAME_FIELD_LEN
#define WAD_FAT_ENTRY_LEN       16
#define WAD_FIRST_FILE_OFFSET   WAD_HEADER_LEN  // empty archive only

#define WAD_SAFETY_MAX_FILECOUNT  8192 // Maximum value we will load

#define WAD_FATENTRY_OFFSET(e) (WAD_HEADER_LEN + e->iIndex * WAD_FAT_ENTRY_LEN)

#define WAD_FILEOFFSET_OFFSET(e) WAD_FATENTRY_OFFSET(e)
#define WAD_FILESIZE_OFFSET(e) (WAD_FATENTRY_OFFSET(e) + 4)
#define WAD_FILENAME_OFFSET(e) (WAD_FATENTRY_OFFSET(e) + 8)

namespace camoto {
namespace gamearchive {

ArchiveType_WAD_Doom::ArchiveType_WAD_Doom()
{
}

ArchiveType_WAD_Doom::~ArchiveType_WAD_Doom()
{
}

std::string ArchiveType_WAD_Doom::code() const
{
	return "wad-doom";
}

std::string ArchiveType_WAD_Doom::friendlyName() const
{
	return "Doom WAD File";
}

std::vector<std::string> ArchiveType_WAD_Doom::fileExtensions() const
{
	return {
		"wad",
		"rts",
	};
}

std::vector<std::string> ArchiveType_WAD_Doom::games() const
{
	return {
		"Doom",
		"Duke Nukem 3D",
		"Heretic",
		"Hexen",
		"Redneck Rampage",
		"Rise of the Triad",
		"Shadow Warrior",
	};
}

ArchiveType::Certainty ArchiveType_WAD_Doom::isInstance(
	stream::input& content) const
{
	stream::pos lenArchive = content.size();

	// TESTED BY: fmt_wad_doom_isinstance_c03
	if (lenArchive < WAD_HEADER_LEN) return Certainty::DefinitelyNo; // too short

	char sig[4];
	content.seekg(0, stream::start);
	content.read(sig, 4);

	// TESTED BY: fmt_wad_doom_isinstance_c00
	if (strncmp(sig, "IWAD", 4) == 0) return Certainty::DefinitelyYes;

	// TESTED BY: fmt_wad_doom_isinstance_c01
	if (strncmp(sig, "PWAD", 4) == 0) return Certainty::DefinitelyYes;

	// TESTED BY: fmt_wad_doom_isinstance_c02
	return Certainty::DefinitelyNo;
}

std::shared_ptr<Archive> ArchiveType_WAD_Doom::create(
	std::unique_ptr<stream::inout> content, SuppData& suppData) const
{
	content->seekp(0, stream::start);
	content->write("IWAD\x00\x00\x00\x00\x0c\x00\x00\x00", WAD_HEADER_LEN);
	return std::make_shared<Archive_WAD_Doom>(std::move(content));
}

std::shared_ptr<Archive> ArchiveType_WAD_Doom::open(
	std::unique_ptr<stream::inout> content, SuppData& suppData) const
{
	return std::make_shared<Archive_WAD_Doom>(std::move(content));
}

SuppFilenames ArchiveType_WAD_Doom::getRequiredSupps(stream::input& content,
	const std::string& filename) const
{
	return {};
}


Archive_WAD_Doom::Archive_WAD_Doom(std::unique_ptr<stream::inout> content)
	:	Archive_FAT(std::move(content), WAD_FIRST_FILE_OFFSET, WAD_MAX_FILENAME_LEN)
{
	this->content->seekg(4, stream::start); // skip sig

	// We still have to perform sanity checks in case the user forced an archive
	// to open even though it failed the signature check.
	if (this->content->tellg() != 4) throw stream::error("file too short");

	uint32_t numFiles, offFAT;
	*this->content
		>> u32le(numFiles)
		>> u32le(offFAT)
	;

	if (numFiles >= WAD_SAFETY_MAX_FILECOUNT) {
		throw stream::error("too many files or corrupted archive");
	}

	this->content->seekg(offFAT, stream::start);
	for (unsigned int i = 0; i < numFiles; i++) {
		auto f = this->createNewFATEntry();

		f->iIndex = i;
		f->lenHeader = 0;
		f->type = FILETYPE_GENERIC;
		f->fAttr = File::Attribute::Default;
		f->bValid = true;

		// Read the data in from the FAT entry in the file
		*this->content
			>> u32le(f->iOffset)
			>> u32le(f->storedSize)
			>> nullPadded(f->strName, WAD_FILENAME_FIELD_LEN)
		;

		f->realSize = f->storedSize;
		this->vcFAT.push_back(std::move(f));
	}

	// Read metadata
	this->v_attributes.emplace_back();
	auto& attrType = this->v_attributes.back();
	attrType.changed = false;
	attrType.type = Attribute::Type::Enum;
	attrType.name = "Type";
	attrType.desc = "Type of WAD format.  IWAD files must contain all data for "
		"the game.  PWAD files take priority and can override files, with any "
		"files missing from a PWAD being read from the IWAD instead.  In other "
		"words, an IWAD contains the original game, and a PWAD contains a mod, "
		"which replaces some parts of the original game where needed.";
	attrType.enumValueNames.emplace_back("IWAD");
	attrType.enumValueNames.emplace_back("PWAD");

	this->content->seekg(0, stream::start);
	char wadType;
	this->content->read(&wadType, 1);
	if (wadType == 'I') attrType.enumValue = 0;
	else attrType.enumValue = 1;
}

Archive_WAD_Doom::~Archive_WAD_Doom()
{
}

void Archive_WAD_Doom::flush()
{
	auto& attrType = this->v_attributes[0];
	if (attrType.changed) {
		uint8_t val;
		switch (attrType.enumValue) {
			case 0: val = 'I'; break;
			case 1: val = 'P'; break;
			default: throw camoto::error("Unknown WAD type.");
		}
		this->content->seekp(0, stream::start);
		*this->content << u8(val);
		attrType.changed = false;
	}
	this->Archive_FAT::flush();
	return;
}

void Archive_WAD_Doom::updateFileName(const FATEntry *pid, const std::string& strNewName)
{
	// TESTED BY: fmt_wad_doom_rename
	assert(strNewName.length() <= WAD_MAX_FILENAME_LEN);
	this->content->seekp(WAD_FILENAME_OFFSET(pid), stream::start);
	*this->content << nullPadded(strNewName, WAD_FILENAME_FIELD_LEN);
	return;
}

void Archive_WAD_Doom::updateFileOffset(const FATEntry *pid, stream::delta offDelta)
{
	// TESTED BY: fmt_wad_doom_insert*
	// TESTED BY: fmt_wad_doom_resize*
	this->content->seekp(WAD_FILEOFFSET_OFFSET(pid), stream::start);
	*this->content << u32le(pid->iOffset);
	return;
}

void Archive_WAD_Doom::updateFileSize(const FATEntry *pid, stream::delta sizeDelta)
{
	// TESTED BY: fmt_wad_doom_insert*
	// TESTED BY: fmt_wad_doom_resize*
	this->content->seekp(WAD_FILESIZE_OFFSET(pid), stream::start);
	*this->content << u32le(pid->storedSize);
	return;
}

void Archive_WAD_Doom::preInsertFile(const FATEntry *idBeforeThis, FATEntry *pNewEntry)
{
	// TESTED BY: fmt_wad_doom_insert*
	assert(pNewEntry->strName.length() <= WAD_MAX_FILENAME_LEN);

	// Set the format-specific variables
	pNewEntry->lenHeader = 0;

	// Because the new entry isn't in the vector yet we need to shift it manually
	pNewEntry->iOffset += WAD_FAT_ENTRY_LEN;

	this->content->seekp(WAD_FATENTRY_OFFSET(pNewEntry), stream::start);
	this->content->insert(WAD_FAT_ENTRY_LEN);
	camoto::uppercase(pNewEntry->strName);

	*this->content
		<< u32le(pNewEntry->iOffset)
		<< u32le(pNewEntry->storedSize)
		<< nullPadded(pNewEntry->strName, WAD_FILENAME_FIELD_LEN)
	;

	// Update the offsets now there's a new FAT entry taking up space.
	this->shiftFiles(
		NULL,
		WAD_FAT_OFFSET + this->vcFAT.size() * WAD_FAT_ENTRY_LEN,
		WAD_FAT_ENTRY_LEN,
		0
	);

	this->updateFileCount(this->vcFAT.size() + 1);
	return;
}

void Archive_WAD_Doom::preRemoveFile(const FATEntry *pid)
{
	// TESTED BY: fmt_wad_doom_remove*

	// Update the offsets now there's one less FAT entry taking up space.  This
	// must be called before the FAT is altered, because it will write a new
	// offset into the FAT entry we're about to erase (and if we erase it first
	// it'll overwrite something else.)
	this->shiftFiles(
		NULL,
		WAD_FAT_OFFSET + this->vcFAT.size() * WAD_FAT_ENTRY_LEN,
		-WAD_FAT_ENTRY_LEN,
		0
	);

	this->content->seekp(WAD_FATENTRY_OFFSET(pid), stream::start);
	this->content->remove(WAD_FAT_ENTRY_LEN);

	this->updateFileCount(this->vcFAT.size() - 1);
	return;
}

void Archive_WAD_Doom::updateFileCount(uint32_t iNewCount)
{
	// TESTED BY: fmt_wad_doom_insert*
	// TESTED BY: fmt_wad_doom_remove*
	this->content->seekp(WAD_FILECOUNT_OFFSET, stream::start);
	*this->content << u32le(iNewCount);
	return;
}

} // namespace gamearchive
} // namespace camoto
