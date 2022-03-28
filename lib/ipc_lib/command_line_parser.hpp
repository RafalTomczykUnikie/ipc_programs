#pragma once
#include <vector>
#include <string>
#include <map>

struct Option
{
    std::string option_short;
    std::string option_long;
    std::string option_description;
    bool option_is_switch_only;

    Option(std::string a, std::string b, std::string c, bool d) :
        option_short(a),
        option_long(b),
        option_description(c),
        option_is_switch_only(d)
    {
        
    };

private:
    bool is_found = false;
    bool arg_error = false;
    std::string option_value;

    friend class CommandLineParser;
};

class CommandLineParser
{
private:
    std::string m_program_name;
    std::vector<Option> m_program_options;
    std::vector<std::string> m_arguments;
    std::map<std::string, std::string> m_found_args;
    bool m_args_error = false;

public:

    void addOption(Option op);
    void parseOptions(int argc, char *argv[]);
    void printHelp(void);
    bool isErrorFound(void);
    bool isOptionFound(std::string option);
    std::string getOptionValue(std::string option);

    CommandLineParser(const char *prog_name);
    ~CommandLineParser();
};

