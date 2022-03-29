#pragma once

#include <cstdint>

namespace coro
{
    using HandleID = uint64_t;

    class handle
    {
    public:
        handle() : id(id_gen++) { }
        virtual ~handle() = default;
        
        HandleID get_handle_id() { return id; }

        virtual void run() = 0;
        virtual void dump_backtrace(size_t) const { }

    private:
        HandleID id;
        inline static HandleID id_gen = 0;
    };

    struct handle_wrapper
    {
        HandleID id;
        handle* handle;
    };
}
