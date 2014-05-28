#ifndef IAUDIOTRACK_H
#define IAUDIOTRACK_H

#include "IMediaLibrary.h"

class IAudioTrack
{
    public:
        virtual ~IAudioTrack() {}
        virtual unsigned int id() const = 0;
        virtual const std::string& codec() const = 0;
        virtual unsigned int bitrate() const = 0;
};

#endif // IAUDIOTRACK_H