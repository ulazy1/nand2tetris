// positive return values for information
#define NO_JUMP 7
#define NO_DESTINATION 6
#define COMP_HAS_M 5
#define C_INSTRUCTION 4
#define A_INSTRUCTION 3
#define LABEL_LINE 2
#define EMPTY_LINE 1
#define SUCCESS 0
// negative return values for errors
#define INVALID_NUMBER_OF_ARGUMENTS -1
#define COULD_NOT_OPEN_FILE_READ -2
#define COULD_NOT_OPEN_FILE_WRITE -3
#define UNDEFINED_REFERENCE -4
#define UNRESOLVED_COMP_SYMBOL -5
#define UNRESOLVED_JUMP_SYMBOL -6

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <cctype>
#include <regex>
#include <bitset>
#include <cstdlib>

int ram_index = 16;
std::unordered_map<std::string, int> variable_table;
std::unordered_map<std::string, int> label_table;
std::unordered_map<std::string, std::string> comp_map({
    {"0", "101010"},
    {"1", "111111"},
    {"-1", "111010"},
    {"D", "001100"},
    {"A", "110000"},
    {"!D", "001101"},
    {"!A", "110001"},
    {"-D", "001111"},
    {"-A", "110011"},
    {"D+1", "011111"},
    {"A+1", "110111"},
    {"D-1", "001110"},
    {"A-1", "110010"},
    {"D+A", "000010"},
    {"D-A", "010011"},
    {"A-D", "000111"},
    {"D&A", "000000"},
    {"D|A", "010101"},
});
std::unordered_map<std::string, std::string> dest_map({
    {"", "000"},
    {"M", "001"},
    {"D", "010"},
    {"MD", "011"},
    {"A", "100"},
    {"AM", "101"},
    {"AD", "110"},
    {"AMD", "111"},
});
std::unordered_map<std::string, std::string> jump_map({
    {"", "000"},
    {"JGT", "001"},
    {"JEQ", "010"},
    {"JGE", "011"},
    {"JLT", "100"},
    {"JNE", "101"},
    {"JLE", "110"},
    {"JMP", "111"},
});
std::unordered_map<std::string, std::string> special_symbols_map({
    {"R0", "0"},
    {"R1", "1"},
    {"R2", "2"},
    {"R3", "3"},
    {"R4", "4"},
    {"R5", "5"},
    {"R6", "6"},
    {"R7", "7"},
    {"R8", "8"},
    {"R9", "9"},
    {"R10", "10"},
    {"R11", "11"},
    {"R12", "12"},
    {"R13", "13"},
    {"R14", "14"},
    {"R15", "15"},
    {"SCREEN", "16384"},
    {"KBD", "24576"},
    {"SP", "0"},
    {"LCL", "1"},
    {"ARG", "2"},
    {"THIS", "3"},
    {"THAT", "4"},
});

std::regex label_regex(R"(\(([a-zA-z][a-zA-Z0-9_.$]*)\))");
std::regex variable_regex(R"(@([a-zA-z][a-zA-Z0-9_.$]*))");
std::regex integer_regex(R"(@(-?[0-9]*))");

void print_label_table()
{
    for (auto &&a : label_table)
    {
        std::cout << "Label Table: " << a.first << ", " << a.second << '\n';
    }
}

void print_variable_table()
{
    for (auto &&a : variable_table)
    {
        std::cout << a.first << ", " << a.second << '\n';
    }
}

void insert_variable_table(std::string variable)
{
    variable_table.insert(std::make_pair(variable, ram_index));
    ++ram_index;
}

int encode_a_instruction(std::string &line, int x)
{
    line = '0' + std::bitset<15>(x).to_string();
    return SUCCESS;
}

int strip_white_space(std::string &line)
{
    // remove comments
    std::regex r(R"(//)");
    std::smatch match;
    if (std::regex_search(line, match, r))
    {
        line = line.substr(0, match.position());
    }

    // remove white space
    std::string::iterator end_pos = std::remove_if(line.begin(), line.end(), ::isspace);
    line.erase(end_pos, line.end()); // remove white space;
    if (line.length() == 0)
    {
        return EMPTY_LINE;
    }

    return SUCCESS;
}

int get_label(std::string &line, std::string &label)
{
    std::smatch matches;
    if (std::regex_search(line, matches, label_regex))
    {
        label = matches.str(1);
        return LABEL_LINE;
    }
    return SUCCESS;
}

int replace_A_with_M(std::string &line)
{
    int status = SUCCESS;
    std::regex r("M");
    for (std::sregex_iterator i = std::sregex_iterator(line.begin(), line.end(), r); i != std::sregex_iterator(); ++i)
    {
        status = COMP_HAS_M;
        std::smatch m = *i;
        line.replace(m.position(), 1, "A");
    }
    return status;
}

int process_a_instruction(std::string &line)
{
    std::string variable;
    std::smatch matches;
    std::string integer;

    std::regex_search(line, matches, variable_regex);
    variable = matches.str(1);
    std::regex_search(line, matches, integer_regex);
    integer = matches.str(1);
    if (label_table.count(variable) > 0)
    {
        return encode_a_instruction(line, label_table[variable]);
    }
    else if (integer.size() > 0)
    {
        return encode_a_instruction(line, atoi(integer.c_str()));
    }
    else if (variable_table.count(variable) > 0)
    {
        return encode_a_instruction(line, variable_table[variable]);
    }
    else
    {
        insert_variable_table(variable);
        return encode_a_instruction(line, variable_table[variable]);
    }
    return A_INSTRUCTION;
}

int process_comp(std::string &line, std::string &comp)
{
    int status;
    size_t idx;
    std::smatch matches;
    std::string integer;
    std::string a_flag;

    // remove `comp` part from line
    idx = line.find(';');
    idx = (idx == std::string::npos) ? line.size() : idx;
    comp = line.substr(0, idx);
    if (idx + 1 < line.size())
    {
        line = line.substr(idx + 1, line.size() - 1);
    }
    else
    {
        line = "";
    }

    status = replace_A_with_M(comp);
    a_flag = (status == COMP_HAS_M) ? '1' : '0';

    if (comp_map.count(comp) > 0)
    {
        comp = a_flag + comp_map[comp];
        return SUCCESS;
    }
    else
    {
        return UNRESOLVED_COMP_SYMBOL;
    }
}

int process_dest(std::string &line, std::string &dest)
{
    int idx;
    // remove `dest` part from line
    idx = line.find("=");
    idx = (idx == std::string::npos) ? line.size() : idx;
    dest = line.substr(0, idx);
    idx = (idx + 1 <= line.size() - 1) ? idx + 1 : line.size() - 1;
    line = line.substr(idx, line.size() - 1);

    if (dest_map.count(dest) > 0)
    {
        dest = dest_map[dest];
        return SUCCESS;
    }
    else
    {
        line = dest;
        dest = "000";
        return NO_DESTINATION;
    }
}

int process_jump(std::string &line, std::string &jump)
{
    jump = line;
    line = "";

    if (jump_map.count(jump) > 0)
    {
        jump = jump_map[jump];
        return SUCCESS;
    }
    else if (jump == "")
    {
        return NO_JUMP;
    }
    else
    {
        return UNRESOLVED_JUMP_SYMBOL;
    }
}

int process_c_instruction(std::string &line)
{
    int status;
    // keep a copy of the line to give as an error message
    std::string line_copy{line};

    std::string dest;
    process_dest(line, dest);

    std::string comp;
    status = process_comp(line, comp);
    if (status == UNRESOLVED_COMP_SYMBOL)
    {
        line = line_copy;
        return UNRESOLVED_COMP_SYMBOL;
    }

    std::string jump;
    status = process_jump(line, jump);
    if (status == UNRESOLVED_JUMP_SYMBOL)
    {
        line = line_copy;
        return UNRESOLVED_JUMP_SYMBOL;
    }

    line = "111" + comp + dest + jump;
    return SUCCESS;
}

void remove_special_symbols(std::string &line)
{
    std::string key;
    std::string value;
    for (auto &&pair : special_symbols_map)
    {
        key = pair.first;
        value = pair.second;

        line = std::regex_replace(line, std::regex(key), value);
    }
}

int process_line(std::string &line, const int &line_number)
{
    int status;
    std::string label;

    status = strip_white_space(line);
    if (status == EMPTY_LINE)
    {
        return EMPTY_LINE;
    }

    remove_special_symbols(line);

    status = get_label(line, label); // skip label lines
    if (status == LABEL_LINE)
    {
        return LABEL_LINE;
    }

    else if (line[0] == '@')
    {
        status = process_a_instruction(line);
        return status;
    }
    else
    {
        status = process_c_instruction(line);
        return status;
    }
}

int process_file(std::ifstream &in, std::ofstream &out)
{
    int status;
    int line_number = 0;
    std::string line;
    while (getline(in, line))
    {
        status = process_line(line, line_number); // process line and get code
        if (status != EMPTY_LINE && status != LABEL_LINE)
        {
            out << line << '\n';
            ++line_number;
        }
    }
    in.close();
    out.close();
    return SUCCESS;
}

int setup_file_read(const std::string &filename, std::ifstream &f)
{
    f.open(filename);
    if (f.bad())
    {
        return COULD_NOT_OPEN_FILE_READ;
    }
    else
    {
        return SUCCESS;
    }
}

int setup_file_write(const std::string &filename, std::ofstream &f)
{
    f.open(filename);
    if (f.bad())
    {
        return COULD_NOT_OPEN_FILE_WRITE;
    }
    else
    {
        return SUCCESS;
    }
}

void get_labels(std::ifstream &in) // shouldn't have exception
{
    int line_number = 0;
    int status;
    std::string line;
    std::string label;
    while (getline(in, line))
    {
        status = strip_white_space(line);
        if (status != EMPTY_LINE)
        {
            ++line_number;
        }
        status = get_label(line, label);
        if (status == LABEL_LINE)
        {
            --line_number;
            label_table.insert(std::make_pair(label, line_number));
        }
    }
    in.clear();
    in.seekg(0);
}

int assemble(const std::string &filename)
{
    int status;
    std::ifstream in_file;
    std::ofstream out_file;
    status = setup_file_read(filename, in_file);
    if (status != SUCCESS)
    {
        return COULD_NOT_OPEN_FILE_READ;
    }
    size_t lastindex = filename.find_last_of(".");
    std::string output_filename = filename.substr(0, lastindex);
    status = setup_file_write(output_filename + ".hack", out_file);
    if (status != SUCCESS)
    {
        return COULD_NOT_OPEN_FILE_WRITE;
    }

    get_labels(in_file);
    status = process_file(in_file, out_file);
    in_file.close();
    out_file.close();
    if (status != SUCCESS)
    {
        return status;
    }
    return SUCCESS;
}

int get_filename(int argc, char **argv, std::string &filename)
{
    if (argc != 2)
    {
        return INVALID_NUMBER_OF_ARGUMENTS;
    }
    else
    {
        filename = argv[1];
        return SUCCESS;
    }
}

void print_error(int code)
{
    switch (code)
    {
    case SUCCESS:
        std::cout << "Success\n";
        break;
    case INVALID_NUMBER_OF_ARGUMENTS:
        std::cout << "ERROR::Invalid number of arguments";
        break;
    case COULD_NOT_OPEN_FILE_WRITE:
        std::cout << "ERROR::Could not open file for writing";
        break;
    case COULD_NOT_OPEN_FILE_READ:
        std::cout << "ERROR::Could not open file for reading";
        break;
    default:
        break;
    }
}

int main(int argc, char **argv)
{
    std::cout << "HACK assember!" << '\n';

    int status;
    std::string filename;

    status = get_filename(argc, argv, filename);
    if (status != SUCCESS)
    {
        print_error(status);
        return status;
    }

    status = assemble(filename);
    if (status != SUCCESS)
    {
        print_error(status);
        return status;
    }

    std::cout << "Files outputted successfully!\n";
    return status;
}