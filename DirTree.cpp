#include "DirTree.hpp"

using namespace TestTask;

void Dir::Clear()
{
    for (auto* dir : dirs_)
    {
        dir->Clear();   
    }
    delete this;
}

void Dir::AddFile(File* f)
{
    files_.push_back(f);
}

Dir* Dir::AddDir(const std::string& name)
{
    dirs_.push_back(new Dir(name));
    return dirs_.back();
}

Dir* Dir::FindDir(const std::string& name)
{
    for (auto* dir : dirs_)
    {
        if (dir->name_ == name)
        {
            return dir;
        }
    }
    return nullptr;
}
