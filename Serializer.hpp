#pragma once

namespace TestTask
{
    template <typename T>
    void Serialize(T pod, std::ostream& out) {
        out.write(reinterpret_cast<const char*>(&pod), sizeof(pod));
    }

    void Serialize(const std::string& str, std::ostream& out) {
        size_t s = str.size();
        out.write(reinterpret_cast<const char*>(&s), sizeof(s));
        for (const auto& i : str) {
            out.write(reinterpret_cast<const char*>(&i), sizeof(i));
        }
    }

    template <typename T>
    void Serialize(const std::vector<T>& data, std::ostream& out);

    template <typename T1, typename T2>
    void Serialize(const std::map<T1, T2>& data, std::ostream& out) {
        size_t s = data.size();
        out.write(reinterpret_cast<const char*>(&s), sizeof(s));
        for (const auto& i : data) {
            Serialize(i.first, out);
            Serialize(i.second, out);
        }
    }

    template <typename T>
    void Serialize(const std::vector<T>& data, std::ostream& out) {
        size_t s = data.size();
        out.write(reinterpret_cast<const char*>(&s), sizeof(s));
        for (const auto& i : data) {
            Serialize(i, out);
        }
    }

    template <typename T>
    void Deserialize(std::istream& in, T& pod) {
        in.read(reinterpret_cast<char*>(&pod), sizeof(pod));
    }

    void Deserialize(std::istream& in, std::string& str) {
        size_t sz;
        in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
        str.resize(sz);
        for (size_t i = 0; i != sz; ++i) {
            in.read(reinterpret_cast<char*>(&str[i]), sizeof(str[i]));
        }
    }

    template <typename T>
    void Deserialize(std::istream& in, std::vector<T>& data);

    template <typename T1, typename T2>
    void Deserialize(std::istream& in, std::map<T1, T2>& data) {
        size_t sz;
        data.clear();
        in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
        for (size_t i = 0; i != sz; ++i) {
            T1 tmp;
            Deserialize(in, tmp);
            Deserialize(in, data[tmp]);
        }
    }

    template <typename T>
    void Deserialize(std::istream& in, std::vector<T>& data) {
        size_t sz;
        in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
        data.resize(sz);
        for (size_t i = 0; i != sz; ++i) {
            Deserialize(in, data[i]);
        }
    }

}