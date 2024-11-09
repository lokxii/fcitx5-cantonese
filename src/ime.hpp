#include <filesystem>
#include <string>
#include <unordered_map>

#include "dagdict.hpp"

class IME {
   public:
    IME(std::filesystem::path path_to_data);
    std::vector<std::string> candidates(const std::string& input);

   private:
    std::vector<std::string> split_words(const std::string& input) const;
    DAGDict codes;
    std::unordered_map<std::string, std::vector<std::string>> vocab;

    // struct ExpandRuleLess {
    //     bool operator()(const std::string& left, const std::string& right)
    //         const {
    //         return left.length() > right.length();
    //     }
    // };
    // std::map<std::string, std::string, ExpandRuleLess> expand_rules;
};
