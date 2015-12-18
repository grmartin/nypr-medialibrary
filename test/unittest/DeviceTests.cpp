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

#include "Tests.h"

#include "Album.h"
#include "Device.h"
#include "Media.h"
#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

class DeviceEntity : public Tests
{
};

class DeviceFs : public Tests
{
protected:
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;

protected:
    virtual void SetUp() override
    {
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        Reload();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithoutParser );
    }

    virtual void Reload()
    {
        Tests::Reload( fsMock, cbMock.get() );
    }
};

// Database/Entity tests

TEST_F( DeviceEntity, Create )
{
    auto d = ml->addDevice( "dummy", true );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    ASSERT_TRUE( d->isPresent() );

    Reload();

    d = ml->device( "dummy" );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    ASSERT_TRUE( d->isPresent() );
}

TEST_F( DeviceEntity, SetPresent )
{
    auto d = ml->addDevice( "dummy", true );
    ASSERT_NE( nullptr, d );
    ASSERT_TRUE( d->isPresent() );

    d->setPresent( false );
    ASSERT_FALSE( d->isPresent() );

    Reload();

    d = ml->device( "dummy" );
    ASSERT_FALSE( d->isPresent() );
}

// Filesystem tests:

TEST_F( DeviceFs, RemoveDisk )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_NE( nullptr, file );

    auto subdir = fsMock->directory( mock::FileSystemFactory::SubFolder );
    subdir->setDevice( nullptr );
    fsMock->removableDevice = nullptr;

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 2u, files.size() );

    file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_EQ( nullptr, file );
}

TEST_F( DeviceFs, UnmountDisk )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_NE( nullptr, file );

    auto subdir = fsMock->directory( mock::FileSystemFactory::SubFolder );
    auto device = std::static_pointer_cast<mock::Device>( subdir->device() );
    device->setPresent( false );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 2u, files.size() );

    file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_EQ( nullptr, file );
}

TEST_F( DeviceFs, ReplugDisk )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_NE( nullptr, file );

    auto subdir = fsMock->directory( mock::FileSystemFactory::SubFolder );
    auto device = std::static_pointer_cast<mock::Device>( subdir->device() );
    subdir->setDevice( nullptr );
    fsMock->removableDevice = nullptr;

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 2u, files.size() );

    file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_EQ( nullptr, file );

    subdir->setDevice( device );
    fsMock->removableDevice = device;
    cbMock->prepareForReload();
    Reload();
    reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_NE( nullptr, file );
}

TEST_F( DeviceFs, ReplugDiskWithExtraFiles )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    auto subdir = fsMock->directory( mock::FileSystemFactory::SubFolder );
    auto device = std::static_pointer_cast<mock::Device>( subdir->device() );
    subdir->setDevice( nullptr );
    fsMock->removableDevice = nullptr;

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    subdir->setDevice( device );
    fsMock->removableDevice = device;
    fsMock->addFile( mock::FileSystemFactory::SubFolder, "newfile.mkv" );

    cbMock->prepareForReload();
    Reload();
    reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 4u, files.size() );
}

TEST_F( DeviceFs, RemoveAlbum )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    // Create an album on a non-removable device
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album" ) );
        auto file = ml->file( std::string( mock::FileSystemFactory::Root ) + "audio.mp3" );
        album->addTrack( std::static_pointer_cast<Media>( file ), 1, 1 );
        auto artist = ml->createArtist( "artist" );
        album->setAlbumArtist( artist.get() );
    }
    // And an album that will disappear, along with its artist
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album 2" ) );
        auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
        album->addTrack( std::static_pointer_cast<Media>( file ), 1, 1 );
        auto artist = ml->createArtist( "artist 2" );
        album->setAlbumArtist( artist.get() );
    }

    auto albums = ml->albums();
    ASSERT_EQ( 2u, albums.size() );
    auto artists = ml->artists();
    ASSERT_EQ( 2u, artists.size() );

    auto subdir = fsMock->directory( mock::FileSystemFactory::SubFolder );
    subdir->setDevice( nullptr );
    fsMock->removableDevice = nullptr;

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    albums = ml->albums();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists();
    ASSERT_EQ( 1u, artists.size() );
}
