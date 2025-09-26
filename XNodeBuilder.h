#pragma once

#include <string>
#include <map>
#include <vector>

namespace XProtocol
{
class MiniHeaderBuilder
{
public:
    MiniHeaderBuilder() = default;

    MiniHeaderBuilder &setLong(const std::string &path, long value)
    {
        std::string cleanPath = stripWhitespace(path);
        params_[cleanPath] = ParamValue{ParamType::Long, std::to_string(value)};
        return *this;
    }

    MiniHeaderBuilder &setDouble(const std::string &path, double value)
    {
        std::string cleanPath = stripWhitespace(path);
        params_[cleanPath] = ParamValue{ParamType::Double, std::to_string(value)};
        return *this;
    }

    MiniHeaderBuilder &setString(const std::string &path, const std::string &value)
    {
        std::string cleanPath = stripWhitespace(path);
        params_[cleanPath] = ParamValue{ParamType::String, "\"" + value + "\""};
        return *this;
    }

    MiniHeaderBuilder &setBool(const std::string &path, bool value)
    {
        std::string cleanPath = stripWhitespace(path);
        params_[cleanPath] = ParamValue{ParamType::Bool, value ? "\"true\"" : "\"false\""};
        return *this;
    }

    MiniHeaderBuilder &setStringArray(const std::string &path, const std::vector<std::string> &values)
    {
        std::string cleanPath = stripWhitespace(path);
        std::vector<std::string> formattedValues;
        for (const auto &val : values)
        {
            formattedValues.push_back("\"" + val + "\"");
        }
        std::string arrayContent = buildArrayContent(values.size(), "ParamString", formattedValues);
        params_[cleanPath] = ParamValue{ParamType::Array, arrayContent};
        return *this;
    }

    MiniHeaderBuilder &setDoubleArray(const std::string &path, const std::vector<double> &values)
    {
        std::string cleanPath = stripWhitespace(path);
        std::vector<std::string> formattedValues;
        for (const auto &val : values)
        {
            formattedValues.push_back(std::to_string(val));
        }
        std::string arrayContent = buildArrayContent(values.size(), "ParamDouble", formattedValues);
        params_[cleanPath] = ParamValue{ParamType::Array, arrayContent};
        return *this;
    }

    MiniHeaderBuilder &setLongArray(const std::string &path, const std::vector<long> &values)
    {
        std::string cleanPath = stripWhitespace(path);
        std::vector<std::string> formattedValues;
        for (const auto &val : values)
        {
            formattedValues.push_back(val == 0 ? "" : std::to_string(val));
        }
        std::string arrayContent = buildArrayContent(values.size(), "ParamLong", formattedValues);
        params_[cleanPath] = ParamValue{ParamType::Array, arrayContent};
        return *this;
    }

    std::string build() const
    {
        std::string result = "<XProtocol>\n{\n  <ParamMap.\"\">\n  {\n";

        // Group parameters by section
        std::map<std::string, std::map<std::string, ParamValue>> sections;
        for (const auto &[path, value] : params_)
        {
            size_t dot_pos = path.find('.');
            if (dot_pos != std::string::npos)
            {
                std::string section = path.substr(0, dot_pos);
                std::string param = path.substr(dot_pos + 1);
                sections[section][param] = value;
            }
            else
            {
                sections["DICOM"][path] = value;
            }
        }

        for (const auto &[section_name, section_params] : sections)
        {
            result += "    <ParamMap.\"" + section_name + "\">\n    {\n";
            for (const auto &[param_name, param_value] : section_params)
            {
                std::string type_str = getTypeString(param_value.type);
                if (param_value.type == ParamType::Array)
                {
                    result += "      <" + type_str + ".\"" + param_name + "\">" + param_value.value + "\n";
                }
                else
                {
                    result += "      <" + type_str + ".\"" + param_name + "\">{ " + param_value.value + " }\n";
                }
            }
            result += "    }\n";
        }

        result += "  }\n}";
        return result;
    }

private:
    enum class ParamType
    {
        Long,
        Double,
        String,
        Bool,
        Array
    };

    struct ParamValue
    {
        ParamType type;
        std::string value;
    };

    std::map<std::string, ParamValue> params_;

    std::string stripWhitespace(const std::string &str) const
    {
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(start, end - start + 1);
    }

    std::string buildArrayContent(size_t size, const std::string &paramType, const std::vector<std::string> &formattedValues) const
    {
        // Handle weird "<MaxSize>" logic
        // std::string maxSize = (size == 3) ? "3" : "2147483647";
        std::string maxSize = std::to_string(size);
        std::string arrayContent = "\n      {\n        <DefaultSize> " + std::to_string(size) +
                                   "\n        <MaxSize> " + maxSize +
                                   "\n        <Default> <" + paramType + ".\"\">{ }\n        ";
        for (const auto &val : formattedValues)
        {
            arrayContent += "{ " + val + " }";
        }
        arrayContent += "\n      }";
        return arrayContent;
    }

    std::string getTypeString(ParamType type) const
    {
        switch (type)
        {
        case ParamType::Long:
            return "ParamLong";
        case ParamType::Double:
            return "ParamDouble";
        case ParamType::String:
            return "ParamString";
        case ParamType::Bool:
            return "ParamBool";
        case ParamType::Array:
            return "ParamArray";
        default:
            return "ParamString";
        }
    }
};

} // namespace XProtocol