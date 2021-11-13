//
// Created by Chanchan on 11/13/21.
//

#ifndef _HTTPSERVERBASE_H_
#define _HTTPSERVERBASE_H_

#include <workflow/WFServer.h>

namespace wfrest
{

template<class REQ, class RESP>
class HttpServerBase : public WFServerBase
{
public:
	HttpServerBase(const struct WFServerParams *params,
			 std::function<void (NetworkTask<REQ, RESP> *)> proc) :
		WFServerBase(params),
		process(std::move(proc))
	{
	}

	explicit HttpServerBase(std::function<void (WFNetworkTask<REQ, RESP> *)> proc) :
		WFServerBase(&SERVER_PARAMS_DEFAULT),
		process(std::move(proc))
	{
	}

protected:
	virtual CommSession *new_session(long long seq, CommConnection *conn);

protected:
	std::function<void (NetworkTask<REQ, RESP> *)> process{};
};

}  // namespace wfrest


#endif //_HTTPSERVERBASE_H_
