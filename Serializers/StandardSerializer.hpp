#pragma once

#include <ostream>
#include <istream>
#include <string>
#include <deque>
#include <vector>
#include <type_traits>

namespace TestTask
{
    template <typename T>
    void Serialize(T pod, std::ostream& out)
    {
        out.write(reinterpret_cast<const char*>(&pod), sizeof(pod));
    }

    void Serialize(const std::string& str, std::ostream& out)
    {
        size_t s = str.size();
        out.write(reinterpret_cast<const char*>(&s), sizeof(s));
        for (const auto& i : str) {
            out.write(reinterpret_cast<const char*>(&i), sizeof(i));
        }
    }

    template <typename T, typename U>
    void Serialize(const std::pair<T, U>& p, std::ostream& out)
    {
        Serialize(p.first, out);
        Serialize(p.second, out);
    }

    template <template <typename> typename Container, typename T>
    void Serialize(const Container<T>& data, std::ostream& out)
    {
        size_t s = data.size();
        out.write(reinterpret_cast<const char*>(&s), sizeof(s));
        for (const auto& i : data)
        {
            Serialize(i, out);
        }
    }

    template <typename T>
    void Deserialize(std::istream& in, T& pod) {
        in.read(reinterpret_cast<char*>(&pod), sizeof(pod));
    }

    void Deserialize(std::istream& in, std::string& str)
    {
        size_t sz;
        in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
        str.resize(sz);
        for (size_t i = 0; i != sz; ++i) {
            in.read(reinterpret_cast<char*>(&str[i]), sizeof(str[i]));
        }
    }

    template <typename T, typename U>
    void Deserialize(std::istream& in, std::pair<T, U>& p)
    {
        Deserialize(in, p.first);
        Deserialize(in, p.second);
    }

    template <template <typename> typename Container, typename T>
    void Deserialize(std::istream& in, Container<T>& data)
    {
        size_t sz;
        in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
        if constexpr (std::is_same_v<Container<T>, std::vector<T>> ||
                            std::is_same_v<Container<T>, std::deque<T>>)
        {
            data.resize(sz);
        }
        for (size_t i = 0; i != sz; ++i) {
            if constexpr (std::is_same_v<Container<T>, std::vector<T>> ||
                            std::is_same_v<Container<T>, std::deque<T>>)
            {
                Deserialize(in, data[i]);
            }
            else
            {
                T elem;
                Deserialize(in, elem);
                data.insert(elem);
            }
        }
    }
}
