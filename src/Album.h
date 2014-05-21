#ifndef ALBUM_H
#define ALBUM_H

#include <memory>
#include <sqlite3.h>

#include "IMediaLibrary.h"

#include "Cache.h"
#include "IAlbum.h"

namespace policy
{
struct AlbumTable
{
    static const std::string Name;
    static const std::string CacheColumn;
};
}

class Album : public IAlbum, public Cache<Album, IAlbum, policy::AlbumTable>
{
    private:
        typedef Cache<Album, IAlbum, policy::AlbumTable> _Cache;
    public:
        Album( sqlite3* dbConnection, sqlite3_stmt* stmt );
        Album( const std::string& id3tag );

        unsigned int id() const;
        virtual const std::string& name();
        virtual unsigned int releaseYear();
        virtual const std::string& shortSummary();
        virtual const std::string& artworkUrl();
        virtual time_t lastSyncDate();
        virtual const std::vector<std::shared_ptr<IAlbumTrack>>& tracks();

        static bool createTable( sqlite3* dbConnection );
        static AlbumPtr create( sqlite3* dbConnection, const std::string& id3Tag );

    protected:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        unsigned int m_releaseYear;
        std::string m_shortSummary;
        std::string m_artworkUrl;
        time_t m_lastSyncDate;
        std::string m_id3tag;

        std::vector<std::shared_ptr<IAlbumTrack>>* m_tracks;

        friend class Cache<Album, IAlbum, policy::AlbumTable>;
};

#endif // ALBUM_H
