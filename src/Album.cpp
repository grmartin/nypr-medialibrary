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

#include <algorithm>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "IGenre.h"
#include "Media.h"

#include "database/SqliteTools.h"

const std::string policy::AlbumTable::Name = "Album";
const std::string policy::AlbumTable::PrimaryKeyColumn = "id_album";
unsigned int Album::* const policy::AlbumTable::PrimaryKey = &Album::m_id;

Album::Album(DBConnection dbConnection, sqlite::Row& row)
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_title
        >> m_artistId
        >> m_releaseYear
        >> m_shortSummary
        >> m_artworkMrl
        >> m_nbTracks
        >> m_isPresent;
}

Album::Album(const std::string& title )
    : m_id( 0 )
    , m_title( title )
    , m_artistId( 0 )
    , m_releaseYear( ~0u )
    , m_nbTracks( 0 )
    , m_isPresent( true )
{
}

Album::Album( const Artist* artist )
    : m_id( 0 )
    , m_artistId( artist->id() )
    , m_releaseYear( ~0u )
    , m_nbTracks( 0 )
    , m_isPresent( true )
{
}

unsigned int Album::id() const
{
    return m_id;
}

const std::string& Album::title() const
{
    return m_title;
}

unsigned int Album::releaseYear() const
{
    if ( m_releaseYear == ~0U )
        return 0;
    return m_releaseYear;
}

bool Album::setReleaseYear( unsigned int date, bool force )
{
    if ( date == m_releaseYear )
        return true;
    if ( force == false )
    {
        if ( m_releaseYear != ~0u && date != m_releaseYear )
        {
            // If we already have set the date back to 0, don't do it again.
            if ( m_releaseYear == 0 )
                return true;
            date = 0;
        }
    }
    static const std::string req = "UPDATE " + policy::AlbumTable::Name
            + " SET release_year = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, date, m_id ) == false )
        return false;
    m_releaseYear = date;
    return true;
}

const std::string& Album::shortSummary() const
{
    return m_shortSummary;
}

bool Album::setShortSummary( const std::string& summary )
{
    static const std::string req = "UPDATE " + policy::AlbumTable::Name
            + " SET short_summary = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

const std::string& Album::artworkMrl() const
{
    return m_artworkMrl;
}

bool Album::setArtworkMrl( const std::string& artworkMrl )
{
    static const std::string req = "UPDATE " + policy::AlbumTable::Name
            + " SET artwork_mrl = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artworkMrl, m_id ) == false )
        return false;
    m_artworkMrl = artworkMrl;
    return true;
}

std::vector<MediaPtr> Album::tracks() const
{
    // This doesn't return the cached version, because it would be fairly complicated, if not impossible or
    // counter productive, to maintain a cache that respects all orderings.
    static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
            " INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
            " WHERE att.album_id = ? AND med.is_present = 1 ORDER BY att.disc_number, att.track_number";
    return Media::fetchAll<IMedia>( m_dbConnection, req, m_id );
}

std::vector<std::shared_ptr<IMedia> > Album::tracks( GenrePtr genre ) const
{
    if ( genre == nullptr )
        return {};
    static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
            " INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
            " WHERE att.album_id = ? AND med.is_present = 1"
            " AND genre_id = ?"
            " ORDER BY att.disc_number, att.track_number";
    return Media::fetchAll<IMedia>( m_dbConnection, req, m_id, genre->id() );
}

std::vector<MediaPtr> Album::cachedTracks() const
{
    auto lock = m_tracks.lock();
    if ( m_tracks.isCached() == false )
        m_tracks = tracks();
    return m_tracks.get();
}

std::shared_ptr<AlbumTrack> Album::addTrack( std::shared_ptr<Media> media, unsigned int trackNb, unsigned int discNumber )
{
    auto t = m_dbConnection->newTransaction();

    auto track = AlbumTrack::create( m_dbConnection, m_id, *media, trackNb, discNumber );
    if ( track == nullptr )
        return nullptr;
    media->setAlbumTrack( track );
    // Assume the media will be saved by the caller
    static const std::string req = "UPDATE " + policy::AlbumTable::Name +
            " SET nb_tracks = nb_tracks + 1 WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, m_id ) == false )
        return nullptr;
    m_nbTracks++;
    t->commit();
    auto lock = m_tracks.lock();
    // Don't assume we have always have a valid value in m_tracks.
    // While it's ok to assume that if we are currently parsing the album, we
    // have a valid cache tracks, this isn't true when restarting an interrupted parsing.
    // The nbTracks value will be correct however. If it's equal to one, it means we're inserting
    // the first track in this album
    if ( m_tracks.isCached() == false && m_nbTracks == 1 )
        m_tracks.markCached();
    if ( m_tracks.isCached() == true )
        m_tracks.get().push_back( media );
    return track;
}

unsigned int Album::nbTracks() const
{
    return m_nbTracks;
}

ArtistPtr Album::albumArtist() const
{
    if ( m_artistId == 0 )
        return nullptr;
    return Artist::fetch( m_dbConnection, m_artistId );
}

bool Album::setAlbumArtist( Artist* artist )
{
    if ( m_artistId == artist->id() )
        return true;
    if ( artist->id() == 0 )
        return false;
    static const std::string req = "UPDATE " + policy::AlbumTable::Name + " SET "
            "artist_id = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artist->id(), m_id ) == false )
        return false;
    if ( m_artistId != 0 )
    {
        auto previousArtist = Artist::fetch( m_dbConnection, m_artistId );
        previousArtist->updateNbAlbum( -1 );
    }
    m_artistId = artist->id();
    artist->updateNbAlbum( 1 );
    static const std::string ftsReq = "UPDATE " + policy::AlbumTable::Name + "Fts SET "
            " artist = ? WHERE rowid = ?";
    sqlite::Tools::executeUpdate( m_dbConnection, ftsReq, artist->name(), m_id );
    return true;
}

std::vector<ArtistPtr> Album::artists() const
{
    static const std::string req = "SELECT art.* FROM " + policy::ArtistTable::Name + " art "
            "INNER JOIN AlbumArtistRelation aar ON aar.artist_id = art.id_artist "
            "WHERE aar.album_id = ?";
    return Artist::fetchAll<IArtist>( m_dbConnection, req, m_id );
}

bool Album::addArtist( std::shared_ptr<Artist> artist )
{
    static const std::string req = "INSERT OR IGNORE INTO AlbumArtistRelation VALUES(?, ?)";
    if ( m_id == 0 || artist->id() == 0 )
    {
        LOG_ERROR("Both artist & album need to be inserted in database before being linked together" );
        return false;
    }
    return sqlite::Tools::insert( m_dbConnection, req, m_id, artist->id() ) != 0;
}

bool Album::removeArtist(Artist* artist)
{
    static const std::string req = "DELETE FROM AlbumArtistRelation WHERE album_id = ? "
            "AND id_artist = ?";
    return sqlite::Tools::executeDelete( m_dbConnection, req, m_id, artist->id() );
}

bool Album::createTable(DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::AlbumTable::Name +
            "("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT COLLATE NOCASE,"
                "artist_id UNSIGNED INTEGER,"
                "release_year UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "artwork_mrl TEXT,"
                "nb_tracks UNSIGNED INTEGER DEFAULT 0,"
                "is_present BOOLEAN NOT NULL DEFAULT 1,"
                "FOREIGN KEY( artist_id ) REFERENCES " + policy::ArtistTable::Name
                + "(id_artist) ON DELETE CASCADE"
            ")";
    static const std::string reqRel = "CREATE TABLE IF NOT EXISTS AlbumArtistRelation("
                "album_id INTEGER,"
                "artist_id INTEGER,"
                "PRIMARY KEY (album_id, artist_id),"
                "FOREIGN KEY(album_id) REFERENCES " + policy::AlbumTable::Name + "("
                    + policy::AlbumTable::PrimaryKeyColumn + ") ON DELETE CASCADE,"
                "FOREIGN KEY(artist_id) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
            ")";
    static const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::AlbumTable::Name + "Fts USING FTS3("
                "title,"
                "artist"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, reqRel ) &&
            sqlite::Tools::executeRequest( dbConnection, vtableReq );
}

bool Album::createTriggers(DBConnection dbConnection)
{
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_album_present AFTER UPDATE OF "
            "is_present ON " + policy::AlbumTrackTable::Name +
            " BEGIN "
            " UPDATE " + policy::AlbumTable::Name + " SET is_present="
                "(SELECT COUNT(id_track) FROM " + policy::AlbumTrackTable::Name + " WHERE album_id=new.album_id AND is_present=1) "
                "WHERE id_album=new.album_id;"
            " END";
    static const std::string vtriggerInsert = "CREATE TRIGGER IF NOT EXISTS insert_album_fts AFTER INSERT ON "
            + policy::AlbumTable::Name +
            // Skip unknown albums
            " WHEN new.title IS NOT NULL"
            " BEGIN"
            " INSERT INTO " + policy::AlbumTable::Name + "Fts(rowid, title) VALUES(new.id_album, new.title);"
            " END";
    static const std::string vtriggerDelete = "CREATE TRIGGER IF NOT EXISTS delete_album_fts BEFORE DELETE ON "
            + policy::AlbumTable::Name +
            // Unknown album probably won't be deleted, but better safe than sorry
            " WHEN old.title IS NOT NULL"
            " BEGIN"
            " DELETE FROM " + policy::AlbumTable::Name + "Fts WHERE rowid = old.id_album;"
            " END";
    return sqlite::Tools::executeRequest( dbConnection, triggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, vtriggerInsert ) &&
            sqlite::Tools::executeRequest( dbConnection, vtriggerDelete );
}

std::shared_ptr<Album> Album::create(DBConnection dbConnection, const std::string& title )
{
    auto album = std::make_shared<Album>( title );
    static const std::string req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, title) VALUES(NULL, ?)";
    if ( insert( dbConnection, album, req, title ) == false )
        return nullptr;
    album->m_dbConnection = dbConnection;
    return album;
}

std::shared_ptr<Album> Album::createUnknownAlbum( DBConnection dbConnection, const Artist* artist )
{
    auto album = std::make_shared<Album>( artist );
    static const std::string req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, artist_id) VALUES(NULL, ?)";
    if ( insert( dbConnection, album, req, artist->id() ) == false )
        return nullptr;
    album->m_dbConnection = dbConnection;
    return album;
}

std::vector<AlbumPtr> Album::search( DBConnection dbConn, const std::string& pattern )
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name + " WHERE id_album IN "
            "(SELECT rowid FROM " + policy::AlbumTable::Name + "Fts WHERE " +
            policy::AlbumTable::Name + "Fts MATCH ?)";
    return fetchAll<IAlbum>( dbConn, req, pattern + "*" );
}
