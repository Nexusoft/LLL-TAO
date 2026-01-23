/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/templates/key.h>
#include <LLD/include/enum.h>

namespace LLD
{

    /* Default Constructor. */
    SectorKey::SectorKey()
    : nSectorFile  (0)
    , nSectorSize  (0)
    , nSectorStart (0)
    , vKey         ( )
    {
    }


    /* Copy Constructor */
    SectorKey::SectorKey(const SectorKey& key)
    : nSectorFile  (key.nSectorFile)
    , nSectorSize  (key.nSectorSize)
    , nSectorStart (key.nSectorStart)
    , vKey         (key.vKey)
    {
    }


    /* Move Constructor */
    SectorKey::SectorKey(SectorKey&& key) noexcept
    : nSectorFile  (std::move(key.nSectorFile))
    , nSectorSize  (std::move(key.nSectorSize))
    , nSectorStart (std::move(key.nSectorStart))
    , vKey         (std::move(key.vKey))
    {
    }


    /* Copy Assignment Operator */
    SectorKey& SectorKey::operator=(const SectorKey& key)
    {
        nSectorFile     = key.nSectorFile;
        nSectorSize     = key.nSectorSize;
        nSectorStart    = key.nSectorStart;
        vKey            = key.vKey;

        return *this;
    }


    /* Move Assignment Operator */
    SectorKey& SectorKey::operator=(SectorKey&& key) noexcept
    {
        nSectorFile     = std::move(key.nSectorFile);
        nSectorSize     = std::move(key.nSectorSize);
        nSectorStart    = std::move(key.nSectorStart);
        vKey            = std::move(key.vKey);

        return *this;
    }


    /* Default Destructor */
    SectorKey::~SectorKey()
    {
    }


    /* Constructor */
    SectorKey::SectorKey(const std::vector<uint8_t>& vKeyIn,
              const uint16_t nSectorFileIn, const uint32_t nSectorStartIn, const uint32_t nSectorSizeIn)
    : nSectorFile(nSectorFileIn)
    , nSectorSize(nSectorSizeIn)
    , nSectorStart(nSectorStartIn)
    , vKey(vKeyIn)
    {
    }


    /*  Set the key for this object. */
    void SectorKey::SetKey(const std::vector<uint8_t>& vKeyIn)
    {
        vKey = vKeyIn;
    }


    /*  Iterator to the beginning of the raw key. */
    uint32_t SectorKey::Begin() const
    {
        return 10;
    }


    /*  Return the size of the key sector on disk. */
    uint32_t SectorKey::Size() const
    {
        return (10 + 16);
    }


    /*  Dump Key to Debug Console. */
    void SectorKey::Print() const
    {
        debug::log(0,
            "SectorKey(",
            ", nSectorFile=", nSectorFile,
            ", nSectorSize=", nSectorSize,
            ", nSectorStart=)", nSectorStart
        );
    }
}
