#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

class INIReader
{
private:
    std::map<std::string, std::map<std::string, std::string>> data;
    std::string filename;
    bool isValid;

    // Helper function to trim whitespace from string
    static std::string trim(const std::string& str)
    {
        const std::string whitespace = " \t\n\r";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    // Helper function to check if line is a comment
    static bool isComment(const std::string& line)
    {
        std::string trimmed = trim(line);
        return trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#';
    }

    // Helper function to check if line contains a section
    static bool isSection(const std::string& line, std::string& sectionName)
    {
        std::string trimmed = trim(line);
        if (trimmed.size() > 2 && trimmed[0] == '[' && trimmed.back() == ']')
        {
            sectionName = trimmed.substr(1, trimmed.size() - 2);
            return true;
        }
        return false;
    }

    // Helper function to parse key=value line
    static bool parseKeyValue(const std::string& line, std::string& key, std::string& value)
    {
        size_t pos = line.find('=');
        if (pos == std::string::npos) return false;
        
        key = trim(line.substr(0, pos));
        value = trim(line.substr(pos + 1));
        
        // Remove inline comments
        size_t commentPos = value.find(';');
        if (commentPos != std::string::npos)
        {
            value = trim(value.substr(0, commentPos));
        }
        commentPos = value.find('#');
        if (commentPos != std::string::npos)
        {
            value = trim(value.substr(0, commentPos));
        }
        
        return !key.empty();
    }

public:
    // Constructor
    INIReader(const std::string& filename) : filename(filename), isValid(false)
    {
        parseFile();
    }

    // Parse INI file
    void parseFile()
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error: Could not open INI file: " << filename << std::endl;
            return;
        }

        std::string line;
        std::string currentSection;
        
        while (std::getline(file, line))
        {
            // Skip comments and empty lines
            if (isComment(line)) continue;
            
            // Check for section
            std::string sectionName;
            if (isSection(line, sectionName))
            {
                currentSection = sectionName;
                continue;
            }
            
            // Parse key=value pairs
            std::string key, value;
            if (parseKeyValue(line, key, value))
            {
                if (!currentSection.empty())
                {
                    data[currentSection][key] = value;
                }
            }
        }
        
        file.close();
        isValid = true;
    }

    // Get string value from INI file
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const
    {
        if (!isValid) return defaultValue;
        
        auto sectionIt = data.find(section);
        if (sectionIt != data.end())
        {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end())
            {
                return keyIt->second;
            }
        }
        return defaultValue;
    }

    // Get integer value from INI file
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const
    {
        std::string value = getString(section, key);
        if (value.empty()) return defaultValue;
        
        try
        {
            return std::stoi(value);
        }
        catch (const std::exception&)
        {
            return defaultValue;
        }
    }

    // Get double value from INI file
    double getDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const
    {
        std::string value = getString(section, key);
        if (value.empty()) return defaultValue;
        
        try
        {
            return std::stod(value);
        }
        catch (const std::exception&)
        {
            return defaultValue;
        }
    }

    // Get boolean value from INI file
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const
    {
        std::string value = getString(section, key);
        if (value.empty()) return defaultValue;
        
        // Convert to lowercase for comparison
        for (char& c : value) c = std::tolower(c);
        
        return (value == "true" || value == "1" || value == "yes" || value == "on");
    }

    // Check if section exists
    bool hasSection(const std::string& section) const
    {
        return isValid && data.find(section) != data.end();
    }

    // Check if key exists in section
    bool hasKey(const std::string& section, const std::string& key) const
    {
        if (!isValid) return false;
        
        auto sectionIt = data.find(section);
        if (sectionIt != data.end())
        {
            return sectionIt->second.find(key) != sectionIt->second.end();
        }
        return false;
    }

    // Get all sections
    std::vector<std::string> getSections() const
    {
        std::vector<std::string> sections;
        if (!isValid) return sections;
        
        for (const auto& section : data)
        {
            sections.push_back(section.first);
        }
        return sections;
    }

    // Get all keys in a section
    std::vector<std::string> getKeys(const std::string& section) const
    {
        std::vector<std::string> keys;
        if (!isValid) return keys;
        
        auto sectionIt = data.find(section);
        if (sectionIt != data.end())
        {
            for (const auto& key : sectionIt->second)
            {
                keys.push_back(key.first);
            }
        }
        return keys;
    }

    // Check if file was loaded successfully
    bool isValidFile() const
    {
        return isValid;
    }

    // Print all loaded data (for debugging)
    void printAll() const
    {
        if (!isValid)
        {
            std::cout << "INI file is not valid or not loaded." << std::endl;
            return;
        }
        
        std::cout << "INI File: " << filename << std::endl;
        std::cout << "Loaded sections and keys:" << std::endl;
        
        for (const auto& section : data)
        {
            std::cout << "[" << section.first << "]" << std::endl;
            for (const auto& key : section.second)
            {
                std::cout << "  " << key.first << " = " << key.second << std::endl;
            }
            std::cout << std::endl;
        }
    }
};