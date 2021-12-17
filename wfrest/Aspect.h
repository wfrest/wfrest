#ifndef WFREST_ASPECT_H_
#define WFREST_ASPECT_H_

namespace wfrest
{

class HttpReq;

class HttpResp;

class Aspect
{
public:
    virtual bool before(const HttpReq *req, HttpResp *resp) = 0;

    virtual bool after(const HttpReq *req, HttpResp *resp) = 0;
};


} // namespace wfrest



#endif // WFREST_ASPECT_H_