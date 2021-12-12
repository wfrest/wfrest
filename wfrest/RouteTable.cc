#include "wfrest/RouteTable.h"
#include "Logger.h"

using namespace wfrest;

RouteTableNode::~RouteTableNode()
{
    for (auto &child: children_)
    {
        delete child.second;
    }
}

VerbHandler &RouteTableNode::find_or_create(const StringPiece &route, int cursor)
{
    if (cursor == route.size())
        return verb_handler_;

    if (route[cursor] == '/')
        cursor++; // skip the /
    int anchor = cursor;
    while (cursor < route.size() and route[cursor] != '/')
        cursor++;

    // get the '/ {mid} /' part
    StringPiece mid(route.begin() + anchor, cursor - anchor);
    auto it = children_.find(mid);
    if (it != children_.end())
    {
        return children_[mid]->find_or_create(route, cursor);
    } else
    {
        auto *new_node = new RouteTableNode();
        children_.insert({mid, new_node});
        return new_node->find_or_create(route, cursor);
    }
}

RouteTableNode::iterator RouteTableNode::find(const StringPiece &route,
                                              int cursor,
                                              OUT RouteParams &route_params,
                                              OUT std::string &route_match_path) const
{
    assert(cursor >= 0);
    // We found the route
    if ((cursor == route.size() and verb_handler_.handler != nullptr) or (children_.empty()))
        return iterator{this, route, verb_handler_};
    // route does not match any.
    if (cursor == route.size() and verb_handler_.handler == nullptr)
        return iterator{nullptr, route, verb_handler_};

    if (route[cursor] == '/')
        cursor++; // skip the first /
    // Find the next /.
    // mark an anchor here
    int anchor = cursor;
    while (cursor < route.size() and route[cursor] != '/')
        cursor++;

    // mid is the string between the 2 /.
    // / {mid} /
    StringPiece mid(route.begin() + anchor, cursor - anchor);

    // look for mid in the children.
    auto it = children_.find(mid);
    // it == <StringPiece: path level part, RouteTableNode* >
    if (it != children_.end())
    {
        // it2 == RouteTableNode::iterator
        auto it2 = it->second->find(route, cursor, route_params, route_match_path); // search in the corresponding child.
        if (it2 != it->second->end())
            return it2;
    }

    // if one child is an url param {name}, choose it
    for (auto &kv: children_)
    {
        StringPiece param(kv.first);
        if (!param.empty() && param[param.size() - 1] == '*')
        {
            StringPiece match(param);
            match.remove_suffix(1);
            if (mid.starts_with(match))
            {
                fprintf(stderr, "wildcast * : %s\n", route.data());
                route_match_path = std::string(mid.data());
                LOG_INFO << "match path : " << route_match_path;
                return iterator{kv.second, route, kv.second->verb_handler_};
            }
        }

        if (param.size() > 2 and param[0] == '{' and
            param[param.size() - 1] == '}')
        {
            int i = 1;
            int j = param.size() - 2;
            while (param[i] == ' ') i++;
            while (param[j] == ' ') j--;

            param.shrink(i, param.size() - 1 - j);
            route_params[param.as_string()] = mid.as_string();
            return kv.second->find(route, cursor, route_params, route_match_path);
        }
    }
    return end();
}



VerbHandler &RouteTable::operator[](const char *route)
{
    // Use pointer to prevent iterator invalidation
    // StringPiece is only a watcher, so we should store the string.
    std::string *p_route = new std::string(route);
    strings.push_back(p_route);
    StringPiece route2(*p_route);
    return root_.find_or_create(route2, 0);
}

RouteTable::~RouteTable()
{
    for(auto str : strings)
    {
        delete str;
    }
}

