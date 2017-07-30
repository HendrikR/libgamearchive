/**
 * @file  camoto/gamearchive/archive-fat.hpp
 * @brief Implementation of a FAT-style archive format.
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

#ifndef _CAMOTO_ARCHIVE_FAT_HPP_
#define _CAMOTO_ARCHIVE_FAT_HPP_

#include <memory>
#include <map>
#include <camoto/config.hpp>
#include <camoto/stream_sub.hpp>
#include <camoto/stream_seg.hpp>
#include <camoto/gamearchive/archive.hpp>

namespace camoto {
namespace gamearchive {

/// Common value for lenMaxFilename in Archive_FAT::Archive_FAT()
#define ARCH_STD_DOS_FILENAMES  12     // 8.3 + dot

/// Common value for lenMaxFilename in Archive_FAT::Archive_FAT()
#define ARCH_NO_FILENAMES (-1)

/// Archive implementation for archives with an associated size/offset table.
class CAMOTO_GAMEARCHIVE_API Archive_FAT: virtual public Archive,
	public std::enable_shared_from_this<Archive_FAT>
{
	public:

		/// FAT-related fields to add to FileHandle.
		/**
		 * This shouldn't really be public, but sometimes it is handy to access the
		 * FAT fields (especially from within the unit tests.)
		 */
		struct CAMOTO_GAMEARCHIVE_API FATEntry: virtual public File {
			/// Index of file in archive.
			/**
			 * We can't use the index into the vector as entries are passed around
			 * outside the vector.
			 */
			unsigned int iIndex;

			/// Offset of file in archive
			/**
			 * File data may not start at this point.  If lenHeader is nonzero, then
			 * that many bytes must be skipped over to reach the start of the actual
			 * file data.
			 */
			stream::pos iOffset;

			/// Size of embedded FAT entry at start of file data
			/**
			 * This is the amount of data beginning at iOffset that belongs to the
			 * archive format and is not considered part of the archive data.
			 * Each file entry begins at iOffset, has lenHeader bytes until the
			 * file data starts, then has storedSize bytes until the end of the file.
			 */
			stream::len lenHeader;

			/// Empty constructor
			FATEntry();

			/// Empty destructor
			virtual ~FATEntry();

			virtual std::string getContent() const;

			/// Prevent copying
			FATEntry(const FATEntry&) = delete;

			/// Convert a FileHandle into a FATEntry pointer
			inline static FATEntry *cast(const Archive::FileHandle& id)
			{
				return dynamic_cast<Archive_FAT::FATEntry *>(
					const_cast<Archive::File*>(&*id)
				);
			}
		};

	protected:
		/// The archive stream must be mutable, because we need to change it by
		/// seeking and reading data in our get() functions, which don't logically
		/// change the archive's state.
		mutable std::shared_ptr<stream::seg> content;

		/// Offset of the first file in an empty archive.
		stream::pos offFirstFile;

		/// Vector of all files in the archive.
		/**
		 * Although we have a specific FAT type for each entry we can't use a
		 * vector of them here because getFileList() must return a vector of the
		 * base type.  So instead each FAT entry type inherits from the base type
		 * so that the specific FAT entry types can still be added to this vector.
		 *
		 * The entries in this vector can be in any order (not necessarily the
		 * order on-disk.  Use the iIndex member for that.)
		 */
		FileVector vcFAT;

		/// Maximum length of filenames in this archive format.
		unsigned int lenMaxFilename;

		/// Create a new Archive_FAT.
		/**
		 * @param content
		 *   Archive data stream, in whatever format the class descended from this
		 *   can handle.
		 *
		 * @param offFirstFile
		 *   The offset (from the start of the archive) where the first file should
		 *   be inserted, if the archive has no existing files.
		 *
		 * @param lenMaxFilename
		 *   Maximum length of the filename including the dot if 8.3 format.  Set
		 *   to zero if there is no limit.  Used by verify() to ensure the
		 *   filename is valid before being passed to insert().  The predefined
		 *   constant %ARCH_STD_DOS_FILENAMES can be used for 8.3 files.
		 *
		 * @throws stream::error on I/O error.
		 */
		Archive_FAT(std::unique_ptr<stream::inout> content, stream::pos offFirstFile,
			int lenMaxFilename);

		// Empty constructor for descendent virtual classes.
		Archive_FAT();

	public:
		virtual ~Archive_FAT();

		virtual const FileHandle find(const std::string& strFilename) const;
		virtual const FileVector& files(void) const;
		virtual bool isValid(const FileHandle& id) const;
		virtual std::unique_ptr<stream::inout> open(const FileHandle& id,
			bool useFilter);
		virtual std::shared_ptr<Archive> openFolder(const FileHandle& id);
		virtual const FileHandle insert(const FileHandle& idBeforeThis,
			const std::string& strFilename, stream::len storedSize, std::string type,
			File::Attribute attr);
		virtual void remove(const FileHandle& id);
		virtual void rename(const FileHandle& id, const std::string& strNewName);
		virtual void move(const FileHandle& idBeforeThis, const FileHandle& id);
		virtual void resize(const FileHandle& id, stream::len newStoredSize,
			stream::len newRealSize);
		virtual void flush();

	protected:
		/// Shift any files *starting* at or after offStart by delta bytes.
		/**
		 * This updates the internal offsets and index numbers.  The FAT is updated
		 * by calling updateFileOffset().  If offStart is in the middle of a file
		 * (which should never happen) that file won't be affected, only those
		 * following it.  This function must notify any open files that their offset
		 * has moved.
		 *
		 * @param fatSkip
		 *   Do not alter this entry, even if it is located in the area to be
		 *   affected.
		 *
		 * @param offStart
		 *   Files starting at or after this offset are affected.
		 *
		 * @param deltaOffset
		 *   How many bytes to add or subtract from each affected file's offset
		 *   field.
		 *
		 * @param deltaIndex
		 *   Value to add or subtract from each affected file's index field.
		 *
		 * @throws stream::error on I/O error.
		 */
		virtual void shiftFiles(const FATEntry *fatSkip, stream::pos offStart,
			stream::delta deltaOffset, int deltaIndex);

		// Methods to be filled out by descendent classes

		/// Adjust the name of the given file in the on-disk FAT.
		/**
		 * @param pid
		 *   The entry to update.
		 *
		 * @param name
		 *   New filename.  This will be within the maximum length passed to the
		 *   constructor, so this function does not need to check that the filename
		 *   length is within range.
		 *
		 * @throws stream::error on I/O error.
		 *
		 * @return If this function returns (as opposed to throwing an exception)
		 *   then the filename in pid will be updated.  If an exception is thrown
		 *   the filename will be unchanged.
		 *
		 * @note If zero was passed to the constructor as the maximum filename
		 *   length then the length check will not happen and this function will
		 *   need to ensure the filename length is within the limit (if there is
		 *   one).
		 *
		 * @note The default implementation of this function throws an exception
		 *   explaining that the file format does not store filenames, so you only
		 *   need to override it for file formats that have a field storing the
		 *   name of each file.
		 */
		virtual void updateFileName(const FATEntry *pid, const std::string& name);

		/// Adjust the offset of the given file in the on-disk FAT.
		/**
		 * @param pid
		 *   The entry to update.  pid->offset is already set to the new offset.
		 *
		 * @param offDelta
		 *   Amount the offset has changed, in case this value is needed.
		 *
		 * @throws stream::error on I/O error.
		 *
		 * @note pid->offset is already set to the new offset, do not add offDelta
		 *   to this or you will get the wrong offset!
		 *
		 * @note The default implementation of this function does nothing, so you
		 *   only need to override it for file formats that have a field storing the
		 *   offset of each file.
		 */
		virtual void updateFileOffset(const FATEntry *pid, stream::delta offDelta);

		/// Adjust the size of the given file in the on-disk FAT.
		/**
		 * @param pid
		 *   The entry to update.  pid->size is already set to the new size.
		 *
		 * @param sizeDelta
		 *   Amount the size has changed, in case this value is needed.
		 *
		 * @throws stream::error on I/O error.
		 *
		 * @note pid->size is already set to the new size, do not add sizeDelta
		 *   to this or you will get the wrong size!
		 *
		 * @note The default implementation of this function does nothing, so you
		 *   only need to override it for file formats that have a field storing the
		 *   size of each file.
		 */
		virtual void updateFileSize(const FATEntry *pid, stream::delta sizeDelta);

		/// Insert a new entry in the on-disk FAT.
		/**
		 * It should be inserted before idBeforeThis, or at the end of the archive
		 * if idBeforeThis is not valid.  All the FAT entries will be updated with
		 * new offsets after this function returns (so this function *must* add a
		 * new entry into the on-disk FAT for this file) however the offsets will
		 * not take into account any changes resulting from the FAT changing size,
		 * which must be handled by this function.  The FAT vector does not contain
		 * the new entry, so pNewEntry->iIndex may be the same as an existing file
		 * (but the existing file will have its index moved after this function
		 * returns.)  All this function has to do is make room in the FAT and write
		 * out the new entry.  It also needs to set the lenHeader field in
		 * pNewEntry.  If createNewFATEntry() has been overridden to return a custom
		 * class, then dynamic_cast<> can be used to convert pNewEntry into an
		 * instance of that class.
		 *
		 * @param idBeforeThis
		 *   The new file is to be inserted before this.  If it is invalid (null)
		 *   the new file should be appended to the end of the archive.
		 *
		 * @param pNewEntry
		 *   Initial details about the new entry.  Populate these as required.  This
		 *   instance is created by createNewFATEntry() so if you override that
		 *   function to generate a custom type, you can cast this value to obtain
		 *   the custom type within this function.
		 *
		 * @throws stream::error on I/O error.
		 */
		virtual void preInsertFile(const FATEntry *idBeforeThis,
			FATEntry *pNewEntry);

		/// Called after the file data has been inserted.
		/**
		 * Only needs to be overridden if there are tasks to perform after the file
		 * has been set.  pNewEntry can be changed if need be, but this is not
		 * required.
		 *
		 * @param pNewEntry
		 *   New file that was just inserted.  May be modified.
		 *
		 * @throws stream::error on I/O error.
		 *
		 * @note preInsertFile() and all subsequent FAT updates and file shifting
		 * is done without the new file, then the new file data is inserted last,
		 * and postInsertFile() immediately called.
		 */
		virtual void postInsertFile(FATEntry *pNewEntry);

		/// Remove the entry from the FAT.
		/**
		 * The file data has not yet been removed from the archive, and the offsets
		 * have not yet been updated.  On return, pid will be removed from the FAT
		 * vector, the on-disk offsets of files following this one will be updated,
		 * then the file content will be removed.
		 *
		 * The offsets are updated via calls to updateFileOffset(), so they don't
		 * need changing by this function.  However the offsets will not take into
		 * account any changes resulting from the FAT changing size, which must be
		 * handled by this function.
		 *
		 * @param pid
		 *   Entry being removed.
		 *
		 * @throws stream::error on I/O error.
		 */
		virtual void preRemoveFile(const FATEntry *pid);

		/// Called after the file data has been removed and the FAT has been
		/// updated.
		/**
		 * Only override if needed.  Note that pid->bValid will be false (because
		 * the file has been removed) but for this function only, the other
		 * parameters are still correct, although no longer used (e.g. the offset
		 * it was at, its size, etc.)
		 *
		 * @param pid
		 *   Entry being removed.
		 *
		 * @throws stream::error on I/O error.
		 */
		virtual void postRemoveFile(const FATEntry *pid);

		/// Allocate a new, empty FAT entry.
		/**
		 * This function creates a new FATEntry instance.  A default implementation
		 * is provided which creates a new FATEntry instance.  If you are
		 * implementing a new archive format and you need to extend FATEntry to hold
		 * additional information, you will need to replace this function with one
		 * that allocates your extended class instead, otherwise the FATEntry
		 * pointers passed to the other functions will be a mixture of FATEntry and
		 * whatever your extended class is from your constructor.  See
		 * fmt-dat-hugo.cpp for an example.
		 */
		virtual std::unique_ptr<FATEntry> createNewFATEntry();

	private:
		/// Should the given entry be moved during an insert/resize operation?
		bool entryInRange(const FATEntry *fat, stream::pos offStart,
			const FATEntry *fatSkip);
};

} // namespace gamearchive
} // namespace camoto

#endif // _CAMOTO_ARCHIVE_FAT_HPP_
