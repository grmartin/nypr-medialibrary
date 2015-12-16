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

#include "gtest/gtest.h"

#include "factory/IFileSystem.h"
#include "MediaLibrary.h"

class MediaLibraryWithoutParser : public MediaLibrary
{
    virtual void startParser() override {}
};

class MediaLibraryWithoutBackground : public MediaLibraryWithoutParser
{
    virtual void startDiscoverer() override {}
};

class Tests : public testing::Test
{
protected:
    std::unique_ptr<MediaLibrary> ml;

    virtual void SetUp() override;
    virtual void InstantiateMediaLibrary();
    void Reload( std::shared_ptr<factory::IFileSystem> fs = nullptr, IMediaLibraryCb* metadataCb = nullptr );
    virtual void TearDown() override;
};
