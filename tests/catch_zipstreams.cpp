/*
  Zipios -- a small C++ library that provides easy access to .zip files.

  Copyright (C) 2000-2007  Thomas Sondergaard
  Copyright (c) 2015-2022  Made to Order Software Corp.  All Rights Reserved

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/** \file
 *
 * Zipios unit tests for the public ZipInputStream and ZipOutputStream
 * classes.
 */

#include "catch_main.hpp"

#include <zipios/directoryentry.hpp>
#include <zipios/zipfile.hpp>
#include <zipios/zipinputstream.hpp>
#include <zipios/zipoutputstream.hpp>

#include <fstream>
#include <iterator>


namespace
{


zipios::FileEntry::pointer_t create_entry(std::string const & name, zipios::StorageMethod method)
{
    zipios::FileEntry::pointer_t entry(std::make_shared<zipios::DirectoryEntry>(zipios::FilePath(name)));
    entry->setMethod(method);
    entry->setLevel(method == zipios::StorageMethod::STORED
                        ? zipios::FileEntry::COMPRESSION_LEVEL_NONE
                        : zipios::FileEntry::COMPRESSION_LEVEL_DEFAULT);
    entry->setUnixTime(1700000000);
    return entry;
}


std::string read_all(std::istream & in)
{
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}


// write the same three entries, with or without flushing the stream
void write_archive(std::ostream & out, bool flush)
{
    zipios::ZipOutputStream zip(out);

    zip.putNextEntry(create_entry("text.txt", zipios::StorageMethod::DEFLATED));
    for(int i(0); i < 1000; ++i)
    {
        zip << "line " << i;
        if(flush)
        {
            zip << std::endl;
        }
        else
        {
            zip << '\n';
        }
    }
    CATCH_REQUIRE(zip.good());

    zip.putNextEntry(create_entry("empty.txt", zipios::StorageMethod::DEFLATED));

    zip.putNextEntry(create_entry("stored.txt", zipios::StorageMethod::STORED));
    zip << "stored data";
    if(flush)
    {
        zip.flush();
    }
    CATCH_REQUIRE(zip.good());

    zip.close();
}


std::string expected_text()
{
    std::string text;
    for(int i(0); i < 1000; ++i)
    {
        text += "line " + std::to_string(i) + "\n";
    }
    return text;
}


} // no name namespace


CATCH_SCENARIO("ZipOutputStream and ZipInputStream", "[ZipOutputStream][ZipInputStream]")
{
    zipios_test::safe_chdir cwd(SNAP_CATCH2_NAMESPACE::g_tmp_dir());

    CATCH_GIVEN("an archive written with a ZipOutputStream that gets flushed")
    {
        zipios_test::auto_unlink_t remove_zip("streams.zip", true);
        {
            std::ofstream out("streams.zip", std::ios::out | std::ios::binary);
            write_archive(out, true);
        }

        CATCH_THEN("all the entries can be read back")
        {
            zipios::ZipFile zf("streams.zip");
            CATCH_REQUIRE(zf.size() == 3);

            zipios::FileEntry::pointer_t text(zf.getEntry("text.txt"));
            CATCH_REQUIRE(text != nullptr);
            CATCH_REQUIRE(text->getMethod() == zipios::StorageMethod::DEFLATED);
            CATCH_REQUIRE(text->getSize() == expected_text().length());
            CATCH_REQUIRE(read_all(*zf.getInputStream("text.txt")) == expected_text());

            zipios::FileEntry::pointer_t stored(zf.getEntry("stored.txt"));
            CATCH_REQUIRE(stored != nullptr);
            CATCH_REQUIRE(stored->getMethod() == zipios::StorageMethod::STORED);
            CATCH_REQUIRE(read_all(*zf.getInputStream("stored.txt")) == "stored data");
        }

        CATCH_THEN("the empty entry is stored, not deflated")
        {
            zipios::ZipFile zf("streams.zip");
            zipios::FileEntry::pointer_t empty(zf.getEntry("empty.txt"));
            CATCH_REQUIRE(empty != nullptr);
            CATCH_REQUIRE(empty->getMethod() == zipios::StorageMethod::STORED);
            CATCH_REQUIRE(empty->getSize() == 0);
            CATCH_REQUIRE(empty->getCompressedSize() == 0);
            CATCH_REQUIRE(read_all(*zf.getInputStream("empty.txt")).empty());
        }

        CATCH_THEN("a ZipInputStream reads the first entry from a stream")
        {
            std::ifstream in("streams.zip", std::ios::in | std::ios::binary);
            zipios::ZipInputStream zis(in);
            CATCH_REQUIRE(read_all(zis) == expected_text());
        }
    }

    CATCH_GIVEN("the same archive written with and without flushing")
    {
        std::ostringstream flushed(std::ios::out | std::ios::binary);
        write_archive(flushed, true);
        std::ostringstream not_flushed(std::ios::out | std::ios::binary);
        write_archive(not_flushed, false);

        CATCH_THEN("the flushes do not change the archive")
        {
            CATCH_REQUIRE(flushed.str() == not_flushed.str());
        }
    }
}



// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
