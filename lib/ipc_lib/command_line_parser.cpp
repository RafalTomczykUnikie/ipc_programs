#include <iostream>
#include <glog/logging.h>
#include "command_line_parser.hpp"

CommandLineParser::CommandLineParser(const char *prog_name) :
    m_program_name(prog_name)
{
    auto pos = m_program_name.find_last_of('/');
    m_program_name = m_program_name.substr(pos + 1);
}

CommandLineParser::~CommandLineParser()
{

}

void CommandLineParser::addOption(Option op)
{
    m_program_options.push_back(op);
}

void CommandLineParser::parseOptions(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        const char * str = argv[i];
        if(str[0] == '-' && str[1] != '-')
        {
            auto size = strlen(str);
            for(auto x = 1u; x < size; x++)
            {
                std::string arg = "-";
                arg += str[x];
                m_arguments.push_back(arg);
            }
        }
        else
        {
            m_arguments.push_back(argv[i]);
        }
    }

    for(auto idx = 0u; idx < m_arguments.size(); ++idx)
    {
        auto max_idx = m_arguments.size() - 1;
        auto & arg = m_arguments[idx];
         
        for(auto &op : m_program_options)
        {
            if(arg == op.option_long || arg == op.option_short)
            {
                if(op.option_is_switch_only != true &&
                   idx < max_idx)
                {
                    auto & arg2 = m_arguments[idx+1];
                    
                    for(auto op1 : m_program_options)
                    {
                        if(op1.option_long == arg2 || op1.option_short == arg2)
                        {
                            op.arg_error = true;
                        }
                    }
                    
                    if(!op.arg_error)
                    {
                        op.is_found = true;
                        op.option_value = arg2;
                    }
                }
                else if(op.option_is_switch_only == true)
                {
                    op.arg_error = false;
                    op.is_found = true;
                }
                else
                {
                    op.arg_error = true;
                }

                if(op.arg_error == true)
                {
                    m_args_error = true;
                    break;
                }
            }
        }

        if(!m_args_error)
        {
            for(auto &op : m_program_options)
            {
                if(op.is_found)
                {
                    if(op.option_is_switch_only)
                        op.option_value = "true";
                    m_found_args.insert({op.option_short, op.option_value});
                }
            }
        }
    }
}

bool CommandLineParser::isErrorFound(void)
{
    return m_args_error;
}

bool CommandLineParser::isOptionFound(std::string option)
{
    auto dupa = m_found_args.find(option);

    if(dupa != m_found_args.end())
    {
        return true;
    }

    return false;
}

std::string CommandLineParser::getOptionValue(std::string option)
{
    auto dupa = m_found_args[option];
    return dupa;
}

void CommandLineParser::printHelp(void) 
{
    std::cout << "______________________________________________________________\r\n\r\n";
    std::cout << "Welcome to: " << m_program_name << "\r\n\r\n";
    std::cout << "This program usage is printed below:\r\n\r\n";

    for(auto op : m_program_options)
    {
        std::string param_switch = "";
        
        if(op.option_param != "")
        {
            param_switch += " [" + op.option_param + "]";
        }

        std::cout << "    " << op.option_short << ", " << op.option_long << param_switch << " > " << op.option_description << "\r\n";
    }
    std::cout << "______________________________________________________________\r\n\r\n";
}