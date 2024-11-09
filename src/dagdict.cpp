#include <algorithm>

#include "dagdict.hpp"

DAGDict::DAGDict() {}

DAGDict::DAGDict(
    std::vector<std::pair<std::string, std::vector<std::string>>> codes) {
    while (codes.size()) {
        auto code = std::move(codes.back());
        codes.pop_back();
        insert(code.first, std::move(code.second));
    }
}

void DAGDict::insert(
    const std::string& key,
    std::vector<std::string>&& elements) {
    auto dag = this;
    for (auto c : key) {
        if (not dag->graph.contains(c)) {
            dag->graph[c] = new DAGDict();
        }
        dag = dag->graph[c];
    }
    dag->elements = elements;
}

std::vector<std::string> DAGDict::get_all(const std::string& prefix) const {
    if (prefix.empty()) {
        return {};
    }

    auto dag = (DAGDict*)this;
    for (auto c : prefix) {
        if (not dag->graph.contains(c)) {
            return {};
        }
        dag = dag->graph[c];
    }
    auto out = dag->child_elements(prefix);

    std::sort(out.begin(), out.end());
    auto last = std::unique(out.begin(), out.end());
    out.erase(last, out.end());

    return out;
}

std::vector<std::string> DAGDict::child_elements(
    const std::string& prefix) const {
    std::vector<std::string> out;
    out.insert(
        out.end(),
        std::make_move_iterator(elements.begin()),
        std::make_move_iterator(elements.end()));
    for (auto [key, val] : graph) {
        std::string new_prefix = prefix + key;
        auto r = val->child_elements(new_prefix);
        out.insert(
            out.end(),
            std::make_move_iterator(r.begin()),
            std::make_move_iterator(r.end()));
        r.clear();
    }
    return out;
}

std::vector<std::string> DAGDict::possible_keys(
    const std::string& prefix) const {
    if (prefix.empty()) {
        return {};
    }

    auto dag = (DAGDict*)this;
    for (auto c : prefix) {
        if (not dag->graph.contains(c)) {
            return {};
        }
        dag = dag->graph[c];
    }
    return dag->child_keys(prefix);
}

std::vector<std::string> DAGDict::child_keys(const std::string& prefix) const {
    std::vector<std::string> out;
    if (elements.size()) {
        out.push_back(prefix);
    }
    for (auto [key, val] : graph) {
        std::string new_prefix = prefix + key;
        auto r = val->child_keys(new_prefix);
        out.insert(
            out.end(),
            std::make_move_iterator(r.begin()),
            std::make_move_iterator(r.end()));
        r.clear();
    }
    return out;
}
