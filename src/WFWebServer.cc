#include <cstring>
#include <workflow/HttpUtil.h>
#include <workflow/HttpMessage.h>
#include <spdlog/spdlog.h>

#include "WFWebServer.h"
#include "WFWebServerTask.h"

namespace wfrest
{

WFWebServer& WFWebServer::Get(const std::string &path, Handler handler) 
{
	if (path.empty())
		return *this;

	auto it = get_handlers_.find(path);
	if (it != get_handlers_.end())
		it->second = handler;
	else
		it = get_handlers_.emplace(std::move(path), std::move(handler)).first;
		
    return *this;
}

// todo : refactor here
WFWebServer& WFWebServer::Post(const std::string &path, Handler handler) 
{
	if (path.empty())
		return *this;

	auto it = post_handlers_.find(path);
	if (it != post_handlers_.end())
		it->second = handler;
	else
		it = post_handlers_.emplace(std::move(path), std::move(handler)).first;

    return *this;
}

void WFWebServer::dispatch_request(const HttpReq *req, 
								HttpResp *resp,
								const Handlers &handlers,
								bool is_ssl) 
{
	spdlog::info("dispatch_request");
	std::string host;
	protocol::HttpHeaderCursor cursor(req);
	cursor.find("Host", host);

	if (host.empty())
	{
		//header Host not found
		protocol::HttpUtil::set_response_status(resp, HttpStatusBadRequest);
		return;
	}

	std::string request_uri;

	if (is_ssl)
		request_uri = "https://";
	else
		request_uri = "http://";

	request_uri += host;
	request_uri += req->get_request_uri();

	ParsedURI uri;

	if (URIParser::parse(request_uri, uri) < 0)
	{
		//parse error
		//todo 500 status code when uri sys error
		protocol::HttpUtil::set_response_status(resp, HttpStatusBadRequest);
		return;
	}

	std::string path;

	if (uri.path && uri.path[0])
		path = uri.path;
	else
		path = "/";
	const auto it = handlers.find(path);

	if (it == handlers.end())
	{
		protocol::HttpUtil::set_response_status(resp, HttpStatusNotFound);
		return;
	}
	else 
	{
		it->second(req, resp);   // handler
	}
}

void WFWebServer::proc(WFWebTask *server_task) 
{
	auto *req = server_task->get_req();
	auto *resp = server_task->get_resp();

	resp->set_http_version("HTTP/1.1");
	WFGoTask *go_task = nullptr;
	// not case sensitive compare
	if(strcasecmp(req->get_method(), "GET") == 0 || strcasecmp(req->get_method(), "HEAD") == 0)
	{
		// todo : Should we just dispatch_request() here?
		go_task = WFTaskFactory::create_go_task("handler", dispatch_request, req, resp, get_handlers_, is_ssl_);
	} 
	else if(strcasecmp(req->get_method(), "POST") == 0)
	{
		go_task = WFTaskFactory::create_go_task("handler", dispatch_request, req, resp, post_handlers_, is_ssl_);
	}
	// .... other method
	server_task->set_callback([](WFWebTask* ) {
		spdlog::info("done");
	});
	
	**server_task << go_task;
}


class __WFHttpTask : public WFServerTask<HttpReq, HttpResp>
{
public:
	static size_t get_req_offset()
	{
		__WFHttpTask task(nullptr);

		return task.req_offset();
	}

	static size_t get_resp_offset()
	{
		__WFHttpTask task(nullptr);

		return task.resp_offset();
	}
private:
	__WFHttpTask(std::function<void (WFWebTask *)> proc):
		WFServerTask(nullptr, nullptr, proc)
	{}

	size_t req_offset() const
	{
		return (const char *)(&this->req) - (const char *)this;
	}

	size_t resp_offset() const
	{
		return (const char *)(&this->resp) - (const char *)this;
	}
};


WFWebTask* WFWebServer::task_of(const HttpReq *req) 
{
	size_t http_req_offset = __WFHttpTask::get_req_offset();
	return (WFWebTask *)((char *)(req) - http_req_offset);
}

WFWebTask* WFWebServer::task_of(const HttpResp *resp) 
{
	size_t http_resp_offset = __WFHttpTask::get_resp_offset();
	return (WFWebTask *)((char *)(resp) - http_resp_offset);
}

CommSession* WFWebServer::new_session(long long seq, CommConnection *conn) 
{
	WFWebTask* task;
	task = WFWebTaskFactory::create_web_task(this, this->process);
	task->set_keep_alive(this->params.keep_alive_timeout);
	task->set_receive_timeout(this->params.receive_timeout);
	task->get_req()->set_size_limit(this->params.request_size_limit);

	return task;
}

}  // namespace wfrest


