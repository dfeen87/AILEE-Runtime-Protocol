#pragma once

#include <cctype>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace nlohmann {

class json {
public:
    struct exception : public std::runtime_error {
        explicit exception(const std::string& message)
            : std::runtime_error(message) {}
    };

    using array_t = std::vector<json>;
    using object_t = std::map<std::string, json>;

    class iterator {
    public:
        using array_iterator = array_t::iterator;
        using object_iterator = object_t::iterator;

        iterator() : is_object_(false) {}
        explicit iterator(array_iterator it) : is_object_(false), array_it_(it) {}
        explicit iterator(object_iterator it) : is_object_(true), object_it_(it) {}

        json& operator*() const {
            return is_object_ ? object_it_->second : *array_it_;
        }

        iterator& operator++() {
            if (is_object_) {
                ++object_it_;
            } else {
                ++array_it_;
            }
            return *this;
        }

        bool operator!=(const iterator& other) const {
            if (is_object_ != other.is_object_) {
                return true;
            }
            return is_object_ ? (object_it_ != other.object_it_) : (array_it_ != other.array_it_);
        }

        const std::string& key() const {
            static const std::string kEmpty;
            return is_object_ ? object_it_->first : kEmpty;
        }

        json& value() const {
            return is_object_ ? object_it_->second : *array_it_;
        }

    private:
        bool is_object_{false};
        array_iterator array_it_{};
        object_iterator object_it_{};
    };

    class const_iterator {
    public:
        using array_iterator = array_t::const_iterator;
        using object_iterator = object_t::const_iterator;

        const_iterator() : is_object_(false) {}
        explicit const_iterator(array_iterator it) : is_object_(false), array_it_(it) {}
        explicit const_iterator(object_iterator it) : is_object_(true), object_it_(it) {}

        const json& operator*() const {
            return is_object_ ? object_it_->second : *array_it_;
        }

        const_iterator& operator++() {
            if (is_object_) {
                ++object_it_;
            } else {
                ++array_it_;
            }
            return *this;
        }

        bool operator!=(const const_iterator& other) const {
            if (is_object_ != other.is_object_) {
                return true;
            }
            return is_object_ ? (object_it_ != other.object_it_) : (array_it_ != other.array_it_);
        }

        const std::string& key() const {
            static const std::string kEmpty;
            return is_object_ ? object_it_->first : kEmpty;
        }

        const json& value() const {
            return is_object_ ? object_it_->second : *array_it_;
        }

    private:
        bool is_object_{false};
        array_iterator array_it_{};
        object_iterator object_it_{};
    };

    json() : data_(nullptr) {}
    json(std::nullptr_t) : data_(nullptr) {}
    json(bool value) : data_(value) {}
    json(int value) : data_(static_cast<double>(value)) {}
    json(size_t value) : data_(static_cast<double>(value)) {}
    json(double value) : data_(value) {}
    json(const char* value) : data_(std::string(value ? value : "")) {}
    json(std::string value) : data_(std::move(value)) {}
    json(object_t value) : data_(std::move(value)) {}
    json(array_t value) : data_(std::move(value)) {}

    json(std::initializer_list<std::pair<std::string, json>> init) : data_(object_t{}) {
        auto& obj = std::get<object_t>(data_);
        for (const auto& entry : init) {
            obj.emplace(entry.first, entry.second);
        }
    }

    static json array() {
        json j;
        j.data_ = array_t{};
        return j;
    }

    static json array(std::initializer_list<json> init) {
        json j;
        j.data_ = array_t{init};
        return j;
    }

    static json number_unsigned(uint64_t value) {
        // Note: stored as double; values > 2^53 lose precision (consistent with existing numeric constructors)
        return json(static_cast<double>(value));
    }

    bool is_object() const { return std::holds_alternative<object_t>(data_); }
    bool is_array() const { return std::holds_alternative<array_t>(data_); }
    bool is_string() const { return std::holds_alternative<std::string>(data_); }
    bool is_boolean() const { return std::holds_alternative<bool>(data_); }
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(data_); }
    bool is_number() const { return std::holds_alternative<double>(data_); }
    bool is_number_unsigned() const {
        if (auto val = std::get_if<double>(&data_)) {
            return *val >= 0.0 && std::floor(*val) == *val;
        }
        return false;
    }

    bool empty() const {
        if (auto val = std::get_if<object_t>(&data_)) return val->empty();
        if (auto val = std::get_if<array_t>(&data_)) return val->empty();
        if (auto val = std::get_if<std::string>(&data_)) return val->empty();
        return true;
    }

    bool contains(const std::string& key) const {
        if (!is_object()) return false;
        const auto& obj = std::get<object_t>(data_);
        return obj.find(key) != obj.end();
    }

    json& operator[](const std::string& key) {
        if (!is_object()) {
            data_ = object_t{};
        }
        auto& obj = std::get<object_t>(data_);
        return obj[key];
    }

    const json& operator[](const std::string& key) const {
        if (!is_object()) {
            return null_json();
        }
        const auto& obj = std::get<object_t>(data_);
        auto it = obj.find(key);
        return it == obj.end() ? null_json() : it->second;
    }

    json& operator[](size_t idx) {
        if (!is_array()) {
            data_ = array_t{};
        }
        auto& arr = std::get<array_t>(data_);
        if (idx >= arr.size()) {
            arr.resize(idx + 1);
        }
        return arr[idx];
    }

    const json& operator[](size_t idx) const {
        if (!is_array()) {
            return null_json();
        }
        const auto& arr = std::get<array_t>(data_);
        if (idx >= arr.size()) {
            return null_json();
        }
        return arr[idx];
    }

    template <typename T>
    T get() const {
        if constexpr (std::is_same_v<T, std::string>) {
            if (auto val = std::get_if<std::string>(&data_)) return *val;
            throw exception("json: value is not a string");
        } else if constexpr (std::is_same_v<T, bool>) {
            if (auto val = std::get_if<bool>(&data_)) return *val;
            throw exception("json: value is not a boolean");
        } else if constexpr (std::is_integral_v<T>) {
            if (auto val = std::get_if<double>(&data_)) return static_cast<T>(*val);
            throw exception("json: value is not a number");
        } else if constexpr (std::is_floating_point_v<T>) {
            if (auto val = std::get_if<double>(&data_)) return static_cast<T>(*val);
            throw exception("json: value is not a number");
        } else {
            static_assert(sizeof(T) == 0, "Unsupported json::get type");
        }
    }

    template <typename T>
    T value(const std::string& key, T default_value) const {
        if (!is_object()) return default_value;
        const auto& obj = std::get<object_t>(data_);
        auto it = obj.find(key);
        if (it == obj.end()) return default_value;
        return it->second.get<T>();
    }

    std::string value(const std::string& key, const char* default_value) const {
        if (!is_object()) return default_value ? std::string(default_value) : std::string();
        const auto& obj = std::get<object_t>(data_);
        auto it = obj.find(key);
        if (it == obj.end()) return default_value ? std::string(default_value) : std::string();
        return it->second.get<std::string>();
    }

    std::string dump() const {
        std::ostringstream oss;
        dump_impl(oss);
        return oss.str();
    }

    static json parse(const std::string& text) {
        Parser parser(text);
        json result = parser.parse_value();
        parser.skip_ws();
        if (!parser.at_end()) {
            throw exception("json: unexpected trailing characters");
        }
        return result;
    }

    iterator begin() {
        if (is_object()) {
            return iterator(std::get<object_t>(data_).begin());
        }
        if (is_array()) {
            return iterator(std::get<array_t>(data_).begin());
        }
        return iterator();
    }

    iterator end() {
        if (is_object()) {
            return iterator(std::get<object_t>(data_).end());
        }
        if (is_array()) {
            return iterator(std::get<array_t>(data_).end());
        }
        return iterator();
    }

    const_iterator begin() const {
        if (is_object()) {
            return const_iterator(std::get<object_t>(data_).begin());
        }
        if (is_array()) {
            return const_iterator(std::get<array_t>(data_).begin());
        }
        return const_iterator();
    }

    const_iterator end() const {
        if (is_object()) {
            return const_iterator(std::get<object_t>(data_).end());
        }
        if (is_array()) {
            return const_iterator(std::get<array_t>(data_).end());
        }
        return const_iterator();
    }

private:
    class Parser {
    public:
        explicit Parser(const std::string& input) : input_(input) {}

        json parse_value() {
            skip_ws();
            if (at_end()) throw exception("json: unexpected end");
            char c = peek();
            if (c == '{') return parse_object();
            if (c == '[') return parse_array();
            if (c == '"') return json(parse_string());
            if (c == 't' || c == 'f') return json(parse_bool());
            if (c == 'n') return parse_null();
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return json(parse_number());
            throw exception("json: invalid value");
        }

        void skip_ws() {
            while (!at_end() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }

        bool at_end() const { return pos_ >= input_.size(); }

    private:
        char peek() const { return input_[pos_]; }

        char get() {
            if (at_end()) throw exception("json: unexpected end");
            return input_[pos_++];
        }

        json parse_object() {
            json result(object_t{});
            get(); // '{'
            skip_ws();
            if (peek_if('}')) return result;
            while (true) {
                skip_ws();
                std::string key = parse_string();
                skip_ws();
                expect(':');
                json value = parse_value();
                result[key] = value;
                skip_ws();
                if (peek_if('}')) break;
                expect(',');
            }
            return result;
        }

        json parse_array() {
            json result = json::array_t{};
            get(); // '['
            skip_ws();
            if (peek_if(']')) return result;
            size_t index = 0;
            while (true) {
                json value = parse_value();
                result[index++] = value;
                skip_ws();
                if (peek_if(']')) break;
                expect(',');
            }
            return result;
        }

        std::string parse_string() {
            expect('"');
            std::ostringstream oss;
            while (!at_end()) {
                char c = get();
                if (c == '"') break;
                if (c == '\\') {
                    char esc = get();
                    switch (esc) {
                        case '"': oss << '"'; break;
                        case '\\': oss << '\\'; break;
                        case '/': oss << '/'; break;
                        case 'b': oss << '\b'; break;
                        case 'f': oss << '\f'; break;
                        case 'n': oss << '\n'; break;
                        case 'r': oss << '\r'; break;
                        case 't': oss << '\t'; break;
                        default:
                            throw exception("json: invalid escape");
                    }
                } else {
                    oss << c;
                }
            }
            return oss.str();
        }

        bool parse_bool() {
            if (input_.compare(pos_, 4, "true") == 0) {
                pos_ += 4;
                return true;
            }
            if (input_.compare(pos_, 5, "false") == 0) {
                pos_ += 5;
                return false;
            }
            throw exception("json: invalid boolean");
        }

        json parse_null() {
            if (input_.compare(pos_, 4, "null") == 0) {
                pos_ += 4;
                return json(nullptr);
            }
            throw exception("json: invalid null");
        }

        double parse_number() {
            size_t start = pos_;
            if (peek() == '-') ++pos_;
            while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos_;
            if (!at_end() && peek() == '.') {
                ++pos_;
                while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos_;
            }
            if (!at_end() && (peek() == 'e' || peek() == 'E')) {
                ++pos_;
                if (!at_end() && (peek() == '+' || peek() == '-')) ++pos_;
                while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos_;
            }
            return std::stod(input_.substr(start, pos_ - start));
        }

        bool peek_if(char expected) {
            if (!at_end() && peek() == expected) {
                ++pos_;
                return true;
            }
            return false;
        }

        void expect(char expected) {
            if (at_end() || get() != expected) {
                throw exception("json: expected character");
            }
        }

        const std::string& input_;
        size_t pos_{0};
    };

    void dump_impl(std::ostringstream& oss) const {
        if (std::holds_alternative<std::nullptr_t>(data_)) {
            oss << "null";
        } else if (auto val = std::get_if<bool>(&data_)) {
            oss << (*val ? "true" : "false");
        } else if (auto val = std::get_if<double>(&data_)) {
            oss << *val;
        } else if (auto val = std::get_if<std::string>(&data_)) {
            oss << '"';
            for (char c : *val) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c; break;
                }
            }
            oss << '"';
        } else if (auto val = std::get_if<array_t>(&data_)) {
            oss << '[';
            for (size_t i = 0; i < val->size(); ++i) {
                if (i > 0) oss << ',';
                (*val)[i].dump_impl(oss);
            }
            oss << ']';
        } else if (auto val = std::get_if<object_t>(&data_)) {
            oss << '{';
            bool first = true;
            for (const auto& [key, value] : *val) {
                if (!first) oss << ',';
                first = false;
                oss << '"' << key << "\":";
                value.dump_impl(oss);
            }
            oss << '}';
        }
    }

    static const json& null_json() {
        static const json null_value;
        return null_value;
    }

public:
    void push_back(const json& j) {
        if (!is_array()) {
            data_ = array_t{};
        }
        std::get<array_t>(data_).push_back(j);
    }

    void push_back(json&& j) {
        if (!is_array()) {
            data_ = array_t{};
        }
        std::get<array_t>(data_).push_back(std::move(j));
    }
private:
    std::variant<std::nullptr_t, bool, double, std::string, array_t, object_t> data_;
};

} // namespace nlohmann
