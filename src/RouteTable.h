//
// Created by Chanchan on 10/30/21.
//
// Taken from lithium
//

#ifndef _ROUTETABLE_H_
#define _ROUTETABLE_H_

#include <vector>
#include <memory>
#include <cassert>
#include <unordered_map>
#include "StringPiece.h"
#include "UniquePtr.h"
#include "Router.h"
#include "Macro.h"

namespace wfrest
{
    namespace detail
    {

        template<typename VerbHandler>
        class RouteTableNode
        {
        public:
            struct iterator
            {
                const RouteTableNode<VerbHandler> *ptr;
                StringPiece first;
                VerbHandler second;

                iterator *operator->()
                { return this; }

                bool operator==(const iterator &other)
                { return this->ptr == other.ptr; }

                bool operator!=(const iterator &other)
                { return this->ptr != other.ptr; }
            };

            VerbHandler &find_or_create(const StringPiece &route, int cursor);

            iterator end() const
            { return iterator{nullptr, StringPiece(), VerbHandler()}; }

            auto find(const StringPiece &route, int cursor,
                      OUT std::unordered_map<std::string, std::string> &query_params) const -> iterator;

            template<typename Func>
            void all_routes(Func func, std::string prefix = "") const;

        private:
            VerbHandler verb_handler_;
            std::unordered_map<StringPiece, std::unique_ptr<RouteTableNode>, StringPieceHash> children_;
        };

        template<typename VerbHandler>
        template<typename Func>
        void RouteTableNode<VerbHandler>::all_routes(Func func, std::string prefix) const
        {
            if (children_.size() == 0)
            {
                func(prefix, verb_handler_);
            } else
            {
                if (!prefix.empty() && prefix.back() != '/')
                    prefix += '/';
                for (auto &pair: children_)
                    pair.second->all_routes(func, prefix + pair.first.as_string());
            }
        }

        template<typename VerbHandler>
        VerbHandler &RouteTableNode<VerbHandler>::find_or_create(const StringPiece &route, int cursor)
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
                auto ret = children_.insert({mid, make_unique<RouteTableNode>()});
                // ret -> pair<iterator,bool>
                // iterator->sencond = std::unique_ptr<RouteTableNode>
                return ret.first->second->find_or_create(route, cursor);
            }
        }

        template<typename VerbHandler>
        auto RouteTableNode<VerbHandler>::find(const StringPiece &route,
                                               int cursor, OUT
                                               std::unordered_map<std::string, std::string> &query_params) const -> RouteTableNode::iterator
        {
            assert(cursor >= 0);
            // We found the route
            if ((cursor == route.size() and verb_handler_.handler != nullptr) or (children_.size() == 0))
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
            // it == <StringPiece: path level part, unique_ptr<RouteTableNode> >
            if (it != children_.end())
            {
                // it2 == RouteTableNode::iterator
                auto it2 = it->second->find(route, cursor, query_params); // search in the corresponding child.
                if (it2 != it->second->end())
                    return it2;
            }

            // if one child is an url param <param_name>, choose it
            for (auto &kv: children_)
            {
                auto name = kv.first;
                if (name.size() > 2 and name[0] == '<' and
                    name[name.size() - 1] == '>')
                {
                    query_params[]
                    return kv.second->find(route, cursor, query_params);
                }

            }
            return end();
        }

    } // namespace detail


    template<typename VerbHandler>
    class RouteTable
    {
    public:
        // Find a route and return reference to the procedure.
        VerbHandler &operator[](const char *route);

        // Find a route and return an iterator.
        // c++14
        // 'auto' return without trailing return type; deduced return types are a C++14 extension
        // auto find(const StringPiece& r) const { return root.find(r, 0); }
        // c++11
        // use typename when refers to Out<T>::Inner, to indicates that this is a type, not a variable

        // todo : this interface is not very good
        auto find(const StringPiece &route, OUT std::unordered_map<std::string, std::string> &query_params) const
        -> typename detail::RouteTableNode<VerbHandler>::iterator
        { return root_.find(route, 0, query_params); }

        template<typename Func>
        void all_routes(Func func) const
        { root_.all_routes(func); }

        auto end() const
        -> typename detail::RouteTableNode<VerbHandler>::iterator
        { return root_.end(); }

    private:
        detail::RouteTableNode<VerbHandler> root_;
    };

    template<typename VerbHandler>
    VerbHandler &RouteTable<VerbHandler>::operator[](const char *route)
    {
        StringPiece route2(route);
        return root_.find_or_create(route2, 0);
    }

} // namespace wfrest
#endif // _ROUTETABLE_H_
