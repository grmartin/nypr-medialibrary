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

#ifndef SHOW_H
#define SHOW_H

#include <sqlite3.h>

#include "database/Cache.h"
#include "IMediaLibrary.h"
#include "IShow.h"

class Show;
class ShowEpisode;

namespace policy
{
struct ShowTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Show::*const PrimaryKey;
};
}

class Show : public IShow, public Cache<Show, IShow, policy::ShowTable>
{
    public:
        Show( DBConnection dbConnection, sqlite3_stmt* stmt );
        Show( const std::string& name );

        virtual unsigned int id() const;
        virtual const std::string& name() const;
        virtual time_t releaseDate() const;
        virtual bool setReleaseDate( time_t date );
        virtual const std::string& shortSummary() const;
        virtual bool setShortSummary( const std::string& summary );
        virtual const std::string& artworkUrl() const;
        virtual bool setArtworkUrl( const std::string& artworkUrl );
        virtual time_t lastSyncDate() const;
        virtual const std::string& tvdbId();
        virtual bool setTvdbId( const std::string& summary );
        virtual std::shared_ptr<ShowEpisode> addEpisode( const std::string& title, unsigned int episodeNumber );
        virtual std::vector<ShowEpisodePtr> episodes();
        virtual bool destroy();

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<Show> create( DBConnection dbConnection, const std::string& name );

    protected:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        time_t m_releaseDate;
        std::string m_shortSummary;
        std::string m_artworkUrl;
        time_t m_lastSyncDate;
        std::string m_tvdbId;

        friend class Cache<Show, IShow, policy::ShowTable>;
        friend struct policy::ShowTable;
        typedef Cache<Show, IShow, policy::ShowTable> _Cache;
};

#endif // SHOW_H
