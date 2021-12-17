#include "wfrest/BluePrint.h"
#include "wfrest/HttpMsg.h"

using namespace wfrest;

void BluePrint::add_blueprint(const BluePrint &bp, const std::string &url_prefix)
{
    bp.router_.routes_map_.all_routes([this, &url_prefix](
            const std::string &sub_prefix, VerbHandler verb_handler)
                                      {
                                          if (!url_prefix.empty() && url_prefix.back() == '/')
                                              verb_handler.path = url_prefix + sub_prefix;
                                          else
                                              verb_handler.path = url_prefix + "/" + sub_prefix;

                                          this->router_.routes_map_[verb_handler.path.c_str()] = verb_handler;
                                      });
}
