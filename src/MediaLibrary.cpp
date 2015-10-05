#include <algorithm>
#include <functional>
#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "File.h"
#include "Folder.h"
#include "MediaLibrary.h"
#include "IMetadataService.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "Parser.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "Utils.h"
#include "VideoTrack.h"

// Discoverers:
#include "discoverer/FsDiscoverer.h"

// Metadata services:
#include "metadata_services/vlc/VLCMetadataService.h"
#include "metadata_services/vlc/VLCThumbnailer.h"

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "factory/FileSystem.h"

const std::vector<std::string> MediaLibrary::supportedVideoExtensions {
    // Videos
    "avi", "3gp", "amv", "asf", "divx", "dv", "flv", "gxf",
    "iso", "m1v", "m2v", "m2t", "m2ts", "m4v", "mkv", "mov",
    "mp2", "mp4", "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg",
    "mts", "mxf", "nsv", "nuv", "ogg", "ogm", "ogv", "ogx", "ps",
    "rec", "rm", "rmvb", "tod", "ts", "vob", "vro", "webm", "wmv"
};

const std::vector<std::string> MediaLibrary::supportedAudioExtensions {
    // Audio
    "a52", "aac", "ac3", "aiff", "amr", "aob", "ape",
    "dts", "flac", "it", "m4a", "m4p", "mid", "mka", "mlp",
    "mod", "mp1", "mp2", "mp3", "mpc", "oga", "ogg", "oma",
    "rmi", "s3m", "spx", "tta", "voc", "vqf", "w64", "wav",
    "wma", "wv", "xa", "xm"
};

MediaLibrary::MediaLibrary()
    : m_parser( new Parser )
{
}

MediaLibrary::~MediaLibrary()
{
    // The log callback isn't shared by all VLC::Instance's, yet since
    // they all share a single libvlc_instance_t, any VLC::Instance still alive
    // with a log callback set will try to invoke it.
    // We manually call logUnset to ensure that the callback that is about to be deleted will
    // not be called anymore.
    if ( m_vlcInstance.isValid() )
        m_vlcInstance.logUnset();
    File::clear();
    Folder::clear();
    Label::clear();
    Album::clear();
    AlbumTrack::clear();
    Show::clear();
    ShowEpisode::clear();
    Movie::clear();
    VideoTrack::clear();
    AudioTrack::clear();
    Artist::clear();
}

void MediaLibrary::setFsFactory(std::shared_ptr<factory::IFileSystem> fsFactory)
{
    m_fsFactory = fsFactory;
}

bool MediaLibrary::initialize( const std::string& dbPath, const std::string& snapshotPath, IMediaLibraryCb* mlCallback )
{
    if ( m_fsFactory == nullptr )
        m_fsFactory.reset( new factory::FileSystemDefaultFactory );
    m_snapshotPath = snapshotPath;
    m_callback = mlCallback;

    if ( mlCallback != nullptr )
    {
        const char* args[] = {
            "-vv",
        };
        m_vlcInstance = VLC::Instance( sizeof(args) / sizeof(args[0]), args );
        m_vlcInstance.logSet([](int lvl, const libvlc_log_t*, std::string msg) {
            if ( lvl == LIBVLC_ERROR )
                Log::Error( msg );
            else if ( lvl == LIBVLC_WARNING )
                Log::Warning( msg );
            else
                Log::Info( msg );
        });

        auto vlcService = std::unique_ptr<VLCMetadataService>( new VLCMetadataService( m_vlcInstance ) );
        auto thumbnailerService = std::unique_ptr<VLCThumbnailer>( new VLCThumbnailer( m_vlcInstance ) );
        addMetadataService( std::move( vlcService ) );
        addMetadataService( std::move( thumbnailerService ) );
    }

    m_discoverers.emplace_back( new FsDiscoverer( m_fsFactory ) );

    sqlite3* dbConnection;
    int res = sqlite3_open( dbPath.c_str(), &dbConnection );
    if ( res != SQLITE_OK )
        return false;
    m_dbConnection.reset( dbConnection, &sqlite3_close );
    if ( sqlite::Tools::executeRequest( DBConnection(m_dbConnection), "PRAGMA foreign_keys = ON" ) == false )
    {
        LOG_ERROR( "Failed to enable foreign keys" );
        return false;
    }
    if ( ! ( File::createTable( m_dbConnection ) &&
        Folder::createTable( m_dbConnection ) &&
        Label::createTable( m_dbConnection ) &&
        Album::createTable( m_dbConnection ) &&
        AlbumTrack::createTable( m_dbConnection ) &&
        Show::createTable( m_dbConnection ) &&
        ShowEpisode::createTable( m_dbConnection ) &&
        Movie::createTable( m_dbConnection ) &&
        VideoTrack::createTable( m_dbConnection ) &&
        AudioTrack::createTable( m_dbConnection ) &&
        Artist::createTable( m_dbConnection ) ) )
    {
        LOG_ERROR( "Failed to create database structure" );
        return false;
    }
    reload();
    return true;
}

std::vector<FilePtr> MediaLibrary::files()
{
    return File::fetchAll( m_dbConnection );
}

std::vector<FilePtr> MediaLibrary::audioFiles()
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name + " WHERE type = ?";
    //FIXME: Replace this with template magic in sqlite's traits
    using type_t = std::underlying_type<IFile::Type>::type;
    return sqlite::Tools::fetchAll<File, IFile>( m_dbConnection, req, static_cast<type_t>( IFile::Type::AudioType ) );
}

std::vector<FilePtr> MediaLibrary::videoFiles()
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name + " WHERE type = ?";
    using type_t = std::underlying_type<IFile::Type>::type;
    return sqlite::Tools::fetchAll<File, IFile>( m_dbConnection, req, static_cast<type_t>( IFile::Type::VideoType ) );
}

FilePtr MediaLibrary::file( const std::string& path )
{
    return File::fetch( m_dbConnection, path );
}

FilePtr MediaLibrary::addFile( const std::string& path, FolderPtr parentFolder )
{
    std::unique_ptr<fs::IFile> file;
    try
    {
        file = m_fsFactory->createFile( path );
    }
    catch (std::exception& ex)
    {
        LOG_ERROR( "Failed to create an IFile for ", path, ": ", ex.what() );
        return nullptr;
    }

    auto type = IFile::Type::UnknownType;
    if ( std::find( begin( supportedVideoExtensions ), end( supportedVideoExtensions ),
                    file->extension() ) != end( supportedVideoExtensions ) )
    {
        type = IFile::Type::VideoType;
    }
    else if ( std::find( begin( supportedAudioExtensions ), end( supportedAudioExtensions ),
                         file->extension() ) != end( supportedAudioExtensions ) )
    {
        type = IFile::Type::AudioType;
    }
    if ( type == IFile::Type::UnknownType )
        return false;

    auto fptr = File::create( m_dbConnection, type, file.get(), parentFolder != nullptr ? parentFolder->id() : 0 );
    if ( fptr == nullptr )
    {
        LOG_ERROR( "Failed to add file ", file->fullPath(), " to the media library" );
        return nullptr;
    }
    LOG_INFO( "Adding ", file->name() );
    if ( m_callback != nullptr )
        m_callback->onFileAdded( fptr );
    m_parser->parse( fptr, m_callback );
    return fptr;
}

FolderPtr MediaLibrary::folder( const std::string& path )
{
    return Folder::fetch( m_dbConnection, path );
}

bool MediaLibrary::deleteFile( const std::string& mrl )
{
    return File::destroy( m_dbConnection, mrl );
}

bool MediaLibrary::deleteFile( FilePtr file )
{
    return File::destroy( m_dbConnection, std::static_pointer_cast<File>( file ) );
}

bool MediaLibrary::deleteFolder( FolderPtr folder )
{
    if ( Folder::destroy( m_dbConnection, std::static_pointer_cast<Folder>( folder ) ) == false )
        return false;
    File::clear();
    return true;
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    return Label::create( m_dbConnection, label );
}

bool MediaLibrary::deleteLabel( const std::string& text )
{
    return Label::destroy( m_dbConnection, text );
}

bool MediaLibrary::deleteLabel( LabelPtr label )
{
    return Label::destroy( m_dbConnection, std::static_pointer_cast<Label>( label ) );
}

AlbumPtr MediaLibrary::album(const std::string& title )
{
    // We can't use Cache helper, since albums are cached by primary keys
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " WHERE title = ?";
    return sqlite::Tools::fetchOne<Album>( DBConnection( m_dbConnection ), req, title );
}

AlbumPtr MediaLibrary::createAlbum(const std::string& title )
{
    return Album::create( m_dbConnection, title );
}

std::vector<AlbumPtr> MediaLibrary::albums()
{
    return Album::fetchAll( m_dbConnection );
}

ShowPtr MediaLibrary::show(const std::string& name)
{
    static const std::string req = "SELECT * FROM " + policy::ShowTable::Name
            + " WHERE name = ?";
    return sqlite::Tools::fetchOne<Show>( m_dbConnection, req, name );
}

ShowPtr MediaLibrary::createShow(const std::string& name)
{
    return Show::create( m_dbConnection, name );
}

MoviePtr MediaLibrary::movie( const std::string& title )
{
    static const std::string req = "SELECT * FROM " + policy::MovieTable::Name
            + " WHERE title = ?";
    return sqlite::Tools::fetchOne<Movie>( m_dbConnection, req, title );
}

MoviePtr MediaLibrary::createMovie( const std::string& title )
{
    return Movie::create( m_dbConnection, title );
}

ArtistPtr MediaLibrary::artist(const std::string &name)
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name
            + " WHERE name = ?";
    return sqlite::Tools::fetchOne<Artist>( m_dbConnection, req, name );
}

ArtistPtr MediaLibrary::createArtist( const std::string& name )
{
    return Artist::create( m_dbConnection, name );
}

std::vector<ArtistPtr> MediaLibrary::artists() const
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name;
    return sqlite::Tools::fetchAll<Artist, IArtist>( m_dbConnection, req );
}

void MediaLibrary::addMetadataService(std::unique_ptr<IMetadataService> service)
{
    if ( service->initialize( m_parser.get(), this ) == false )
    {
        std::cout << "Failed to initialize service" << std::endl;
        return;
    }
    m_parser->addService( std::move( service ) );
}

void MediaLibrary::reload()
{
    //FIXME: Create a proper wrapper to handle discoverer threading
    std::thread t([this] {
        //FIXME: This will crash if the media library gets deleted while we
        //are discovering.
        for ( auto& d : m_discoverers )
            d->reload( this, this->m_dbConnection );
    });
    t.detach();
}

void MediaLibrary::discover( const std::string &entryPoint )
{
    std::thread t([this, entryPoint] {
        //FIXME: This will crash if the media library gets deleted while we
        //are discovering.
        if ( m_callback != nullptr )
            m_callback->onDiscoveryStarted( entryPoint );

        for ( auto& d : m_discoverers )
            d->discover( this, this->m_dbConnection, entryPoint );

        if ( m_callback != nullptr )
            m_callback->onDiscoveryCompleted( entryPoint );
    });
    t.detach();
}

const std::string& MediaLibrary::snapshotPath() const
{
    return m_snapshotPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

