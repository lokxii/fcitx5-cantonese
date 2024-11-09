#include <algorithm>
#include <fstream>
#include <ranges>
#include <string_view>

#include "utf8/utf8.h"

#include "ime.hpp"

namespace rv = std::views;
namespace ranges = std::ranges;

const std::vector<std::string> initials = {
    "a", "e", "i",  "o", "u",  "b",  "p", "m", "f",  "d", "t", "n", "l",
    "g", "k", "ng", "h", "gw", "kw", "w", "j", "ch", "s", "y", "*",
};

const std::vector<std::string> finals = {
    "a",   "ai",   "aai", "au",  "aau", "am",  "aam", "an",  "aan",
    "ang", "aang", "ap",  "aap", "at",  "aat", "ak",  "aak", "e",
    "ei",  "eng",  "ek",  "i",   "iu",  "im",  "in",  "ing", "ip",
    "it",  "ik",   "oh",  "oi",  "o",   "on",  "ong", "ot",  "ok",
    "oo",  "ooi",  "oon", "oot", "ui",  "un",  "ung", "ut",  "uk",
    "euh", "eung", "euk", "ue",  "uen", "uet", "m",   "ng",
};

DAGDict codes_from_file(std::ifstream&& code_f) {
    std::string line;
    std::vector<std::pair<std::string, std::vector<std::string>>> codes;

    while (std::getline(code_f, line, ',')) {
        auto it = std::find(line.cbegin(), line.cend(), ' ');
        auto key = std::string(line.cbegin(), it);
        it++;

        std::vector<std::string> chars;
        while (std::distance(line.cbegin(), it) < line.length()) {
            std::string c;
            utf8::append(utf8::next(it, line.cend()), std::back_inserter(c));
            chars.push_back(c);
            it++;
        }
        codes.push_back(std::make_pair(key, chars));
    }
    return DAGDict(codes);
}

// template <typename T>
// std::map<std::string, std::string, T> expand_rules_from_file(
//     std::ifstream&& expand_rules_f) {
//     std::map<std::string, std::string, T> out;
//     std::string line;
//     while (std::getline(expand_rules_f, line, ',')) {
//         std::string alternative, fix;
//         std::stringstream ss(line);
//         ss >> fix >> alternative;
//         out.insert({fix, alternative});
//     }
//     return out;
// }

std::unordered_map<std::string, std::vector<std::string>> vocab_from_file(
    std::ifstream&& words_f) {
    std::unordered_map<std::string, std::vector<std::string>> out;
    std::string line;
    while (std::getline(words_f, line, ',')) {
        auto it = line.cbegin();
        std::string prefix;
        utf8::append(utf8::next(it, line.cend()), std::back_inserter(prefix));

        std::vector<std::string> candidates;
        std::string word;
        while (std::distance(line.cbegin(), it) < line.length()) {
            std::string c;
            utf8::append(utf8::next(it, line.cend()), std::back_inserter(c));
            if (c == " ") {
                candidates.push_back(word);
                word.clear();
            } else {
                word += c;
            }
        }
        candidates.push_back(word);

        auto tagged_candidates =
            candidates | rv::transform([](const auto& c) {
                auto s = std::string(c.cend() - 1, c.cend());
                int order = std::stoi(s);
                std::string can = std::string(c.cbegin(), c.cend() - 1);
                return std::make_pair(can, order);
            }) |
            ranges::to<std::vector>();
        std::stable_sort(
            tagged_candidates.begin(),
            tagged_candidates.end(),
            [](const auto& l, const auto& r) { return l.second < r.second; });
        out[prefix] =
            tagged_candidates |
            rv::transform([](auto&& p) { return std::move(p.first); }) |
            ranges::to<std::vector>();
    }
    return out;
}

IME::IME(std::filesystem::path path_to_data) {
    auto code_f = std::ifstream(path_to_data / "codecantonese.csv");
    this->codes = codes_from_file(std::move(code_f));

    auto expand_rules_f = std::ifstream(path_to_data / "codeexpand.csv");
    // this->expand_rules =
    //     expand_rules_from_file<ExpandRuleLess>(std::move(expand_rules_f));

    auto words_f = std::ifstream(path_to_data / "chiwordscat.csv");
    this->vocab = vocab_from_file(std::move(words_f));
}

std::vector<std::string> IME::split_words(const std::string& input) const {
    std::vector<std::string> out;
    auto it = input.cbegin();
    while (std::distance(input.cbegin(), it) < input.length()) {
        auto prefix = std::string(1, *it);
        auto pos = std::find(initials.cbegin(), initials.cend(), prefix);
        if (pos == initials.cend()) {
            it += 1;
            out.push_back(prefix);
            continue;
        }

        auto possible_keys = codes.possible_keys(prefix);
        std::stable_sort(
            possible_keys.begin(),
            possible_keys.end(),
            [](const std::string& l, const std::string& r) {
                return l.length() > r.length();
            });

        auto view = std::string_view(it, input.end());
        auto word_it = std::find_if(
            possible_keys.cbegin(),
            possible_keys.cend(),
            [&](const std::string& key) { return view.starts_with(key); });
        if (word_it != possible_keys.cend()) {
            out.push_back(*word_it);
            it += (*word_it).length();
            continue;
        }
        out.push_back(prefix);
        it += 1;
    }
    return out;
}

struct flatten : ranges::range_adaptor_closure<flatten> {
    flatten() {}
    constexpr std::vector<std::string> operator()(
        const ranges::range auto&& r) {
        std::vector<std::string> out;
        for (auto&& [key, l] : r) {
            out.push_back(std::move(key));
            out.insert(
                out.end(),
                std::make_move_iterator(l.cbegin()),
                std::make_move_iterator(l.cend()));
        }
        return out;
    }
};

std::vector<std::string> IME::candidates(const std::string& input) {
    // TODO: replace wrong input... HOW?
    auto words = split_words(input);
    if (words.empty()) {
        return {};
    }

    auto candidates =
        codes.get_all(words[0]) | rv::transform([&](const std::string& key) {
            return std::make_pair(
                key,
                (vocab.contains(key) ? vocab[key]
                                     : std::vector<std::string>()) |
                    rv::transform([&](const std::string& candidate) {
                        return (key == "*" ? "" : key) + candidate;
                    }));
        }) |
        flatten();
    for (int i = 1; i < words.size(); i++) {
        auto sub_candidate = codes.get_all(words[i]);
        candidates =
            candidates | rv::filter([&](const auto& c) {
                return utf8::distance(c.cbegin(), c.cend()) > i;
            }) |
            rv::filter([&](const auto& c) {
                auto s = c.cbegin();
                utf8::advance(s, i, c.cend());
                auto e = s;
                utf8::next(e, c.cend());
                return std::find(
                           sub_candidate.cbegin(),
                           sub_candidate.cend(),
                           std::string_view(s, e)) != sub_candidate.cend();
            }) |
            ranges::to<std::vector>();
    }
    return candidates;
}
