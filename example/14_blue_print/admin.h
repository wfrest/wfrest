#include "wfrest/BluePrint.h"

using namespace wfrest;

inline void admin_pages(BluePrint *bp)
{
    bp->GET("/page/new/", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "New page\n");
    });

    bp->GET("/page/edit/", [](const HttpReq *req, HttpResp *resp)
    {
        fprintf(stderr, "Edit page\n");
    });
}