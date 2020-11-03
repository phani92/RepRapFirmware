///////////////////////////////////////////////////////////////////////////////
// BOSSA
//
// Copyright (c) 2011-2018, ShumaTech
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the <organization> nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
#if 0
#include <string>
#include <exception>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#endif

#include "Flasher.h"
#include "RepRap.h"
#include "Platform.h"
#include "General/Vector.hpp"

#if 0
void
FlasherInfo::print()
{
    bool first;

    printf("Device       : %s\n", name.c_str());
    printf("Version      : %s\n", version.c_str());
    printf("Address      : 0x%x\n", address);
    printf("Pages        : %d\n", numPages);
    printf("Page Size    : %d bytes\n", pageSize);
    printf("Total Size   : %dKB\n", totalSize / 1024);
    printf("Planes       : %d\n", numPlanes);
    printf("Lock Regions : %zd\n", lockRegions.size());
    printf("Locked       : ");
    first = true;
    for (uint32_t region = 0; region < lockRegions.size(); region++)
    {
        if (lockRegions[region])
        {
            printf("%s%d", first ? "" : ",", region);
            first = false;
        }
    }
    printf("%s\n", first ? "none" : "");
    printf("Security     : %s\n", security ? "true" : "false");
    if (canBootFlash)
        printf("Boot Flash   : %s\n", bootFlash ? "true" : "false");
    if (canBod)
        printf("BOD          : %s\n", bod ? "true" : "false");
    if (canBor)
        printf("BOR          : %s\n", bor ? "true" : "false");
}
#endif

void
Flasher::erase(uint32_t foffset)
{
#if 0
    _observer.onStatus("Erase flash\n");
#endif
    _flash->eraseAll(foffset);
    _flash->eraseAuto(false);
}

void
Flasher::write(const char* file, const char* dir, uint32_t foffset)
{
    uint32_t pageSize = _flash->pageSize();
    uint32_t pageNum = 0;
    uint32_t numPages;
    size_t fbytes;

#if 0
    if (foffset % pageSize != 0 || foffset >= _flash->totalSize())
        throw FlashOffsetError();

    infile = fopen(filename, "rb");
    if (!infile)
        throw FileOpenError(errno);
#endif

	Platform& platform = reprap.GetPlatform();
	static const MessageType mt = AddError(MessageType::GenericMessage);
	auto infile = platform.OpenFile(dir, file, OpenMode::read);
	if (infile == nullptr)
	{
		platform.MessageF(mt, "Failed to open file %s\n", file);
		return;
	}
	const auto fileSize = infile->Length();
	if (fileSize == 0)
	{
		infile->Close();
		platform.MessageF(mt, "Upload file is empty %s\n", file);
		return;
	}

    try
    {
#if 0
        if (fseek(infile, 0, SEEK_END) != 0 || (fsize = ftell(infile)) < 0)
            throw FileIoError(errno);

        rewind(infile);
#endif

        numPages = (fileSize + pageSize - 1) / pageSize;
        if (numPages > _flash->numPages())
            throw FileSizeError("Flasher::write: FileSizeError");

#if 0
        _observer.onStatus("Write %ld bytes to flash (%u pages)\n", fsize, numPages);
#endif

        if (_samba.canWriteBuffer())
        {
            uint32_t offset = 0;
            uint32_t bufferSize = _samba.writeBufferSize();
            uint8_t buffer[bufferSize];

            while ((fbytes = infile->Read(buffer, bufferSize)) > 0)
            {
#if 0
                _observer.onProgress(offset / pageSize, numPages);
#endif

                if (fbytes < bufferSize)
                {
                    memset(buffer + fbytes, 0, bufferSize - fbytes);
                    fbytes = (fbytes + pageSize - 1) / pageSize * pageSize;
                }

                _flash->loadBuffer(buffer, fbytes);
                _flash->writeBuffer(foffset + offset, fbytes);
                offset += fbytes;
            }

        }
        else
        {
            uint8_t buffer[pageSize];
            uint32_t pageOffset = foffset / pageSize;

            while ((fbytes = infile->Read(buffer, pageSize)) > 0)
            {
#if 0
                _observer.onProgress(pageNum, numPages);
#endif

                _flash->loadBuffer(buffer, fbytes);
                _flash->writePage(pageOffset + pageNum);

                pageNum++;
                if (pageNum == numPages || fbytes != pageSize)
                    break;
            }

        }
    }
    catch(...)
    {
        infile->Close();
        throw;
    }

    infile->Close();
#if 0
    _observer.onProgress(numPages, numPages);
#endif
}

bool
Flasher::verify(const char* file, const char* dir, uint32_t& pageErrors, uint32_t& totalErrors, uint32_t foffset)
{
    uint32_t pageSize = _flash->pageSize();
    uint8_t bufferA[pageSize];
    uint8_t bufferB[pageSize];
    uint32_t pageNum = 0;
    uint32_t numPages;
    uint32_t pageOffset;
    uint32_t byteErrors = 0;
    uint16_t flashCrc;
    size_t fbytes;

    pageErrors = 0;
    totalErrors = 0;

#if 0
    if (foffset % pageSize != 0 || foffset >= _flash->totalSize())
        throw FlashOffsetError();
#endif

    pageOffset = foffset / pageSize;

	Platform& platform = reprap.GetPlatform();
	static const MessageType mt = AddError(MessageType::GenericMessage);
	auto infile = platform.OpenFile(dir, file, OpenMode::read);
	if (infile == nullptr)
	{
		platform.MessageF(mt, "Failed to open file %s\n", file);
		return false;
	}
	const auto fileSize = infile->Length();
	if (fileSize == 0)
	{
		infile->Close();
		platform.MessageF(mt, "Upload file is empty %s\n", file);
		return false;
	}

    try
    {
#if 0
        if (fseek(infile, 0, SEEK_END) != 0 || (fsize = ftell(infile)) < 0)
            throw FileIoError(errno);

        rewind(infile);
#endif

        numPages = (fileSize + pageSize - 1) / pageSize;
        if (numPages > _flash->numPages())
            throw FileSizeError("Flasher::verify: FileSizeError");

#if 0
        _observer.onStatus("Verify %ld bytes of flash\n", fsize);
#endif

        while ((fbytes = infile->Read(bufferA, pageSize)) > 0)
        {
            byteErrors = 0;

#if 0
            _observer.onProgress(pageNum, numPages);
#endif

            if (_samba.canChecksumBuffer())
            {
                uint16_t calcCrc = 0;
                for (uint32_t i = 0; i < fbytes; i++)
                    calcCrc = _samba.checksumCalc(bufferA[i], calcCrc);

                flashCrc = _samba.checksumBuffer((pageOffset + pageNum) * pageSize, fbytes);

                if (flashCrc != calcCrc)
                {
                    _flash->readPage(pageOffset + pageNum, bufferB);

                    for (uint32_t i = 0; i < fbytes; i++)
                    {
                        if (bufferA[i] != bufferB[i])
                            byteErrors++;
                    }
                }
            }
            else
            {
                _flash->readPage(pageOffset + pageNum, bufferB);

                for (uint32_t i = 0; i < fbytes; i++)
                {
                    if (bufferA[i] != bufferB[i])
                        byteErrors++;
                }
            }

            if (byteErrors != 0)
            {
                pageErrors++;
                totalErrors += byteErrors;
            }

            pageNum++;
            if (pageNum == numPages || fbytes != pageSize)
                break;
        }
    }
    catch(...)
    {
        infile->Close();
        throw;
    }

    infile->Close();

#if 0
     _observer.onProgress(numPages, numPages);
#endif

    if (pageErrors != 0)
        return false;

    return true;
}

#if 0
void
Flasher::read(const char* filename, uint32_t fsize, uint32_t foffset)
{
    FILE* outfile;
    uint32_t pageSize = _flash->pageSize();
    uint8_t buffer[pageSize];
    uint32_t pageNum = 0;
    uint32_t pageOffset;
    uint32_t numPages;
    size_t fbytes;

    if (foffset % pageSize != 0 || foffset >= _flash->totalSize())
        throw FlashOffsetError();

    pageOffset = foffset / pageSize;

    if (fsize == 0)
        fsize = pageSize * (_flash->numPages() - pageOffset);

    numPages = (fsize + pageSize - 1) / pageSize;
    if (pageOffset + numPages > _flash->numPages())
        throw FileSizeError();

    outfile = fopen(filename, "wb");
    if (!outfile)
        throw FileOpenError(errno);

    _observer.onStatus("Read %d bytes from flash\n", fsize);

    try
    {
        for (pageNum = 0; pageNum < numPages; pageNum++)
        {
            _observer.onProgress(pageNum, numPages);

            _flash->readPage(pageOffset + pageNum, buffer);

            if (pageNum == numPages - 1 && fsize % pageSize > 0)
                pageSize = fsize % pageSize;
            fbytes = fwrite(buffer, 1, pageSize, outfile);
            if (fbytes != pageSize)
                throw FileShortError();
        }
    }
    catch(...)
    {
        fclose(outfile);
        throw;
    }

    _observer.onProgress(numPages, numPages);

    fclose(outfile);
}
#endif

void
Flasher::lock(/* string& regionArg, */ bool enable)
{
#if 0
    if (regionArg.empty())
    {
        _observer.onStatus("%s all regions\n", enable ? "Lock" : "Unlock");
#endif
        Vector<bool, 16> regions(_flash->lockRegions(), enable);
        _flash->setLockRegions(regions);
#if 0
    }
    else
    {
        size_t pos = 0;
        size_t delim;
        uint32_t region;
        string sub;
        std::vector<bool> regions = _flash->getLockRegions();

        do
        {
            delim = regionArg.find(',', pos);
            sub = regionArg.substr(pos, delim - pos);
            region = strtol(sub.c_str(), NULL, 0);
            _observer.onStatus("%s region %d\n", enable ? "Lock" : "Unlock", region);
            regions[region] = enable;
            pos = delim + 1;
        } while (delim != string::npos);

        _flash->setLockRegions(regions);
    }
#endif
}

#if 0
void
Flasher::info(FlasherInfo& info)
{
    info.name = _flash->name();
    info.version = _samba.version();
    info.address = _flash->address();
    info.numPages = _flash->numPages();
    info.pageSize = _flash->pageSize();
    info.totalSize = _flash->numPages() * _flash->pageSize();
    info.numPlanes = _flash->numPlanes();
    info.security = _flash->getSecurity();
    info.bootFlash = _flash->getBootFlash();
    info.bod = _flash->getBod();
    info.bor = _flash->getBor();

    info.canBootFlash = _flash->canBootFlash();
    info.canBod = _flash->canBod();
    info.canBor = _flash->canBor();
    info.canChipErase = _samba.canChipErase();
    info.canWriteBuffer = _samba.canWriteBuffer();
    info.canChecksumBuffer = _samba.canChecksumBuffer();
    info.lockRegions = _flash->getLockRegions();
}
#endif
