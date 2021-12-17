#ifndef WFREST_AOP_H_
#define WFREST_AOP_H_

namespace wfrest
{

class HttpReq;

class HttpResp;

class AOP
{
public:
    virtual bool before(const HttpReq *req, HttpResp *resp) = 0;

    virtual bool after(const HttpReq *req, HttpResp *resp) = 0;
};


} // namespace wfrest



#endif // WFREST_AOP_H_