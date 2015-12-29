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

#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Media.h"

class Artists : public Tests
{
};

TEST_F( Artists, Create )
{
    auto a = ml->createArtist( "Flying Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->name(), "Flying Otters" );

    Reload();

    auto a2 = ml->artist( "Flying Otters" );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->name(), "Flying Otters" );
}

TEST_F( Artists, CreateDefaults )
{
    // Ensure this won't fail due to duplicate insertions
    // We just reload, which will call the initialization routine again.
    // This is implicitely tested by all other tests, though it seems better
    // to have an explicit one. We might also just run the request twice from here
    // sometime in the future.
    Reload();
}

TEST_F( Artists, ShortBio )
{
    auto a = ml->createArtist( "Raging Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->shortBio(), "" );

    std::string bio("An otter based post-rock band");
    a->setShortBio( bio );
    ASSERT_EQ( a->shortBio(), bio );

    Reload();

    auto a2 = ml->artist( "Raging Otters" );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->shortBio(), bio );
}

TEST_F( Artists, ArtworkUrl )
{
    auto a = ml->createArtist( "Dream seaotter" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->artworkUrl(), "" );

    std::string artwork("/tmp/otter.png");
    a->setArtworkUrl( artwork );
    ASSERT_EQ( a->artworkUrl(), artwork );

    Reload();

    auto a2 = ml->artist( "Dream seaotter" );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->artworkUrl(), artwork );
}

TEST_F( Artists, Albums )
{
    auto artist = ml->createArtist( "Cannibal Otters" );
    auto album1 = ml->createAlbum( "album1" );
    auto album2 = ml->createAlbum( "album2" );

    ASSERT_NE( artist, nullptr );
    ASSERT_NE( album1, nullptr );
    ASSERT_NE( album2, nullptr );

    album1->setAlbumArtist( artist.get() );
    album2->setAlbumArtist( artist.get() );

    auto albums = artist->albums();
    ASSERT_EQ( albums.size(), 2u );

    Reload();

    auto artist2 = ml->artist( "Cannibal Otters" );
    auto albums2 = artist2->albums();
    ASSERT_EQ( albums.size(), 2u );
}

TEST_F( Artists, AllSongs )
{
    auto artist = ml->createArtist( "Cannibal Otters" );
    ASSERT_NE( artist, nullptr );

    for (auto i = 1; i <= 3; ++i)
    {
        auto f = ml->addFile( "song" + std::to_string(i) + ".mp3" );
        auto res = artist->addMedia( f.get() );
        ASSERT_TRUE( res );
    }

    auto songs = artist->media();
    ASSERT_EQ( songs.size(), 3u );

    Reload();

    auto artist2 = ml->artist( "Cannibal Otters" );
    songs = artist2->media();
    ASSERT_EQ( songs.size(), 3u );
}

TEST_F( Artists, GetAll )
{
    auto artists = ml->artists();
    // Ensure we don't include Unknown Artist // Various Artists
    ASSERT_EQ( artists.size(), 0u );

    for ( int i = 0; i < 5; i++ )
    {
        auto a = ml->createArtist( std::to_string( i ) );
        auto alb = ml->createAlbum( std::to_string( i ) );
        ASSERT_NE( nullptr, alb );
        alb->setAlbumArtist( a.get() );
        ASSERT_NE( a, nullptr );
    }
    artists = ml->artists();
    ASSERT_EQ( artists.size(), 5u );

    Reload();

    auto artists2 = ml->artists();
    ASSERT_EQ( artists2.size(), 5u );
}

TEST_F( Artists, UnknownAlbum )
{
    auto a = ml->createArtist( "Explotters in the sky" );
    auto album = a->unknownAlbum();
    auto album2 = a->unknownAlbum();

    ASSERT_NE( nullptr, album );
    ASSERT_NE( nullptr, album2 );
    ASSERT_EQ( album->id(), album2->id() );

    Reload();

    a = std::static_pointer_cast<Artist>( ml->artist( a->name() ) );
    album2 = a->unknownAlbum();
    ASSERT_NE( nullptr, album2 );
    ASSERT_EQ( album2->id(), album->id() );
}
