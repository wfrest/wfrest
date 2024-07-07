// Modified from lithium

#ifndef WFREST_ROUTETABLE_H_
#define WFREST_ROUTETABLE_H_

#include <vector>
#include <memory>
#include <cassert>
#include <unordered_map>

#include "StringPiece.h"
#include "VerbHandler.h"

namespace wfrest
{

class RouteTableNode : public Noncopyable
{
public:
    ~RouteTableNode();

    struct iterator
    {
        const RouteTableNode *ptr;
        StringPiece first;
        VerbHandler second;

        iterator *operator->()
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
    VerbHandler verb_handler_;
    std::map<StringPiece, RouteTableNode *> children_;
};

template<typename Func>
void RouteTableNode::all_routes(const Func &func, std::string prefix) const
{
    if (children_.empty())
    {
        func(prefix, verb_handler_);
    } else
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
    RouteTableNode root_;
    std::set<StringPiece> string_pieces_;  // check if exists
};

} // namespace wfrest

#endif // WFREST_ROUTETABLE_H_
