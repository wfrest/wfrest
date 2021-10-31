//
// Created by Chanchan on 10/30/21.
//

#ifndef _UNIQUEPTR_H_
#define _UNIQUEPTR_H_

namespace wfrest
{
    namespace detail
    {
        // C++11 version make_unique<T>()
        template<typename T, typename... Args>
        std::unique_ptr<T> make_unique(Args &&... args)
        {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }
    }
}


#endif //_UNIQUEPTR_H_
