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

namespace wfrest
{
    namespace detail
    {

        template <typename VerbHandler>
        class RouteTableNode {
        public:
            RouteTableNode() = default;
            struct iterator
            {
                const RouteTableNode<VerbHandler>* ptr;
                StringPiece first;
                VerbHandler second;

                iterator* operator->() { return this; }
                bool operator==(const iterator& other) { return this->ptr == other.ptr; }
                bool operator!=(const iterator& other) { return this->ptr != other.ptr; }
            };

            VerbHandler& find_or_create(const StringPiece& route, int cursor);
            iterator end() { return iterator(nullptr, StringPiece(), VerbHandler()); }
            auto find(const StringPiece& route, int cursor) const -> iterator;

            template <typename F>
            void for_all_routes(F f, std::string prefix = "") const;

        private:
            VerbHandler verb_handler_;
            std::unordered_map<StringPiece, RouteTableNode *, StringPieceHash> children_;
        };

        template<typename VerbHandler>
        template<typename F>
        void RouteTableNode<VerbHandler>::for_all_routes(F f, std::string prefix) const {
            if (children_.size() == 0)
                f(prefix, verb_handler_);
            else {
                if (!prefix.empty() && prefix.back() != '/')
                    prefix += '/';
                for (auto pair : children_)
                    pair.second->for_all_routes(f, prefix + std::string(pair.first));
            }
        }

        // todo : optimize here
        template<typename VerbHandler>
        VerbHandler &RouteTableNode<VerbHandler>::find_or_create(const StringPiece& route, int cursor) {
            if (cursor == route.size())
                return verb_handler_;

            if (route[cursor] == '/')
                cursor++; // skip the /
            int anchor = cursor;
            while (cursor < route.size() and route[cursor] != '/')
                cursor++;

            StringPiece mid = StringPiece(route.begin() + anchor, cursor - anchor);
            auto it = children_.find(mid);
            if (it != children_.end())
                return children_[mid]->find_or_create(route, cursor);
            else
            {
                auto new_node = new RouteTableNode();   // todo : delete
                children_.insert({mid, new_node});
                return new_node->find_or_create(route, cursor);
            }
        }

        template<typename VerbHandler>
        auto RouteTableNode<VerbHandler>::find(const StringPiece &route, int cursor) const -> RouteTableNode::iterator
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
            StringPiece mid(route.begin() + anchor, cursor - anchor);

            // look for mid in the children.
            auto it = children_.find(mid);
            if (it != children_.end())
            {
                auto it2 = it->second->find(route, cursor); // search in the corresponding child.
                if (it2 != it->second->end())
                    return it2;
            }

            // if one child is an url param <param_name>, choose it
            for (auto& kv : children_) {
                auto name = kv.first;
                if (name.size() > 2 and name[0] == '<' and
                    name[name.size() - 1] == '>')
                    return kv.second->find(route, cursor);
            }
            return end();
        }


    } // namespace detail


    template <typename VerbHandler>
    class RouteTable
    {
    public:
        // Find a route and return reference to a procedure.
        VerbHandler& operator[](const StringPiece& route);
        VerbHandler& operator[](const std::string& route);

        // Find a route and return an iterator.
        // c++14
        // 'auto' return without trailing return type; deduced return types are a C++14 extension
        // auto find(const StringPiece& r) const { return root.find(r, 0); }
        // c++11
        // use typename when refers to Out<T>::Inner, to indicates that this is a type, not a variable
        auto find(const StringPiece& route) const
            -> typename detail::RouteTableNode<VerbHandler>::iterator
            { return root_.find(route, 0); }

        template <typename F>
        void for_all_routes(F f) const { root_.for_all_routes(f); }

        auto end() const
            -> typename detail::RouteTableNode<VerbHandler>::iterator
            { return root_.end(); }

    private:
        std::vector<std::unique_ptr<std::string> > strings_;
        detail::RouteTableNode<VerbHandler> root_;
    };

    template<typename VerbHandler>
    VerbHandler &RouteTable<VerbHandler>::operator[](const StringPiece &route)
    {
        strings_.emplace_back(detail::make_unique<std::string>(route));
        StringPiece route2(*strings_.back());
        return root_.find_or_create(route2, 0);
    }

    template<typename VerbHandler>
    VerbHandler &RouteTable<VerbHandler>::operator[](const std::string &route)
    {
        strings_.emplace_back(detail::make_unique<std::string>(route));
        StringPiece route2(*strings_.back());
        return root_.find_or_create(route2, 0);
    }

} // namespace wfrest
#endif // _ROUTETABLE_H_
