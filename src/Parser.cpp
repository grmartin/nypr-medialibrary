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

#include "Parser.h"

#include <algorithm>

#include "IMedia.h"

Parser::Parser( DBConnection dbConnection )
    : m_thread( &Parser::run, this )
    , m_stopParser( false )
    , m_dbConnection( dbConnection )
{
}

Parser::~Parser()
{
    {
        std::lock_guard<std::mutex> lock( m_lock );
        if ( m_tasks.empty() == true )
            m_cond.notify_all();
        m_stopParser = true;
    }
    m_thread.join();
    while ( m_tasks.empty() == false )
    {
        delete m_tasks.front();
        m_tasks.pop();
    }
}

void Parser::addService(std::unique_ptr<IMetadataService> service)
{
    m_services.emplace_back( std::move( service ) );
    std::push_heap( m_services.begin(), m_services.end(), []( const ServicePtr& a, const ServicePtr& b )
    {
        // We want higher priority first
        return a->priority() < b->priority();
    });
}

void Parser::parse( std::shared_ptr<Media> file )
{
    std::lock_guard<std::mutex> lock( m_lock );

    if ( m_services.size() == 0 )
        return;
    m_tasks.push( new Task( file, m_services, m_callback ) );
    m_cond.notify_all();
}

void Parser::run()
{
    LOG_INFO("Starting Parser thread");
    restore();

    while ( m_stopParser == false )
    {
        Task* task = nullptr;
        {
            std::unique_lock<std::mutex> lock( m_lock );
            if ( m_tasks.empty() == true )
            {
                m_cond.wait( lock, [this]() { return m_tasks.empty() == false || m_stopParser == true; });
                // We might have been woken up because the parser is being destroyed
                if ( m_stopParser  == true )
                    break;
            }
            // Otherwise it's safe to assume we have at least one element.
            task = m_tasks.front();
            m_tasks.pop();
        }
        (*task->it)->run( task->file, task );
    }
    LOG_INFO("Exiting Parser thread");
}

void Parser::restore()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE parsed = 0";
    auto media = Media::fetchAll( m_dbConnection, req );

    std::lock_guard<std::mutex> lock( m_lock );
    for ( auto& it : media )
    {
        auto m = std::static_pointer_cast<Media>( it );
        //FIXME: No callback here
        m_tasks.push( new Task( m, m_services, nullptr ) );
    }
}


Parser::Task::Task(std::shared_ptr<Media> file, Parser::ServiceList& serviceList, IMediaLibraryCb* metadataCb )
    : file(file)
    , it( serviceList.begin() )
    , end( serviceList.end() )
    , cb( metadataCb )
{
}


void Parser::done(std::shared_ptr<Media> file, IMetadataService::Status status, void* data )
{
    Task *t = reinterpret_cast<Task*>( data );
    if ( status == IMetadataService::Status::TemporaryUnavailable ||
         status == IMetadataService::Status::Fatal )
    {
        delete t;
        return;
    }
    else if ( status == IMetadataService::Status::Success )
    {
        t->cb->onFileUpdated( file );
    }

    ++t->it;
    if (t->it == t->end)
    {
        file->markParsed();
        delete t;
        return;
    }
    std::lock_guard<std::mutex> lock( m_lock );
    m_tasks.push( t );
    m_cond.notify_all();
}
