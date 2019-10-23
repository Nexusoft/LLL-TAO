/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <TAO/Register/types/state.h>
#include <TAO/Register/include/enum.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Default Constructor */
        State::State()
        : vchState     ( )
        , nVersion     (0)
        , nType        (0)
        , hashOwner    (0)
        , nCreated     (runtime::unifiedtimestamp())
        , nModified    (runtime::unifiedtimestamp())
        , hashChecksum (0)
        , nReadPos     (0)
        {
        }


        /* Copy Constructor */
        State::State(const State& state)
        : vchState     (state.vchState)
        , nVersion     (state.nVersion)
        , nType        (state.nType)
        , hashOwner    (state.hashOwner)
        , nCreated     (state.nCreated)
        , nModified    (state.nModified)
        , hashChecksum (state.hashChecksum)
        , nReadPos     (0)
        {
        }


        /* Move Constructor. */
        State::State(State&& state) noexcept
        : vchState     (std::move(state.vchState))
        , nVersion     (std::move(state.nVersion))
        , nType        (std::move(state.nType))
        , hashOwner    (std::move(state.hashOwner))
        , nCreated     (std::move(state.nCreated))
        , nModified    (std::move(state.nModified))
        , hashChecksum (std::move(state.hashChecksum))
        , nReadPos     (0)
        {
        }


        /* Copy assignment overload */
        State& State::operator=(const State& state)
        {
            vchState     = state.vchState;
            nVersion     = state.nVersion;
            nType        = state.nType;
            hashOwner    = state.hashOwner;
            nCreated     = state.nCreated;
            nModified    = state.nModified;
            hashChecksum = state.hashChecksum;

            nReadPos     = 0; //don't copy over read position

            return *this;
        }


        /* Move assignment overload */
        State& State::operator=(State&& state) noexcept
        {
            vchState     = std::move(state.vchState);
            nVersion     = std::move(state.nVersion);
            nType        = std::move(state.nType);
            hashOwner    = std::move(state.hashOwner);
            nCreated     = std::move(state.nCreated);
            nModified    = std::move(state.nModified);
            hashChecksum = std::move(state.hashChecksum);

            nReadPos     = 0; //don't copy over read position

            return *this;
        }


        /* Default Destructor */
        State::~State()
        {
        }


        /* Basic Type Constructor */
        State::State(const uint8_t nTypeIn)
        : vchState     ( )
        , nVersion     (1)
        , nType        (nTypeIn)
        , hashOwner    (9)
        , nCreated     (runtime::unifiedtimestamp())
        , nModified    (runtime::unifiedtimestamp())
        , hashChecksum (0)
        , nReadPos     (0)
        {
            vchState.clear();
        }


        /* Default Constructor */
        State::State(const std::vector<uint8_t>& vchData)
        : vchState     (vchData)
        , nVersion     (1)
        , nType        (0)
        , hashOwner    (0)
        , nCreated     (runtime::unifiedtimestamp())
        , nModified    (runtime::unifiedtimestamp())
        , hashChecksum (0)
        , nReadPos     (0)
        {
            SetChecksum();
        }

        /* Default Constructor */
        State::State(const uint8_t nTypeIn, const uint256_t& hashOwnerIn)
        : vchState     ( )
        , nVersion     (1)
        , nType        (nTypeIn)
        , hashOwner    (hashOwnerIn)
        , nCreated     (runtime::unifiedtimestamp())
        , nModified    (runtime::unifiedtimestamp())
        , hashChecksum (0)
        , nReadPos     (0)
        {
        }


        /* Default Constructor */
        State::State(const std::vector<uint8_t>& vchData, const uint8_t nTypeIn, const uint256_t& hashOwnerIn)
        : vchState     (vchData)
        , nVersion     (1)
        , nType        (nTypeIn)
        , hashOwner    (hashOwnerIn)
        , nCreated     (runtime::unifiedtimestamp())
        , nModified    (runtime::unifiedtimestamp())
        , hashChecksum (0)
        , nReadPos     (0)
        {
            SetChecksum();
        }


        /* Default Constructor */
        State::State(const uint64_t hashChecksumIn)
        : vchState     ( )
        , nVersion     (1)
        , nType        (0)
        , hashOwner    (uint256_t(0))
        , nCreated     (runtime::unifiedtimestamp())
        , nModified    (runtime::unifiedtimestamp())
        , hashChecksum (hashChecksumIn)
        , nReadPos     (0)
        {
        }


        /* Operator overload to check for equivilence. */
        bool State::operator==(const State& state) const
        {
            return GetHash() == state.GetHash();
        }


        /* Operator overload to check for non-equivilence. */
        bool State::operator!=(const State& state) const
        {
            return GetHash() != state.GetHash();
        }


        /* Set the State Register into a nullptr state. */
        void State::SetNull()
        {
            nVersion     = 0;
            nType        = 0;
            hashOwner    = uint256_t(0);
            nCreated     = 0;
            nModified    = 0;
            hashChecksum = 0;

            vchState.clear();
            nReadPos     = 0;
        }


        /* Null Checking flag for a State Register. */
        bool State::IsNull() const
        {
            return (vchState.size() == 0 && hashChecksum == 0);
        }


        /* Flag to determine if the state register has been pruned. */
        bool State::IsPruned() const
        {
            return (nVersion == 0 && vchState.size() == 0 && hashChecksum != 0);
        }


        /* Get the hash of the current state. */
        uint64_t State::GetHash() const
        {
            DataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            return LLC::SK64(ss.begin(), ss.end());
        }


        /* Set the Checksum of this Register. */
        void State::SetChecksum()
        {
            hashChecksum = GetHash();
        }


        /* Check if the register is valid. */
        bool State::IsValid() const
        {
            /* Check for null state. */
            if(IsNull())
                return debug::error(FUNCTION, "register cannot be null");

            /* Check the checksum. */
            if(GetHash() != hashChecksum)
                return debug::error(FUNCTION, "register checksum (", GetHash(), ") mismatch (", hashChecksum, ")");

            /* Check the timestamp. */
            if(nCreated > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "created timestamp too far in the future");

            /* Check the timestamp. */
            if(nModified > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "modified timestamp too far in the future");

            /* Check register version. */
            if(nVersion != 1) //TODO: make this a global constant
                return debug::error(FUNCTION, "register can't have version other than 1");

            /* System register specific checks. */
            if(nType == REGISTER::SYSTEM && hashOwner != 0)
                return debug::error(FUNCTION, "system register cannot have an owner");

            return true;
        }


        /* Get the State from the Register. */
        const std::vector<uint8_t>& State::GetState() const
        {
            return vchState;
        }


        /* Set the State from Byte Vector. */
        void State::SetState(const std::vector<uint8_t>& vchStateIn)
        {
            vchState = vchStateIn;

            SetChecksum();
        }


        /* Set the State from Byte Vector. */
        void State::Append(const std::vector<uint8_t>& vchStateIn)
        {
            vchState.insert(vchState.end(), vchStateIn.begin(), vchStateIn.end());

            SetChecksum();
        }


        /* Clear a register's state. */
        void State::ClearState()
        {
            vchState.clear();
            nReadPos = 0;
        }


        /* Detect end of register stream. */
        bool State::end() const
        {
            return nReadPos >= vchState.size();
        }


        /* Reads raw data from the stream. */
        const State& State::read(char* pch, int nSize) const
        {
            /* Check size constraints. */
            if(nReadPos + nSize > vchState.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "reached end of stream ", nReadPos));

            /* Copy the bytes into tmp object. */
            std::copy((uint8_t*)&vchState[nReadPos], (uint8_t*)&vchState[nReadPos] + nSize, (uint8_t*)pch);

            /* Iterate the read position. */
            nReadPos += nSize;

            return *this;
        }


        /*  Writes data into the stream. */
        State& State::write(const char* pch, int nSize)
        {
            /* Push the obj bytes into the vector. */
            vchState.insert(vchState.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

            /* Set the length and checksum. */
            SetChecksum();

            return *this;
        }


        /* Print debug information to the console and log file. */
        void State::print() const
        {
            debug::log(0,
                "State(version=", uint32_t(nVersion),
                ", type=", uint32_t(nType),
                ", length=", vchState.size(),
                ", owner=", hashOwner.SubString(),
                ", created=", nCreated,
                ", modified=", nModified,
                ", checksum=", hashChecksum,
                ", state=", HexStr(vchState.begin(), vchState.end()));
        }
    }
}
