/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "gtest/gtest.h"

#include "utils/Filename.h"

using namespace medialibrary;

TEST( FsUtils, extension )
{
    ASSERT_EQ( "ext", utils::file::extension( "file.ext" ) );
    ASSERT_EQ( "", utils::file::extension( "file." ) );
    ASSERT_EQ( "ext2", utils::file::extension( "file.ext.ext2" ) );
    ASSERT_EQ( "", utils::file::extension( "" ) );
    ASSERT_EQ( "", utils::file::extension( "file.ext." ) );
}

TEST( FsUtils, directory )
{
    ASSERT_EQ( "/a/b/c/", utils::file::directory( "/a/b/c/d.e" ) );
    ASSERT_EQ( "", utils::file::directory( "" ) );
    ASSERT_EQ( "", utils::file::directory( "file.test" ) );
}

TEST( FsUtils, fileName )
{
    ASSERT_EQ( "d.e", utils::file::fileName( "/a/b/c/d.e" ) );
    ASSERT_EQ( "noextfile", utils::file::fileName( "/a/b/noextfile" ) );
    ASSERT_EQ( "file.test", utils::file::fileName( "file.test" ) );
}

TEST( FsUtils, firstFolder )
{
    ASSERT_EQ( "f00", utils::file::firstFolder( "f00/bar/" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "/f00/bar" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "////f00/bar" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "/f00/" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "f00/" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "/f00" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "/" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "/foo.bar" ) );
}

TEST( FsUtils, removePath )
{
    ASSERT_EQ( "bar/", utils::file::removePath( "f00/bar/", "f00" ) );
    ASSERT_EQ( "bar/", utils::file::removePath( "/f00/bar/", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "f00/bar", "f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "/f00/bar", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "////f00/bar", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "/f00///bar", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "/f00///bar", "/f00/" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "bar", "" ) );
    ASSERT_EQ( "", utils::file::removePath( "bar/", "bar" ) );
    ASSERT_EQ( "", utils::file::removePath( "/f00/", "/f00/" ) );
}

TEST( FsUtils, parentFolder )
{
    ASSERT_EQ( "/a/b/", utils::file::parentDirectory( "/a/b/c/" ) );
    ASSERT_EQ( "/a/b/", utils::file::parentDirectory( "/a/b/c" ) );
    ASSERT_EQ( "", utils::file::parentDirectory( "" ) );
#ifdef _WIN32
    ASSERT_EQ( "C:\\a/b/", utils::file::parentDirectory( "C:\\a/b/c" ) );
    ASSERT_EQ( "C:/a/b/", utils::file::parentDirectory( "C:/a/b/c\\" ) );
    ASSERT_EQ( "C:\\a\\b\\", utils::file::parentDirectory( "C:\\a\\b\\c\\" ) );
    ASSERT_EQ( "C:\\a\\b\\", utils::file::parentDirectory( "C:\\a\\b\\c" ) );
#endif
}

TEST( FsUtils, toLocalPath )
{
    ASSERT_EQ( "/a/b/c/movie.avi", utils::file::toLocalPath( "file:///a/b/c/movie.avi" ) );
    ASSERT_EQ( "/yea /sp ace", utils::file::toLocalPath( "file:///yea%20/sp%20ace" ) );
    ASSERT_EQ( "/tést/ßóíú/file", utils::file::toLocalPath( "file:///t%C3%A9st/%C3%9F%C3%B3%C3%AD%C3%BA/file" ) );
    ASSERT_EQ( "/&/#/~", utils::file::toLocalPath( "file:///%26/%23/%7E" ) );
}
