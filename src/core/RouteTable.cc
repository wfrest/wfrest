#include <queue>
#include <utility>
#include "RouteTable.h"

using namespace wfrest;

namespace
{

bool parameter_name(const StringPiece &pattern, std::string *name)
{
    if (pattern.size() <= 2 || pattern.data()[0] != '{' ||
        pattern.data()[pattern.size() - 1] != '}')
    {
        return false;
    }

    size_t begin = 1;
    size_t end = pattern.size() - 1;
    while (begin < end &&
           (pattern.data()[begin] == ' ' || pattern.data()[begin] == '\t'))
    {
        ++begin;
    }
    while (end > begin &&
           (pattern.data()[end - 1] == ' ' || pattern.data()[end - 1] == '\t'))
    {
        --end;
    }

    if (begin == end)
        return false;

    name->assign(pattern.data() + begin, end - begin);
    return true;
}

bool wildcard_prefix(const StringPiece &pattern, StringPiece *prefix)
{
    if (pattern.empty() || pattern.data()[pattern.size() - 1] != '*')
        return false;

    prefix->set(pattern.data(), pattern.size() - 1);
    return true;
}

} // namespace

RouteTableNode *RouteTableNode::find_or_create_child(const StringPiece &segment)
{
    auto child = children_.find(segment);
    if (child != children_.end())
        return child->second.get();

    const auto stored = child_keys_.insert(segment.as_string()).first;
    std::unique_ptr<RouteTableNode> node(new RouteTableNode);
    RouteTableNode *node_ptr = node.get();
    children_.emplace(StringPiece(*stored), std::move(node));
    return node_ptr;
}

VerbHandler &RouteTableNode::find_or_create(const StringPiece &route,
                                            size_t cursor)
{
    if (cursor >= route.size())
        return verb_handler_;

    if (cursor == 0 && route.size() == 1 && route.data()[0] == '/')
    {
        RouteTableNode *child = find_or_create_child(route);
        return child->find_or_create(route, 1);
    }

    if (route.data()[cursor] == '/')
        ++cursor;
    if (cursor >= route.size())
        return verb_handler_;

    const size_t anchor = cursor;
    while (cursor < route.size() && route.data()[cursor] != '/')
        ++cursor;

    const StringPiece segment(route.data() + anchor, cursor - anchor);
    RouteTableNode *child = find_or_create_child(segment);
    return child->find_or_create(route, cursor);
}

RouteTableNode::iterator RouteTableNode::find(
    const StringPiece &route,
    size_t cursor,
    std::map<std::string, std::string> &route_params,
    std::string &route_match_path) const
{
    const auto original_params = route_params;
    const std::string original_match_path = route_match_path;
    iterator result = find_impl(route, cursor, route_params, route_match_path);
    if (result == end())
    {
        route_params = original_params;
        route_match_path = original_match_path;
    }
    return result;
}

RouteTableNode::iterator RouteTableNode::find_impl(
    const StringPiece &route,
    size_t cursor,
    std::map<std::string, std::string> &route_params,
    std::string &route_match_path) const
{
    if (cursor > route.size())
        return end();

    if (cursor == route.size())
    {
        if (!verb_handler_.verb_handler_map.empty())
            return iterator{this, route, verb_handler_};

        const auto wildcard = children_.find(StringPiece("*"));
        if (wildcard != children_.end() &&
            !wildcard->second->verb_handler_.verb_handler_map.empty())
        {
            route_match_path.clear();
            return iterator{wildcard->second.get(), route,
                            wildcard->second->verb_handler_};
        }
        return end();
    }

    if (cursor == 0 && route.size() == 1 && route.data()[0] == '/')
    {
        const auto root = children_.find(route);
        if (root != children_.end())
        {
            iterator result = root->second->find_impl(
                route, 1, route_params, route_match_path);
            if (result != end())
                return result;
        }
    }

    if (route.data()[cursor] == '/')
        ++cursor;

    const size_t anchor = cursor;
    while (cursor < route.size() && route.data()[cursor] != '/')
        ++cursor;
    const StringPiece segment(route.data() + anchor, cursor - anchor);

    const auto exact = children_.find(segment);
    std::string exact_parameter;
    StringPiece exact_wildcard;
    if (exact != children_.end() &&
        !parameter_name(exact->first, &exact_parameter) &&
        !wildcard_prefix(exact->first, &exact_wildcard))
    {
        const auto saved_params = route_params;
        const std::string saved_match_path = route_match_path;
        iterator result = exact->second->find_impl(
            route, cursor, route_params, route_match_path);
        if (result != end())
            return result;
        route_params = saved_params;
        route_match_path = saved_match_path;
    }

    if (!segment.empty())
    {
        for (const auto &entry : children_)
        {
            std::string name;
            if (!parameter_name(entry.first, &name))
                continue;

            const auto previous = route_params.find(name);
            const bool had_previous = previous != route_params.end();
            const std::string previous_value = had_previous
                                                   ? previous->second
                                                   : std::string();
            const std::string saved_match_path = route_match_path;
            route_params[name] = segment.as_string();

            iterator result = entry.second->find_impl(
                route, cursor, route_params, route_match_path);
            if (result != end())
                return result;

            if (had_previous)
                route_params[name] = previous_value;
            else
                route_params.erase(name);
            route_match_path = saved_match_path;
        }
    }

    const RouteTableNode *best_node = nullptr;
    size_t best_prefix_size = 0;
    bool found_wildcard = false;
    for (const auto &entry : children_)
    {
        StringPiece prefix;
        if (!wildcard_prefix(entry.first, &prefix) ||
            !segment.starts_with(prefix) ||
            entry.second->verb_handler_.verb_handler_map.empty())
        {
            continue;
        }

        if (!found_wildcard || prefix.size() > best_prefix_size)
        {
            found_wildcard = true;
            best_prefix_size = prefix.size();
            best_node = entry.second.get();
        }
    }

    if (best_node != nullptr)
    {
        route_match_path = segment.as_string();
        route_match_path.append(route.data() + cursor, route.size() - cursor);
        return iterator{best_node, route, best_node->verb_handler_};
    }

    return end();
}

void RouteTableNode::print_node_arch()
{
    std::queue<std::pair<StringPiece, RouteTableNode *>> node_queue;
    const StringPiece root("/");
    node_queue.push({root, this});
    int level = 0;
    while (!node_queue.empty())
    {
        fprintf(stderr, "level %d:\t", level);
        const size_t queue_size = node_queue.size();
        fprintf(stderr, "(size : %zu)\t", queue_size);
        for (size_t i = 0; i < queue_size; ++i)
        {
            const auto node = node_queue.front();
            node_queue.pop();

            fprintf(stderr, "[%s :", node.first.as_string().c_str());
            const auto &children = node.second->children_;
            if (children.empty())
                fprintf(stderr, "\tNULL");
            for (const auto &child : children)
            {
                fprintf(stderr, "\t%s", child.first.as_string().c_str());
                node_queue.push({child.first, child.second.get()});
            }
            fprintf(stderr, "]");
        }
        ++level;
        fprintf(stderr, "\n");
    }
}

VerbHandler &RouteTable::find_or_create(const char *route)
{
    const auto stored = routes_.insert(route == nullptr ? "" : route).first;
    return root_.find_or_create(StringPiece(*stored), 0);
}
