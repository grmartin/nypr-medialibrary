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

#include "Mountpoint.h"

#include <mntent.h>
#include <cstring>

namespace
{
    // Allow private ctors to be used from make_shared
    struct MountpointBuilder : fs::Mountpoint
    {
        MountpointBuilder( const std::string& path ) : Mountpoint( path ) {}
    };
}

namespace fs
{

const Mountpoint::MountpointMap Mountpoint::Cache = Mountpoint::listMountpoints();
std::shared_ptr<IMountpoint> Mountpoint::unknownMountpoint = std::make_shared<UnknownMountpoint>();

Mountpoint::Mountpoint( const std::string& devicePath )
    : m_device( devicePath )
    , m_uuid( "fake uuid" )
{
}

const std::string& Mountpoint::uuid() const
{
    return m_uuid;
}

bool Mountpoint::isPresent() const
{
    return true;
}

bool Mountpoint::isRemovable() const
{
    return false;
}

std::shared_ptr<IMountpoint> Mountpoint::fromPath( const std::string& path )
{
    for ( const auto& p : Cache )
    {
        if ( path.find( p.first ) == 0 )
            return p.second;
    }
    return unknownMountpoint;
}

Mountpoint::MountpointMap Mountpoint::listMountpoints()
{
    MountpointMap res;
    FILE* f = setmntent("/etc/mtab", "r");
    if ( f == nullptr )
        throw std::runtime_error( "Failed to read /etc/mtab" );
    std::unique_ptr<FILE, int(*)(FILE*)>( f, &endmntent );
    char buff[512];
    mntent s;
    while ( getmntent_r( f, &s, buff, sizeof(buff) ) != nullptr )
    {
        // Ugly work around for mountpoints we don't care
        if ( strcmp( s.mnt_type, "proc" ) == 0
                || strcmp( s.mnt_type, "devtmpfs" ) == 0
                || strcmp( s.mnt_type, "devpts" ) == 0
                || strcmp( s.mnt_type, "sysfs" ) == 0
                || strcmp( s.mnt_type, "cgroup" ) == 0
                || strcmp( s.mnt_type, "debugfs" ) == 0
                || strcmp( s.mnt_type, "hugetlbfs" ) == 0
                || strcmp( s.mnt_type, "efivarfs" ) == 0
                || strcmp( s.mnt_type, "securityfs" ) == 0
                || strcmp( s.mnt_type, "mqueue" ) == 0
                || strcmp( s.mnt_type, "pstore" ) == 0
                || strcmp( s.mnt_type, "autofs" ) == 0
                || strcmp( s.mnt_type, "binfmt_misc" ) == 0
                || strcmp( s.mnt_type, "tmpfs" ) == 0 )
            continue;
        res[s.mnt_dir] = std::make_shared<MountpointBuilder>( s.mnt_fsname );
    }
    return res;
}

}
