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

#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Media.h"

#include "database/SqliteTools.h"

const std::string policy::ArtistTable::Name = "artist";
const std::string policy::ArtistTable::PrimaryKeyColumn = "id_artist";
unsigned int Artist::*const policy::ArtistTable::PrimaryKey = &Artist::m_id;


Artist::Artist( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_name
        >> m_shortBio
        >> m_artworkUrl
        >> m_nbAlbums
        >> m_isPresent;
}

Artist::Artist( const std::string& name )
    : m_id( 0 )
    , m_name( name )
    , m_nbAlbums( 0 )
    , m_isPresent( true )
{
}

unsigned int Artist::id() const
{
    return m_id;
}

const std::string &Artist::name() const
{
    return m_name;
}

const std::string &Artist::shortBio() const
{
    return m_shortBio;
}

bool Artist::setShortBio(const std::string &shortBio)
{
    static const std::string req = "UPDATE " + policy::ArtistTable::Name
            + " SET shortbio = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, shortBio, m_id ) == false )
        return false;
    m_shortBio = shortBio;
    return true;
}

std::vector<AlbumPtr> Artist::albums() const
{
    if ( m_id == 0 )
        return {};
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name + " alb "
            "WHERE artist_id = ? ORDER BY release_year, title";
    return Album::fetchAll<IAlbum>( m_dbConnection, req, m_id );
}

std::vector<MediaPtr> Artist::media() const
{
    if ( m_id )
    {
        static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
                "LEFT JOIN MediaArtistRelation mar ON mar.id_media = med.id_media "
                "WHERE mar.id_artist = ? AND med.is_present = 1";
        return Media::fetchAll<IMedia>( m_dbConnection, req, m_id );
    }
    else
    {
        // Not being able to rely on ForeignKey here makes me a saaaaad panda...
        // But sqlite only accepts "IS NULL" to compare against NULL...
        static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
                "LEFT JOIN MediaArtistRelation mar ON mar.id_media = med.id_media "
                "WHERE mar.id_artist IS NULL";
        return Media::fetchAll<IMedia>( m_dbConnection, req );
    }
}

bool Artist::addMedia(Media* media)
{
    static const std::string req = "INSERT INTO MediaArtistRelation VALUES(?, ?)";
    // If track's ID is 0, the request will fail due to table constraints
    sqlite::ForeignKey artistForeignKey( m_id );
    return sqlite::Tools::insert( m_dbConnection, req, media->id(), artistForeignKey ) != 0;
}

const std::string& Artist::artworkUrl() const
{
    return m_artworkUrl;
}

bool Artist::setArtworkUrl( const std::string& artworkUrl )
{
    if ( m_artworkUrl == artworkUrl )
        return true;
    static const std::string req = "UPDATE " + policy::ArtistTable::Name +
            " SET artwork_url = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artworkUrl, m_id ) == false )
        return false;
    m_artworkUrl = artworkUrl;
    return true;
}

bool Artist::updateNbAlbum( int increment )
{
    assert( increment != 0 );
    assert( increment > 0 || ( increment < 0 && m_nbAlbums >= 1 ) );

    static const std::string req = "UPDATE " + policy::ArtistTable::Name +
            " SET nb_albums = nb_albums + ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, increment, m_id ) == false )
        return false;
    m_nbAlbums += increment;
    return true;
}

std::shared_ptr<Album> Artist::unknownAlbum()
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
                        " WHERE artist_id = ? AND title IS NULL";
    auto album = Album::fetch( m_dbConnection, req, m_id );
    if ( album == nullptr )
    {
        album = Album::createUnknownAlbum( m_dbConnection, this );
        if ( album == nullptr )
            return nullptr;
        if ( updateNbAlbum( 1 ) == false )
        {
            Album::destroy( m_dbConnection, album->id() );
            return nullptr;
        }
    }
    return album;
}

bool Artist::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::ArtistTable::Name +
            "("
                "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
                "shortbio TEXT,"
                "artwork_url TEXT,"
                "nb_albums UNSIGNED INT DEFAULT 0,"
                "is_present BOOLEAN NOT NULL DEFAULT 1"
            ")";
    static const std::string reqRel = "CREATE TABLE IF NOT EXISTS MediaArtistRelation("
                "id_media INTEGER NOT NULL,"
                "id_artist INTEGER,"
                "PRIMARY KEY (id_media, id_artist),"
                "FOREIGN KEY(id_media) REFERENCES " + policy::MediaTable::Name +
                "(id_media) ON DELETE CASCADE,"
                "FOREIGN KEY(id_artist) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, reqRel );
}

bool Artist::createTriggers(DBConnection dbConnection)
{
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS has_album_present AFTER UPDATE OF "
            "is_present ON " + policy::AlbumTable::Name +
            " BEGIN "
            " UPDATE " + policy::ArtistTable::Name + " SET is_present="
                "(SELECT COUNT(id_album) FROM " + policy::AlbumTable::Name + " WHERE artist_id=new.artist_id AND is_present=1) "
                "WHERE id_artist=new.artist_id;"
            " END";
    return sqlite::Tools::executeRequest( dbConnection, triggerReq );
}

bool Artist::createDefaultArtists( DBConnection dbConnection )
{
    // Don't rely on Artist::create, since we want to insert or do nothing here.
    // This will skip the cache for those new entities, but they will be inserted soon enough anyway.
    static const std::string req = "INSERT OR IGNORE INTO " + policy::ArtistTable::Name +
            "(id_artist) VALUES(?),(?)";
    sqlite::Tools::insert( dbConnection, req, medialibrary::UnknownArtistID,
                                          medialibrary::VariousArtistID );
    // Always return true. The insertion might succeed, but we consider it a failure when 0 row
    // gets inserted, while we are explicitely specifying "OR IGNORE" here.
    return true;
}

std::shared_ptr<Artist> Artist::create( DBConnection dbConnection, const std::string &name )
{
    auto artist = std::make_shared<Artist>( name );
    static const std::string req = "INSERT INTO " + policy::ArtistTable::Name +
            "(id_artist, name) VALUES(NULL, ?)";
    if ( insert( dbConnection, artist, req, name ) == false )
        return nullptr;
    artist->m_dbConnection = dbConnection;
    return artist;
}

