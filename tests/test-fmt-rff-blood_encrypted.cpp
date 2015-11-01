/**
 * @file   test-arch-rff-blood_encrypted.cpp
 * @brief  Test code for encrypted Blood .RFF archives.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
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

#define DATA_ONE   "Thhr\"kp#kj`+bgs"
#define DATA_TWO   "Thhr\"kp#psj+bgs"
#define DATA_THREE "Thhr\"kp#plw`c(cf|"
#define DATA_FOUR  "Thhr\"kp#bkpw(bfs"

#include "test-archive.hpp"

class test_rff_blood_encrypted: public test_archive
{
	public:
		test_rff_blood_encrypted()
		{
			this->type = "rff-blood";
			this->lenMaxFilename = 12;
			this->insertAttr = Archive::File::Attribute::Encrypted;

			Attribute ver;
			ver.type = Attribute::Type::Enum;
			ver.enumValue = 1; // version 0x301
			this->attributes.push_back(ver);
		}

		virtual std::string initialstate()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x3e\x00\x00\x00" "\x02\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_ONE
				DATA_TWO
				"\x3E\x3E\x3F\x3F\x40\x40\x41\x41\x42\x42\x43\x43\x44\x44\x45\x45"
				"\x66\x46\x47\x47\x47\x48\x49\x49\x4A\x4A\x4B\x4B\x4C\x4C\x4D\x4D"
				"\x5E\x0A\x0E\x1B\x1F\x1E\x14\x51\x52\x52\x53\x53\x54\x54\x55\x55"
				"\x56\x56\x57\x57\x58\x58\x59\x59\x5A\x5A\x5B\x5B\x5C\x5C\x5D\x5D"
				"\x71\x5E\x5F\x5F\x6F\x60\x61\x61\x62\x62\x63\x63\x64\x64\x65\x65"
				"\x76\x22\x26\x33\x3C\x3F\x26\x69\x6A\x6A\x6B\x6B\x6C\x6C\x6D\x6D"
			);
		}

		virtual std::string rename()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x3e\x00\x00\x00" "\x02\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_ONE
				DATA_TWO
				"\x3E\x3E\x3F\x3F\x40\x40\x41\x41\x42\x42\x43\x43\x44\x44\x45\x45"
				"\x66\x46\x47\x47\x47\x48\x49\x49\x4A\x4A\x4B\x4B\x4C\x4C\x4D\x4D"
				"\x5E\x0A\x0E\x1B\x04\x18\x03\x14\x17\x52\x53\x53\x54\x54\x55\x55"
				"\x56\x56\x57\x57\x58\x58\x59\x59\x5A\x5A\x5B\x5B\x5C\x5C\x5D\x5D"
				"\x71\x5E\x5F\x5F\x6F\x60\x61\x61\x62\x62\x63\x63\x64\x64\x65\x65"
				"\x76\x22\x26\x33\x3C\x3F\x26\x69\x6A\x6A\x6B\x6B\x6C\x6C\x6D\x6D"
			);
		}

		virtual std::string insert_end()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x4f\x00\x00\x00" "\x03\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_ONE
				DATA_TWO
				DATA_THREE
				"\x4F\x4F\x50\x50\x51\x51\x52\x52\x53\x53\x54\x54\x55\x55\x56\x56"
				"\x77\x57\x58\x58\x56\x59\x5A\x5A\x5B\x5B\x5C\x5C\x5D\x5D\x5E\x5E"
				"\x4F\x1B\x21\x34\x2E\x2F\x27\x62\x63\x63\x64\x64\x65\x65\x66\x66"
				"\x67\x67\x68\x68\x69\x69\x6A\x6A\x6B\x6B\x6C\x6C\x6D\x6D\x6E\x6E"
				"\x40\x6F\x70\x70\x7E\x71\x72\x72\x73\x73\x74\x74\x75\x75\x76\x76"
				"\x67\x33\x39\x2C\x2D\x2E\x35\x7A\x7B\x7B\x7C\x7C\x7D\x7D\x7E\x7E"
				"\x7F\x7F\x80\x80\x81\x81\x82\x82\x83\x83\x84\x84\x85\x85\x86\x86"
				"\xB9\x87\x88\x88\x98\x89\x8A\x8A\x8B\x8B\x8C\x8C\x8D\x8D\x8E\x8E"
				"\x9F\xCB\xD1\xC4\xC5\xD9\xC0\xD7\xD6\x93\x94\x94\x95\x95\x96\x96"
			);
		}

		virtual std::string insert_mid()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x4f\x00\x00\x00" "\x03\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_ONE
				DATA_THREE
				DATA_TWO
				"\x4F\x4F\x50\x50\x51\x51\x52\x52\x53\x53\x54\x54\x55\x55\x56\x56"
				"\x77\x57\x58\x58\x56\x59\x5A\x5A\x5B\x5B\x5C\x5C\x5D\x5D\x5E\x5E"
				"\x4F\x1B\x21\x34\x2E\x2F\x27\x62\x63\x63\x64\x64\x65\x65\x66\x66"
				"\x67\x67\x68\x68\x69\x69\x6A\x6A\x6B\x6B\x6C\x6C\x6D\x6D\x6E\x6E"
				"\x40\x6F\x70\x70\x60\x71\x72\x72\x73\x73\x74\x74\x75\x75\x76\x76"
				"\x67\x33\x39\x2C\x2D\x31\x28\x3F\x3E\x7B\x7C\x7C\x7D\x7D\x7E\x7E"
				"\x7F\x7F\x80\x80\x81\x81\x82\x82\x83\x83\x84\x84\x85\x85\x86\x86"
				"\xC7\x87\x88\x88\x86\x89\x8A\x8A\x8B\x8B\x8C\x8C\x8D\x8D\x8E\x8E"
				"\x9F\xCB\xD1\xC4\xC5\xC6\xDD\x92\x93\x93\x94\x94\x95\x95\x96\x96"
			);
		}

		virtual std::string insert2()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x5f\x00\x00\x00" "\x04\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_ONE
				DATA_THREE
				DATA_FOUR
				DATA_TWO
				"\x5F\x5F\x60\x60\x61\x61\x62\x62\x63\x63\x64\x64\x65\x65\x66\x66"
				"\x47\x67\x68\x68\x66\x69\x6A\x6A\x6B\x6B\x6C\x6C\x6D\x6D\x6E\x6E"
				"\x7F\x2B\x31\x24\x3E\x3F\x37\x72\x73\x73\x74\x74\x75\x75\x76\x76"
				"\x77\x77\x78\x78\x79\x79\x7A\x7A\x7B\x7B\x7C\x7C\x7D\x7D\x7E\x7E"
				"\x50\x7F\x80\x80\x90\x81\x82\x82\x83\x83\x84\x84\x85\x85\x86\x86"
				"\x97\xC3\xC9\xDC\xDD\xC1\xD8\xCF\xCE\x8B\x8C\x8C\x8D\x8D\x8E\x8E"
				"\x8F\x8F\x90\x90\x91\x91\x92\x92\x93\x93\x94\x94\x95\x95\x96\x96"
				"\xD7\x97\x98\x98\x89\x99\x9A\x9A\x9B\x9B\x9C\x9C\x9D\x9D\x9E\x9E"
				"\x8F\xDB\xE1\xF4\xE7\xEE\xF7\xF0\xA3\xA3\xA4\xA4\xA5\xA5\xA6\xA6"
				"\xA7\xA7\xA8\xA8\xA9\xA9\xAA\xAA\xAB\xAB\xAC\xAC\xAD\xAD\xAE\xAE"
				"\xFF\xAF\xB0\xB0\xBE\xB1\xB2\xB2\xB3\xB3\xB4\xB4\xB5\xB5\xB6\xB6"
				"\xA7\xF3\xF9\xEC\xED\xEE\xF5\xBA\xBB\xBB\xBC\xBC\xBD\xBD\xBE\xBE"
			);
		}

		virtual std::string remove()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x2f\x00\x00\x00" "\x01\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_TWO
				"\x2F\x2F\x30\x30\x31\x31\x32\x32\x33\x33\x34\x34\x35\x35\x36\x36"
				"\x17\x37\x38\x38\x36\x39\x3A\x3A\x3B\x3B\x3C\x3C\x3D\x3D\x3E\x3E"
				"\x2F\x7B\x01\x14\x15\x16\x0D\x42\x43\x43\x44\x44\x45\x45\x46\x46"
			);
		}

		virtual std::string remove2()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x20\x00\x00\x00" "\x00\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
			);
		}

		virtual std::string insert_remove()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x40\x00\x00\x00" "\x02\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_THREE
				DATA_TWO
				"\x40\x40\x41\x41\x42\x42\x43\x43\x44\x44\x45\x45\x46\x46\x47\x47"
				"\x68\x48\x49\x49\x5B\x4A\x4B\x4B\x4C\x4C\x4D\x4D\x4E\x4E\x4F\x4F"
				"\x40\x14\x10\x05\x06\x1A\x01\x16\x11\x54\x55\x55\x56\x56\x57\x57"
				"\x58\x58\x59\x59\x5A\x5A\x5B\x5B\x5C\x5C\x5D\x5D\x5E\x5E\x5F\x5F"
				"\x51\x60\x61\x61\x6D\x62\x63\x63\x64\x64\x65\x65\x66\x66\x67\x67"
				"\x78\x2C\x28\x3D\x3E\x3D\x24\x6B\x6C\x6C\x6D\x6D\x6E\x6E\x6F\x6F"
			);
		}

		virtual std::string move()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x3e\x00\x00\x00" "\x02\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_TWO
				DATA_ONE
				"\x3E\x3E\x3F\x3F\x40\x40\x41\x41\x42\x42\x43\x43\x44\x44\x45\x45"
				"\x66\x46\x47\x47\x47\x48\x49\x49\x4A\x4A\x4B\x4B\x4C\x4C\x4D\x4D"
				"\x5E\x0A\x0E\x1B\x04\x07\x1E\x51\x52\x52\x53\x53\x54\x54\x55\x55"
				"\x56\x56\x57\x57\x58\x58\x59\x59\x5A\x5A\x5B\x5B\x5C\x5C\x5D\x5D"
				"\x71\x5E\x5F\x5F\x6F\x60\x61\x61\x62\x62\x63\x63\x64\x64\x65\x65"
				"\x76\x22\x26\x33\x27\x26\x2C\x69\x6A\x6A\x6B\x6B\x6C\x6C\x6D\x6D"
			);
		}

		virtual std::string resize_larger()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x43\x00\x00\x00" "\x02\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				DATA_ONE "\0\0\0\0\0"
				DATA_TWO
				"\x43\x43\x44\x44\x45\x45\x46\x46\x47\x47\x48\x48\x49\x49\x4A\x4A"
				"\x6B\x4B\x4C\x4C\x59\x4D\x4E\x4E\x4F\x4F\x50\x50\x51\x51\x52\x52"
				"\x43\x17\x15\x00\x1A\x1B\x13\x56\x57\x57\x58\x58\x59\x59\x5A\x5A"
				"\x5B\x5B\x5C\x5C\x5D\x5D\x5E\x5E\x5F\x5F\x60\x60\x61\x61\x62\x62"
				"\x57\x63\x64\x64\x6A\x65\x66\x66\x67\x67\x68\x68\x69\x69\x6A\x6A"
				"\x7B\x2F\x2D\x38\x39\x3A\x21\x6E\x6F\x6F\x70\x70\x71\x71\x72\x72"
			);
		}

		virtual std::string resize_smaller()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x39\x00\x00\x00" "\x02\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				"Thhr\"kp#kj"
				DATA_TWO
				"\x39\x39\x3A\x3A\x3B\x3B\x3C\x3C\x3D\x3D\x3E\x3E\x3F\x3F\x40\x40"
				"\x61\x41\x42\x42\x49\x43\x44\x44\x45\x45\x46\x46\x47\x47\x48\x48"
				"\x59\x0D\x0B\x1E\x04\x05\x09\x4C\x4D\x4D\x4E\x4E\x4F\x4F\x50\x50"
				"\x51\x51\x52\x52\x53\x53\x54\x54\x55\x55\x56\x56\x57\x57\x58\x58"
				"\x73\x59\x5A\x5A\x54\x5B\x5C\x5C\x5D\x5D\x5E\x5E\x5F\x5F\x60\x60"
				"\x71\x25\x23\x36\x37\x34\x2B\x64\x65\x65\x66\x66\x67\x67\x68\x68"
			);
		}

		virtual std::string resize_write()
		{
			return STRING_WITH_NULLS(
				"RFF\x1a" "\x01\x03\x00\x00" "\x46\x00\x00\x00" "\x02\x00\x00\x00"
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00"
				"Nov!pgpj~aa%ri'5;(jakxx"  /* "Now resized to 23 chars" */
				DATA_TWO
				"\x46\x46\x47\x47\x48\x48\x49\x49\x4A\x4A\x4B\x4B\x4C\x4C\x4D\x4D"
				"\x6E\x4E\x4F\x4F\x47\x50\x51\x51\x52\x52\x53\x53\x54\x54\x55\x55"
				"\x46\x12\x16\x03\x17\x16\x1C\x59\x5A\x5A\x5B\x5B\x5C\x5C\x5D\x5D"
				"\x5E\x5E\x5F\x5F\x60\x60\x61\x61\x62\x62\x63\x63\x64\x64\x65\x65"
				"\x51\x66\x67\x67\x67\x68\x69\x69\x6A\x6A\x6B\x6B\x6C\x6C\x6D\x6D"
				"\x7E\x2A\x2E\x3B\x24\x27\x3E\x71\x72\x72\x73\x73\x74\x74\x75\x75"
			);
		}
};

IMPLEMENT_TESTS(rff_blood_encrypted);
