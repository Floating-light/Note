#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>

#include <ranges>
// 
namespace Tranditional
{
std::vector<int> count_lines_in_files_command(const std::vector<std::string>& files)
{
    std::vector<int> results;
    char c = 0;
    for(const auto & file : files)
    {
        int line_count = 0 ;
        std::ifstream in(file);
        while(in.get(c))
        {
            if(c == '\n')
            {
                line_count++;
            }
        }
        results.push_back(line_count);
    }
    return results;
}
};

namespace UseCount
{
    int count_lines(const std::string& filename)
    {
        std::ifstream in(filename);

        return std::count(
            std::istreambuf_iterator<char>(in), 
            std::istreambuf_iterator<char>(), 
            '\n');
    }
    std::vector<int> count_lines_in_files_command(const std::vector<std::string>& files)
    {
        std::vector<int> results;
        for(const auto & file : files)
        {
            results.push_back(count_lines(file));
        }
        return results;
    }
};

namespace UseTransform
{
    std::vector<int> count_lines_in_files(const std::vector<std::string>& files)
    {
        std::vector<int> results(files.size());
        std::transform(files.cbegin(), files.cend(),results.begin(), UseCount::count_lines);
        return results;
    }
};
namespace UsePipeline
{
    std::vector<int> count_lines_in_files(const std::vector<std::string>& files)
    {
        auto res = files | std::views::transform(UseCount::count_lines) | std::ranges::to<std::vector<int>>();
        return res;
    }
}

int main() 
{
    std::vector<std::string> Files = {"FP.cpp", "FP.exe"};
    auto res = UsePipeline::count_lines_in_files(Files);
    for(const auto & It : res)
    {
        std::cout << "------>> " << It << std::endl;
    }
    return 0;
}