/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_MRUSET_H
#define NEXUS_MRUSET_H

#include <set>
#include <deque>

/**
 *
 *  STL-like set container that only keeps the most recent N elements.
 *
 **/
template <typename T>
class mruset
{
public:
    typedef T key_type;
    typedef T value_type;
    typedef typename std::set<T>::iterator iterator;
    typedef typename std::set<T>::const_iterator const_iterator;
    typedef typename std::set<T>::size_type size_type;

protected:
    std::set<T> set;
    std::deque<T> queue;
    size_type nMaxSize;

public:

    /**	mruset
     *
     *  Default constructor
     *
     *  @param nMaxSizeIn max size of most recent N elements
     *
     **/
    mruset(size_type nMaxSizeIn = 0)
    : nMaxSize(nMaxSizeIn) { }


    /**	begin
     *
     *  Get the begin iterator of the set
     *
     *  @return begin iterator of set
     *
     **/
    iterator begin() const
    {
        return set.begin();
    }


    /**	end
     *
     *  Get the end iterator of the set
     *
     *  @return end iterator of set
     *
     **/
    iterator end() const
    {
        return set.end();
    }


    /**	size
     *
     *  Get the size of the set container
     *
     *  @return size of the set
     *
     **/
    size_type size() const
    {
         return set.size();
    }


    /**	empty
     *
     *  Determine if the set is empty
     *
     *  @return True if empty, false otherwise
     *
     **/
    bool empty() const
    {
         return set.empty();
    }


    /**	find
     *
     *  Finds an element with the given key
     *
     *  @param[in] k The key to index into the set
     *
     *  @return The iterator to the element to find
     *
     **/
    iterator find(const key_type& k) const
    { return set.find(k);
    }


    /**	count
     *
     *  Get the count of elements with key k
     *
     *  @return count of elements with key k
     *
     **/
    size_type count(const key_type& k) const
    {
        return set.count(k);
    }


    /**	operator==
     *
     *  Determine if the two mru sets are equal
     *
     *  @param[in] a The first mruset to compare
     *  @param[in] b The second mruset to compare
     *
     *  @return True if the sets are equal, false otherwise
     *
     **/
    bool inline friend operator==(const mruset<T>& a, const mruset<T>& b)
    {
        return a.set == b.set;
    }


    /**	operator==
     *
     *  Determine if an mru set and a standard set are equal
     *
     *  @param[in] a The mru set to compare
     *  @param[in] b The standard set to compare
     *
     *  @return True if the sets are equal, false otherwise
     *
     **/
    bool inline friend operator==(const mruset<T>& a, const std::set<T>& b)
    {
        return a.set == b;
    }


    /**	operator<
     *
     *  Determine if mruset a is less than mruset b
     *
     *  @param[in] a The first mruset to compare
     *  @param[in] b The second mruset to compare
     *
     *  @return True if a is less than b, false otherwise
     *
     **/
    bool inline friend operator<(const mruset<T>& a, const mruset<T>& b)
    {
        return a.set < b.set;
    }


    /**	insert
     *
     *  Insert an element into the mru set
     *
     *  @param[in] x The element to insert
     *
     *  @return a pair containing iterator to inserted element and success flag.
     *
     **/
    std::pair<iterator, bool> insert(const key_type& x)
    {
        std::pair<iterator, bool> ret = set.insert(x);
        if (ret.second)
        {
            if (nMaxSize && queue.size() == nMaxSize)
            {
                set.erase(queue.front());
                queue.pop_front();
            }
            queue.push_back(x);
        }
        return ret;
    }


    /**	max_size
     *
     *  Get the max size of the mru set
     *
     *  @return The max size of the mru set
     *
     **/
    size_type max_size() const
    {
        return nMaxSize;
    }


    /**	max_size
     *
     *  Get and set the size of the mru set
     *
     *  @param[in] s The new size to set
     *
     *  @return The max size of the mru set
     *
     **/
    size_type max_size(size_type s)
    {
        if (s)
        {
            while (queue.size() >= s)
            {
                set.erase(queue.front());
                queue.pop_front();
            }
        }
        nMaxSize = s;
        return nMaxSize;
    }
};

#endif
