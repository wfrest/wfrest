#include "wfrest/RouteTable.h"
#include "wfrest/StringPiece.h"
using namespace wfrest;

int main()
{
    RouteTableNode rtn;
    StringPiece route1("/api/v1/www");
    rtn.find_or_create(route1, 0);
    StringPiece route2("/api/v1/test");
    rtn.find_or_create(route2, 0);
    StringPiece route3("/api/v2/test/v1");
    rtn.find_or_create(route3, 0);
    StringPiece route4("/abi/v2/test/v1");
    rtn.find_or_create(route4, 0);
    rtn.print_node_arch();
}