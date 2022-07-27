#ifndef WFREST_ASPECT_H_
#define WFREST_ASPECT_H_

#include <vector>

namespace wfrest
{

class HttpReq;

class HttpResp;

class Aspect
{
public:
    virtual ~Aspect() = default;

    virtual bool before(const HttpReq *req, HttpResp *resp) = 0;

    virtual bool after(const HttpReq *req, HttpResp *resp) = 0;
};

class GlobalAspect
{
public:
    static GlobalAspect *get_instance();

    GlobalAspect(const GlobalAspect&) = delete;

    GlobalAspect& operator=(const GlobalAspect&) = delete;

public:
    std::vector<Aspect *> aspect_list;

private:
    GlobalAspect() = default;
    
    ~GlobalAspect();
};

} // namespace wfrest



#endif // WFREST_ASPECT_H_