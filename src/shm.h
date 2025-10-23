#ifndef SHM_H
#define SHM_H

#include <string>
#include <atomic>

#include <hj/sync/shared_memory.hpp>

namespace livermore::tx
{

template <typename T>
struct shm
{
    shm(std::string code, const std::size_t sz = sizeof(T))
        : _shm_data{hj::shared_memory(code.c_str(), sz)}
        , _shm_flag{hj::shared_memory(code.append("_readable").c_str(),
                                      sizeof(std::int64_t))}
    {
        if(_shm_data.map() == nullptr)
            throw std::runtime_error("shm map data failed");

        if(_shm_flag.map() == nullptr)
            throw std::runtime_error("shm map flag failed");
    }
    ~shm() {}

    inline T *data()
    {
        return _data_null() ? nullptr : static_cast<T *>(_shm_data.addr());
    }
    inline bool readable() { return !_flag_null() && !_data_null(); }
    inline bool writeable() { return !_flag_null() && !_data_null(); }
    inline bool consumable()
    {
        return !_flag_null() && !_data_null() && *(_flag()) > 0;
    }

    bool write()
    {
        if(!writeable())
            return false;

        *(_flag()) += 1;
        return true;
    }
    bool write(const T *arg, const std::size_t nconsum = 1)
    {
        if(!writeable())
            return false;

        *(data()) = arg;
        *(_flag()) += nconsum;
        return true;
    }

    bool read()
    {
        if(!readable())
            return false;

        return true;
    }
    bool read(T *arg)
    {
        if(!readable())
            return false;

        *arg = *(data());
        return true;
    }

    bool consume()
    {
        if(!consumable())
            return false;

        *(_flag()) -= 1;
        return true;
    }
    bool consume(T *arg)
    {
        if(!consumable())
            return false;

        *(_flag()) -= 1;
        *arg() = *(data());
        return true;
    }

  private:
    inline bool          _data_null() { return _shm_data.addr() == nullptr; }
    inline bool          _flag_null() { return _shm_flag.addr() == nullptr; }
    inline std::int64_t *_flag()
    {
        return static_cast<std::int64_t *>(_shm_flag.addr());
    }

  private:
    hj::shared_memory _shm_data;
    hj::shared_memory _shm_flag;
};

} // namespace livermore::tx

#endif