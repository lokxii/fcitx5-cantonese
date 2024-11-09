#include <map>
#include <string>
#include <vector>

class DAGDict {
   public:
    DAGDict();
    // TODO: write destructor
    DAGDict(
        std::vector<std::pair<std::string, std::vector<std::string>>> codes);

    void insert(const std::string& key, std::vector<std::string>&& elements);

    std::vector<std::string> get_all(const std::string& prefix) const;

    std::vector<std::string> possible_keys(const std::string& prefix) const;

   private:
    std::map<char, DAGDict*> graph;
    std::vector<std::string> elements;

    std::vector<std::string> child_elements(const std::string& prefix) const;

    std::vector<std::string> child_keys(const std::string& prefix) const;
};
