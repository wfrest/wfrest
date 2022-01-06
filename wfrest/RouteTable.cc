#include <queue>
#include "wfrest/RouteTable.h"

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

    // store GET("/", ...)
    if(cursor == 0 && route.as_string() == "/")
    {
        auto *new_node = new RouteTableNode();
        children_.insert({route, new_node});
        return new_node->find_or_create(route, ++cursor);
    }

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
                                        OUT std::map<std::string, std::string> &route_params,
                                        OUT std::string &route_match_path) const
{
    assert(cursor >= 0);
    // We found the route
    if ((cursor == route.size() and verb_handler_.handler != nullptr) or (children_.empty()))
        return iterator{this, route, verb_handler_};
    // route does not match any.
    if (cursor == route.size() and verb_handler_.handler == nullptr)
        return iterator{nullptr, route, verb_handler_};

    // find GET("/", ...)
    if(cursor == 0 && route.as_string() == "/")
    {
        // look for "/" in the children.
        auto it = children_.find(route);
        // it == <StringPiece: path level part, RouteTableNode* >
        if (it != children_.end())
        {
            // it2 == RouteTableNode::iterator
            // search in the corresponding child.
            auto it2 = it->second->find(route, ++cursor, route_params, route_match_path); 
            if (it2 != it->second->end())
                return it2;
        }
    }   

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
                StringPiece match_path(route.data() + cursor);
                route_match_path = mid.as_string() + match_path.as_string();
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

void RouteTableNode::bfs_transverse()
{
    std::queue<std::pair<StringPiece, RouteTableNode *>> node_queue;
    StringPiece root("/");
    node_queue.push({root, this});
    int level = 0;
    while(!node_queue.empty())
    {
        fprintf(stderr, "level %d:\t", level);
        size_t queue_size = node_queue.size();
        fprintf(stderr, "(size : %zu)\t", queue_size);  
        for(size_t i = 0; i < queue_size; i++)
        {
            std::pair<StringPiece, RouteTableNode *> node = node_queue.front();
            node_queue.pop();
            
            fprintf(stderr, "[%s :", node.first.as_string().c_str());
            const std::map<StringPiece, RouteTableNode *> &children = node.second->children_;
            
            if(children.empty()) 
                fprintf(stderr, "\tNULL");
            for (const auto &pair: children)
            {
                fprintf(stderr, "\t%s", pair.first.as_string().c_str());
                node_queue.push(pair);
            }
            fprintf(stderr, "]");
        }
        level++;
        fprintf(stderr, "\n");
    } 
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

