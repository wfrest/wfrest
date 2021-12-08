// Modified from lithium

#ifndef WFREST_ROUTETABLE_H_
#define WFREST_ROUTETABLE_H_

#include <vector>
#include <memory>
#include <cassert>
#include <unordered_map>

#include "wfrest/StringPiece.h"
#include "wfrest/Macro.h"
#include "wfrest/VerbHandler.h"
#include "Logger.h"

namespace wfrest
{
namespace detail
{
class RouteTableNode
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

    VerbHandler &find_or_create(const StringPiece &route, int cursor);

    iterator end() const
    { return iterator{nullptr, StringPiece(), VerbHandler()}; }

    iterator find(const StringPiece &route,
                  int cursor,
                  OUT RouteParams &route_params) const;

    template<typename Func>
    void all_routes(const Func &func, std::string prefix) const;

private:
    VerbHandler verb_handler_;
    std::unordered_map<StringPiece, RouteTableNode *, StringPieceHash> children_;
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

} // namespace detail

class RouteTable
{
public:
    // Find a route and return reference to the procedure.
    VerbHandler &operator[](const char *route);

    // todo : this interface is not very good
    detail::RouteTableNode::iterator find(const StringPiece &route, 
                                        OUT RouteParams &route_params) const
    { return root_.find(route, 0, route_params); }

    template<typename Func>
    void all_routes(const Func &func) const
    { root_.all_routes(func, ""); }

    detail::RouteTableNode::iterator end() const
    { return root_.end(); }

    ~RouteTable();
    
private:
    detail::RouteTableNode root_;
    std::vector<std::string *> strings; 
};

} // namespace wfrest

#endif // WFREST_ROUTETABLE_H_
