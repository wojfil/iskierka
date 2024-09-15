// Created by WojFil Games 2024
// This code is licensed under MIT license (see LICENSE.txt for details)

#ifndef ISKIERKA_H_INCLUDED
#define ISKIERKA_H_INCLUDED

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <random>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <cstring>
#endif



namespace iskierka
{

// extension of Iskierka code files
static constexpr char EXTENSION[] = "iski";

// this is the name of the root variable, we start evaluation from here
static constexpr char ROOT[] = "output";

// prefix of variables in Iskierka code
static constexpr char PREFIX = '_';

// the default limit of function recursive calls, it prevents stack overflows
static constexpr uint64_t DEFAULT_RECURSION_LEVEL_LIMIT = 2048;

// no execution flags
static constexpr uint32_t ISKIERKA_FLAG_NONE = 0;  

// errors happen as usual, but they do not print messages in the console
static constexpr uint32_t ISKIERKA_FLAG_SHOW_NO_ERRORS = 1; 


// this section of code provides optimized string concatenation

static size_t unitLen(const char value)
{
   return 1;
}

static size_t unitLen(const char value[])
{
   return strlen(value);
}

static size_t unitLen(const std::string& value)
{
   return value.size();
}

static size_t charsLen()
{
   return 0;
}

template<typename T, typename... Args>
static size_t charsLen(const T& firstValue, Args const&... args)
{
   return unitLen(firstValue) + charsLen(args...);
}

static void insertStr(std::string& result)
{
  // do nothing as expected
}

template<typename T, typename... Args>
static void insertStr(std::string& result, const T& firstValue, Args const&... args)
{
   result += firstValue;
   insertStr(result, args...);
}

// concatenate values into one string
// args can be of 3 types: char, char[], std::string
template<typename... Args>
static std::string concat(Args const&... args)
{
   std::string result;
   result.reserve(charsLen(args...));
   insertStr(result, args...);
   return result;
}


struct Variable;


// the smallest unit of an expression
struct Unit
{
public:
    virtual Variable* getVariablePtr() const { return nullptr; };
    virtual std::string getString() const { return std::string(); };
};


// string literal
struct ConstUnit : Unit
{
public:
    ConstUnit() = delete;
    ConstUnit(const std::string& val) : m_value(val) { };

    std::string getString() const override { return m_value; }

private:
    const std::string m_value;
};


// reference to some variable
struct VariableUnit : Unit
{
public:
    VariableUnit() = delete;
    VariableUnit(Variable& var) : m_variable(var) { };

    Variable* getVariablePtr() const override { return &m_variable; };

private:
    Variable& m_variable;
};


typedef std::vector<std::unique_ptr<Unit>> units_t;


struct HashExpression
{
public:
    HashExpression() = delete;

    HashExpression(units_t& natural, units_t& programming)
    {
        transferUniquePtrs(natural, m_natural);
        transferUniquePtrs(programming, m_programming);
        insertUniqueIdents(m_natural);
        insertUniqueIdents(m_programming);
    };


    units_t m_natural;
    units_t m_programming;
    std::unordered_set<Variable*> m_uniqueIdents;

private:
    void transferUniquePtrs(units_t& source, units_t& destination)
    {
        for (std::unique_ptr<Unit>& s : source) {
            destination.push_back(std::move(s));
        }
    };

    void insertUniqueIdents(const units_t& values) 
    {
        for (const std::unique_ptr<Unit>& v : values) {
            if (v->getVariablePtr() != nullptr) {
                m_uniqueIdents.emplace(v->getVariablePtr());
            }
        }
    };
};


// variable is a collection of all hash expressions named the same
struct Variable
{
public:
    const HashExpression& getRandomHashExpression(std::mt19937_64& randomness)
    {
        if (m_units.size() == 1) {
            return m_units[0];
        }

        const int64_t rand = (*m_distribution)(randomness);

        for (size_t i = 0; i < m_weights.size(); i++) {
            if (m_weights[i] > rand) {
                return m_units[i];
            }
        }

        return m_units[m_weights.size() - 1];
    };

    bool weightIntegerOverflow(const int64_t addition) const
    {
        const int64_t result = m_totalWeight + addition;
        return result < addition || result < m_totalWeight;
    };

    // insert new values
    // but only if this variable is not sealed
    bool insert(units_t& natural, units_t& programming, const int64_t weight)
    {
        if (m_sealed) {
            return false;
        }

        m_totalWeight += weight;
        m_weights.emplace_back(m_totalWeight);
        m_units.emplace_back(natural, programming);

        return true;
    };

    // variable is sealed only once
    // we prepare the probability distribution, so the lists no longer can be extended
    void seal() 
    {
        m_sealed = true;
        if (m_units.size() == 1) {
            return;
        }

        // if variable only contains hash expressions with weight == 0
        // prepare a discrete uniform distribution
        if (m_totalWeight == 0) {
            m_totalWeight = static_cast<int64_t>(m_units.size());

            for (size_t i = 0; i < m_units.size(); i++) {
                m_weights[i] = static_cast<int64_t>(i + 1);
            }
        }

        m_distribution = std::make_unique<std::uniform_int_distribution<int64_t>>(0, m_totalWeight - 1);
    };

    bool isEmpty() const 
    {
        return m_units.empty();
    };

private:
    std::vector<HashExpression> m_units;
    int64_t m_totalWeight = 0;
    std::vector<int64_t> m_weights;
    bool m_sealed = false;
    std::unique_ptr<std::uniform_int_distribution<int64_t>> m_distribution;
};


// we parse lines using a state machine
enum LineParsingMode
{
    FirstLine,
    SecondLine,
    ThirdLine
};


class IskierkaGen
{
public:
    IskierkaGen() = delete;

    // path = ralative path to the directory with *.iski files (NO recursive traversation)
    // flags = execution flags
    IskierkaGen(const std::string& path, const uint32_t flags) 
        : m_flags(flags), m_randomness(m_randomDevice())
    {
        loadData(path);
    };

    // path = ralative path to the directory with *.iski files (NO recursive traversation)
    IskierkaGen(const std::string& path) 
        : m_flags(ISKIERKA_FLAG_NONE), m_randomness(m_randomDevice())
    {
        loadData(path);
    };

    bool isParsed() const
    {
        return m_isParsed;
    };

    // call this to generate a new pair of values
    bool next(std::string& natural, std::string& programming)
    {
        if (! isParsed()) {
            return false;
        }

        m_level = 0;
        natural.clear();
        programming.clear();

        return evaluateVariable(*m_rootPtr, natural, programming);
    };

    uint32_t getFlags() const
    {
        return m_flags;
    };

    // set new recursion level limit
    // be careful - too big will cause the program to crash in intense situations
    // you better don't touch that
    void setLevelLimit(const int64_t limit)
    {
        m_levelLimit = limit;
    };
    

private:

    // read source files with Iskierka codes
    void loadData(const std::string& path)
    {
        // get relative paths to all *.iski files in the source directory
        std::vector<std::string> src;
        if (! getFilesInDirectory(src, path)) {
            return;
        }

        // directory exists, but there is no file with extension *.iski
        if (src.empty()) {
            error(concat("Iskierka error: not a single *.iski file has been found in directory '", path, "'."));
            return;
        }

        // then, do the superficial 'first pass'
        for (const std::string& file : src) {
            if (! firstPass(file)) {
                return;
            }
        }

        // escape early if the root variable is not found
        if (m_variables.find(ROOT) == m_variables.end()) {
            error(concat("Iskierka error: not a single instance of the variable '", ROOT, "' has been found."));
            return;
        }

        // build all expressions
        for (const std::string& file : src) {
            if (! secondPass(file)) {
                return;
            }
        }

        // check a very rare error if files were mutated during parsing
        for (auto& v : m_variables) {
            if (v.second.isEmpty()) {
                error(concat("Iskierka error: variable '", v.first, "' does not have any hash expression. ",
                    "The source code file was probably mutated by an external program during parsing. Try to run again."
                ));
                return;
            }
        }

        // seal variables => prepare probability distributions for them
        for (auto& v : m_variables) {
            v.second.seal();
        }
       
        // if everything is fine, set value m_isParsed and load m_rootPtr for future fast access
        m_rootPtr = &m_variables.find(ROOT)->second;
        m_isParsed = true;
    };

    bool evaluateVariable(Variable& var, std::string& natural, std::string& programming)
    {
        const HashExpression& expr = var.getRandomHashExpression(m_randomness);
        return evaluateHashExpression(expr, natural, programming);
    };

    bool evaluateHashExpression(const HashExpression& expr, std::string& natural, std::string& programming)
    {
        std::unordered_map<Variable*, std::string> naturals;
        std::unordered_map<Variable*, std::string> programmings;
        
        for (Variable* var : expr.m_uniqueIdents) {
            std::string n;
            std::string p;

            m_level++;

            if (m_level >= m_levelLimit || ! evaluateVariable(*var, n, p)) {
                return false;
            }

            m_level--;
            
            naturals.emplace(var, n);
            programmings.emplace(var, p);
        }

        bool naturalOmitSpace = false;
        bool programmingOmitSpace = false;

        for (const std::unique_ptr<Unit>& he : expr.m_natural) {
            if (he->getVariablePtr() == nullptr) {
                if (naturalOmitSpace) {
                    naturalOmitSpace = false;

                    natural += std::isspace(he->getString()[0])
                        ? he->getString().substr(1)
                        : he->getString();
                }
                else {
                    natural += he->getString();
                }
            }
            else {
                naturalOmitSpace = false;
                const std::string& add = naturals.find(he->getVariablePtr())->second;
                if (add.empty()) {
                    if (! natural.empty() && std::isspace(natural[natural.size() - 1])) {
                        natural.resize(natural.size() - 1);
                    }
                    else {
                        naturalOmitSpace = true;
                    }
                }
                else {
                    natural += add;
                }
            }
        }

        for (const std::unique_ptr<Unit>& he : expr.m_programming) {
            if (he->getVariablePtr() == nullptr) {
                if (programmingOmitSpace) {
                    programmingOmitSpace = false;

                    programming += std::isspace(he->getString()[0])
                        ? he->getString().substr(1)
                        : he->getString();
                }
                else {
                    programming += he->getString();
                }
            }
            else {
                programmingOmitSpace = false;
                const std::string& add = programmings.find(he->getVariablePtr())->second;
                if (add.empty()) {
                    if (! programming.empty() && std::isspace(programming[programming.size() - 1])) {
                        programming.resize(programming.size() - 1);
                    }
                    else {
                        programmingOmitSpace = true;
                    }
                }
                else {
                    programming += add;
                }
            }
        }

        return true;
    };

    void error(const std::string& msg)
    {
        if (m_flags & ISKIERKA_FLAG_SHOW_NO_ERRORS) {
            return;
        }

        std::cout << msg << std::endl;
    };

    void error(const std::string& msg, const std::string& filePath, const int64_t line)
    {
        if (m_flags & ISKIERKA_FLAG_SHOW_NO_ERRORS) {
            return;
        }

        std::cout << concat("Iskierka error in file '", filePath, "' at line ", std::to_string(line), ": ", msg) << std::endl;
    };

    bool getFilesInDirectory(std::vector<std::string>& result, const std::string& path)
    {

#ifdef _WIN32

        WIN32_FIND_DATAA findFileData;
        HANDLE hFind;

        const std::string searchPattern = concat(path, "\\*.", EXTENSION);

        hFind = FindFirstFileA(searchPattern.c_str(), &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
            error(concat("Iskierka error: source directory '", path, "' could not be opened."));
            return false;
        }

        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                result.emplace_back(concat(path, '\\', findFileData.cFileName));
            }
        }
        while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
       
        return true;

#else

        DIR* dir;
        struct dirent* entry;
        dir = opendir(path.c_str());

        if (dir == nullptr) {
            error(concat("Iskierka error: source directory '", path, "' could not be opened."));
            return false;
        }

        const std::string extension = concat('.', EXTENSION);

        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                const std::string filename = entry->d_name;
                if (filename.size() >= extension.size() && 
                    filename.compare(filename.size() - extension.size(), extension.size(), extension) == 0) {
                    result.emplace_back(concat(path, '/', filename));
                }
            }
        }

        closedir(dir);
        return true;
    
#endif

    };

    bool variableAllowedStartChar(const char ch) const
    {
        return (ch >= 'a' && ch <= 'z')
            || (ch >= 'A' && ch <= 'Z');
    };
   
    bool variableAllowedChar(const char ch) const
    {
        return (ch >= 'a' && ch <= 'z')
            || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9');
    };

    bool isLetter(const char ch) const
    {
        return (ch >= 'a' && ch <= 'z')
            || (ch >= 'A' && ch <= 'Z');
    };


// the first pass scans all source files
// and prepares a map of all used variables
// this is the only thing we do here (besides some error checks)
    bool firstPass(const std::string& filePath)
    {
        std::ifstream file(filePath);

        if (!file.is_open()) {
            error(concat("Iskierka error: unable to open file '", filePath, "'."));
            return false;
        }

        LineParsingMode mode = LineParsingMode::FirstLine;
        std::string line;
        int64_t lineId = 0;

        while (std::getline(file, line)) {
            rightTrim(line);
            lineId++;

            switch (mode) {
                case LineParsingMode::FirstLine: {
                    if (line.empty() || line[0] != '#' || line == "##empty") {
                        break;
                    }

                    if (line.size() == 1 && line[0] == '#') {
                        error("missing variable name after #.", filePath, lineId);
                        file.close();
                        return false;
                    }

                    if (line[1] == '#') {
                        error(concat("the double hash expression '", line, "' is not recognized."), filePath, lineId);
                        file.close();
                        return false;
                    }

                    size_t start = 1;

                    if (! variableAllowedStartChar(line[start])) {
                        error(concat("variable name cannot start with '", line[1], "'. Only letters a-zA-Z are allowed."), filePath, lineId);
                        file.close();
                        return false;
                    }

                    mode = LineParsingMode::SecondLine;

                    size_t i = start;
                    for (; i < line.size(); i++) {
                        if (std::isspace(line[i])) {
                            break;
                        }

                        if (! variableAllowedChar(line[i])) {
                            error(concat("character '", line[i], "' is not allowed in a variable name."), filePath, lineId);
                            file.close();
                            return false;
                        }
                    }

                    const std::string variable = line.substr(start, i - start);
                    m_variables.emplace(variable, Variable());
                    break;
                }
                case LineParsingMode::SecondLine: {
                    if (line.empty()) {
                        error("second line of this hash expression is missing.", filePath, lineId);
                        file.close();
                        return false;
                    }

                    mode = LineParsingMode::ThirdLine;
                    break;
                }
                case LineParsingMode::ThirdLine: {
                    if (line.empty()) {
                        error("third line of this hash expression is missing.", filePath, lineId);
                        file.close();
                        return false;
                    }

                    mode = LineParsingMode::FirstLine;
                    break;
                }
            }
        }

        switch (mode) {
            case LineParsingMode::SecondLine: {
                error("second line of this hash expression is missing.", filePath, lineId);
                file.close();
                return false;
            }
            case LineParsingMode::ThirdLine: {
                error("third line of this hash expression is missing.", filePath, lineId);
                file.close();
                return false;
            }
            default: {
                break;
            }
        }

        file.close();
        return true;
    };


// the second pass parses the code
// and builds all the hash expressions
    bool secondPass(const std::string& filePath)
    {
        std::ifstream file(filePath);

        if (!file.is_open()) {
            error(concat("Iskierka error: unable to open file '", filePath, "'."));
            return false;
        }

        LineParsingMode mode = LineParsingMode::FirstLine;
        std::string line;
        int64_t lineId = 0;

        std::string variableName;
        int64_t weight = 1;

        units_t natural;
        units_t programming;

        while (std::getline(file, line)) {
            rightTrim(line);

            lineId++;

            switch (mode) {
                case LineParsingMode::FirstLine: {
                    if (line.empty() || line[0] != '#' || line == "##empty") {
                        break;
                    }

                    if (line.size() == 1 && line[0] == '#') {
                        error("missing variable name after #.", filePath, lineId);
                        file.close(); 
                        return false;
                    }

                    if (line[1] == '#') {
                        error(concat("the double hash expression '", line,"' is not recognized."), filePath, lineId);
                        file.close(); 
                        return false;
                    }
                    
                    size_t start = 1;

                    if (! variableAllowedStartChar(line[start])) {
                        error(concat("variable name cannot start with '", line[1], "'. Only letters a-zA-Z are allowed."), filePath, lineId);
                        file.close(); 
                        return false;
                    }

                    mode = LineParsingMode::SecondLine;

                    size_t i = start;
                    for (; i < line.size(); i++) {
                        if (std::isspace(line[i])) {
                            break;
                        }

                        if (! variableAllowedChar(line[i])) {
                            error(concat("character '", line[i], "' is not allowed within a variable name."), filePath, lineId);
                            file.close(); 
                            return false;
                        }
                    }

                    variableName = line.substr(start, i - start);

                    if (i == line.size()) {
                        weight = 1;
                        break;
                    }

                    for (; i < line.size(); i++) {
                        if (! std::isspace(line[i])) {
                            break;
                        }
                    }

                    const size_t memberStart = i;

                    for (; i < line.size(); i++) {
                        if (std::isspace(line[i])) {
                            break;
                        }
                    }

                    const size_t memberLen = i - memberStart;
                    const std::string property = line.substr(memberStart, memberLen);

                    if (property != "weight") {
                        error(concat("'", property, "' is not a property of a hash expression."), filePath, lineId);
                        file.close(); 
                        return false;
                    }

                    for (; i < line.size(); i++) {
                        if (! std::isspace(line[i])) {
                            break;
                        }
                    }

                    if (i == line.size()) {
                        error(concat("property '", property, "' is not followed by a positive integer argument."), 
                            filePath, lineId);

                        file.close(); 
                        return false;
                    }
                    
                    const size_t integerStart = i;
                    bool wrongNumber = false;

                    for (; i < line.size(); i++) {
                        if (std::isspace(line[i])) {
                            break;
                        }
                        else if (! std::isdigit(line[i])) {
                            wrongNumber = true;
                        }
                    }

                    const size_t integerLen = i - integerStart;
                    const std::string textWithNumber = line.substr(integerStart, integerLen);

                    if (wrongNumber) {
                        error(concat("value '", textWithNumber, "' is not a positive integer."), filePath, lineId);
                        file.close();
                        return false;
                    }

                    try {
                        weight = std::stoll(textWithNumber);
                    } 
                    catch (const std::out_of_range& e) {
                        error(concat("number '", property, "' is too big. We are restricted by the range of int64."), 
                            filePath, lineId);

                        file.close(); 
                        return false;
                    }

                    break;
                }
                case LineParsingMode::SecondLine: {
                    const bool isEmptyLineIdentifier = line == "##empty";
                    leftTrim(line);

                    if (line.empty()) {
                        error("second line of this hash expression is missing.", filePath, lineId);
                        file.close(); 
                        return false;
                    }
                    
                    if (isEmptyLineIdentifier) {
                        natural.clear();
                    }
                    else if (! parseLine(natural, line, filePath, lineId)) {
                        file.close(); 
                        return false;
                    }

                    mode = LineParsingMode::ThirdLine;
                    break;
                }
                case LineParsingMode::ThirdLine: {
                    const bool isEmptyLineIdentifier = line == "##empty";
                    leftTrim(line);

                    if (line.empty()) {
                        error("third line of this hash expression is missing.", filePath, lineId);
                        file.close(); 
                        return false;
                    }

                    if (isEmptyLineIdentifier) {
                        programming.clear();
                    }
                    else if (! parseLine(programming, line, filePath, lineId)) {
                        file.close(); 
                        return false;
                    }

                    Variable& id = m_variables.find(variableName)->second;

                    if (id.weightIntegerOverflow(weight)) {
                        error("the weight of this hash expression is too big. Integer overflow happened.", filePath, lineId);
                        file.close(); 
                        return false;
                    }

                    if (! id.insert(natural, programming, weight)) {
                        error("we cannot add more hash expressions. The variable is sealed and finished.", filePath, lineId);
                        file.close(); 
                        return false;
                    }

                    mode = LineParsingMode::FirstLine;
                    break;
                }
            }
        }

        switch (mode) {
            case LineParsingMode::SecondLine: {
                error("second line of this hash expression is missing.", filePath, lineId);
                file.close(); 
                return false;
            }
            case LineParsingMode::ThirdLine: {
                error("third line of this hash expression is missing.", filePath, lineId);
                file.close(); 
                return false;
            }
            default: {
                break;
            }
        }

        file.close();
        return true;
    };

    void leftTrim(std::string& value) const
    {
        for (size_t i = 0; i < value.size(); i++) {
            if (! std::isspace(value[i])) {
                if (i != 0) {
                    value = value.substr(i);
                }
                
                return;
            }
        }

        value.clear();
    };

    void rightTrim(std::string& value) const
    {
        const int end = static_cast<int>(value.size()) - 1;

        for (int i = end; i >= 0; --i) {
            if (! std::isspace(value[i])) {
                if (i == end) {
                    return;
                }

                value = value.substr(0, i + 1);
                return;
            }
        }

        value.clear();
    };

    bool parseLine(units_t& result, const std::string& line, const std::string& filePath, const int64_t lineId)
    {
        result.clear();

        if (line.size() >= 2 && line[0] == '#' && line[1] == '#') {
            error(concat("the double hash expression '", line, "' is not recognized."), filePath, lineId);
            return false;
        }

        bool nowStringLiteral = true;
        size_t start = 0;

        for (size_t i = 0; i < line.size(); i++) {
            if (nowStringLiteral) {
                if (line[i] == PREFIX 
                    && !(i == (line.size() - 1) || std::isspace(line[i + 1]))
                    && (i == 0 || !isLetter(line[i - 1])))
                {
                    const std::string literal = line.substr(start, i - start);
                    start = i;
                    nowStringLiteral = false;
                    result.emplace_back(std::make_unique<ConstUnit>(literal));
                }
            }
            else {
                if (! variableAllowedChar(line[i])) {
                    const std::string name = line.substr(start + 1, i - start - 1);

                    if (name.empty()) {
                        error(concat("variables with prefix __ are not allowed in this version of Iskierka."), filePath, lineId);
                        return false;
                    }

                    auto var = m_variables.find(name);

                    if (var == m_variables.end()) {
                        error(concat("variable '", name, "' has not been defined."), filePath, lineId);
                        return false;
                    }

                    start = i;
                    nowStringLiteral = line[i] != '_';
                    result.emplace_back(std::make_unique<VariableUnit>(var->second));
                }
            }
        }

        if (nowStringLiteral) {
            const std::string literal = line.substr(start);
            result.emplace_back(std::make_unique<ConstUnit>(literal));
        }
        else {
            const std::string name = line.substr(start + 1);
            auto var = m_variables.find(name);
            if (var == m_variables.end()) {
                error(concat("variable '", name, "' has not been defined."), filePath, lineId);
                return false;
            }

            result.emplace_back(std::make_unique<VariableUnit>(var->second));
        }

        return true;
    };

    const uint32_t m_flags;
    bool m_isParsed = false;
    std::unordered_map<std::string, Variable> m_variables;
    Variable* m_rootPtr = nullptr;
    std::random_device m_randomDevice;
    std::mt19937_64 m_randomness;
    
    // this mechanism protects us from runtime stack overflows
    // if too many functions are called recursively
    // the next() function is forced to fail instead of causing program crash
    // you can turn this off and go YOLO - set m_levelLimit to some arbitrary huge value
    int64_t m_levelLimit = DEFAULT_RECURSION_LEVEL_LIMIT;
    int64_t m_level = 0;
};

};

#endif // ISKIERKA_H_INCLUDED
