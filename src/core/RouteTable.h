// Modified from lithium

#ifndef WFREST_ROUTETABLE_H_
#define WFREST_ROUTETABLE_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "StringPiece.h"
#include "VerbHandler.h"

namespace wfrest
{

class RouteTableNode : public Noncopyable
{
public:
    ~RouteTableNode() = default;

    struct iterator
    {
        const RouteTableNode *ptr;
        StringPiece first;
        VerbHandler second;

        iterator *operator->()
        { return this; }

        const iterator *operator->() const
        { return this; }

        bool operator==(const iterator &other) const
        { return this->ptr == other.ptr; }

        bool operator!=(const iterator &other) const
        { return this->ptr != other.ptr; }
    };

    VerbHandler &find_or_create(const StringPiece &route, size_t cursor);

    iterator end() const
    { return iterator{nullptr, StringPiece(), VerbHandler()}; }

    iterator find(const StringPiece &route,
                  size_t cursor,
                  std::map<std::string, std::string> &route_params,
                  std::string &route_match_path) const;

    template<typename Func>
    void all_routes(const Func &func, std::string prefix) const;

    void print_node_arch();  // for test
    
private:
    RouteTableNode *find_or_create_child(const StringPiece &segment);

    iterator find_impl(const StringPiece &route,
                       size_t cursor,
                       std::map<std::string, std::string> &route_params,
                       std::string &route_match_path) const;

private:
    VerbHandler verb_handler_;
    std::set<std::string> child_keys_;
    std::map<StringPiece, std::unique_ptr<RouteTableNode>> children_;
};

template<typename Func>
void RouteTableNode::all_routes(const Func &func, std::string prefix) const
{
    if (!verb_handler_.verb_handler_map.empty())
        func(prefix, verb_handler_);

    if (!children_.empty())
    {
        if (!prefix.empty() && prefix.back() != '/')
            prefix += '/';
        for (auto &pair: children_)
        {
            pair.second->all_routes(func, prefix + pair.first.as_string());
        }
    }
}

class RouteTable : public Noncopyable
{ 
public:
    // Find a route and return reference to the procedure.
    VerbHandler &find_or_create(const char *route);

    RouteTableNode::iterator find(const StringPiece &route, 
                                  std::map<std::string, std::string> &route_params,
                                  std::string &route_match_path) const
    { return root_.find(route, 0, route_params, route_match_path); }

    template<typename Func>
    void all_routes(const Func &func) const
    { root_.all_routes(func, ""); }

    RouteTableNode::iterator end() const
    { return root_.end(); }

    ~RouteTable() = default;

    void print_node_arch() { root_.print_node_arch(); }  // for test
    
private:
    std::set<std::string> routes_;
    RouteTableNode root_;
};

} // namespace wfrest

#endif // WFREST_ROUTETABLE_H_
