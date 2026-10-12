// Created by WojFil Games 2024 - 2026
// This code is licensed under MIT license (see LICENSE.txt for details)

#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <limits>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <cerrno>
    #include <cstring>
    #include <dirent.h>
    #include <sys/stat.h>
#endif



namespace iskierka
{

// extension of Iskierka code files
static const std::string EXTENSION = "iski";

// this is the name of the root Variable, we start evaluation from here
static const std::string ROOT = "output";

// the only built-in variable so far, it represents empty text
static const std::string VAR_NOTHING = "__nothing";

// prefix of Variables in Iskierka code
static constexpr char PREFIX = '_';

// the default limit of function recursive calls, it prevents stack overflows
static constexpr uint64_t DEFAULT_RECURSION_LEVEL_LIMIT = 2048;

// this limit of allowed failures in a row seems small
// but this is only a mirage
// when the failure rate is 99% and we try 32768 times
// the probability of all failures is smaller than breaking AES-256 at first trial of brute force
static constexpr uint64_t DEFAULT_ALLOWED_FAILURES_IN_ROW = 32768;

static constexpr int64_t MAX_INT64 = 0x7FFFFFFFFFFFFFFF;
static constexpr uint64_t MAX_UINT64 = 0xFFFFFFFFFFFFFFFF;

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

static bool isSpace(const char ch)
{
    return std::isspace(ch);
}

static bool isLetter(const char ch)
{
    return (ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z');
};

static bool isDigit(const char ch)
{
    return ch >= '0' && ch <= '9';
};

static bool variableAllowedStartChar(const char ch)
{
    return (ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z');
};

static bool variableAllowedChar(const char ch)
{
    return (ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9');
};

struct Error : public std::runtime_error 
{
public:
    using std::runtime_error::runtime_error;
};

struct CompilationError : public Error 
{
public:
    using Error::Error;
};

struct GenerationError : public Error 
{
public:
    using Error::Error;
};


struct DeterministicRandom 
{
public:

// reset the engine deterministically
    void setSeed(const uint64_t seed) 
    {
        m_state = seed;
    }

// generate a value between 0 and (max - 1), so the upper bound is not included
    uint64_t nextUint64(const uint64_t max)
    {
        if (max == 0) {
            throw GenerationError("The RNG of IskierkaGen could not produce a number smaller than zero.");
        }

        const uint64_t threshold = -max % max;
        uint64_t value;

        do 
        {
            value = nextU64();
        } 
        while (value < threshold);

        return value % max;
    }

private:
    uint64_t m_state;

    uint64_t nextU64() 
    {
        uint64_t z = (m_state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};


// here starts the real code

struct Variable;

// the smallest unit of a rule
struct Unit
{
public:
    Unit(const std::string& value) 
        : m_value(value), m_variable(nullptr) { };

    Unit(Variable* variable) 
        : m_value(), m_variable(variable) { };

    std::string getString() const { return m_value; }  
    std::string getSpaces() const { return m_followingSpaces; }  
    void appendSpace(const char ch) { m_followingSpaces += ch; }  
    Variable* getVariablePtr() const { return m_variable; };
    bool isConstant() const { return ! m_value.empty() && m_variable == nullptr; };
    bool isVariable() const { return m_variable != nullptr; };
    bool isNothing() const { return m_value.empty() && m_variable == nullptr; };
    bool hasSpaces() const { return ! m_followingSpaces.empty(); };
    void removeSpaces() { m_followingSpaces.clear(); };
    
private:
    std::string m_value;
    std::string m_followingSpaces;
    Variable* m_variable;
};


struct Rule
{
public:
    Rule() = delete;

    Rule(const std::vector<Unit>& natural, const std::vector<Unit>& programming)
        : m_natural(natural), m_programming(programming)
    {
        insertUniqueIdents(m_natural);
        insertUniqueIdents(m_programming);
    };

    const std::vector<Unit> m_natural;
    const std::vector<Unit> m_programming;
    std::unordered_set<Variable*> m_uniqueIdents;

private:
    void insertUniqueIdents(const std::vector<Unit>& values) 
    {
        for (const Unit& v : values) {
            if (v.isVariable()) {
                m_uniqueIdents.emplace(v.getVariablePtr());
            }
        }
    };
};


struct Variable
{
public:
    const Rule& getRandomRule(DeterministicRandom& random)
    {
        if (m_units.size() == 1) {
            return m_units[0];
        }

        const uint64_t rand = random.nextUint64(m_totalWeight);

        for (size_t i = 0; i < m_weights.size(); i++) {
            if (m_weights[i] > rand) {
                return m_units[i];
            }
        }

        return m_units[m_weights.size() - 1];
    };

    bool weightIntegerOverflow(const uint64_t addition) const
    {
        return addition > MAX_INT64 - m_totalWeight;
    };

    void insert(std::vector<Unit>& natural, std::vector<Unit>& programming, const int64_t weight)
    {
        m_totalWeight += weight;
        m_weights.emplace_back(m_totalWeight);
        m_units.emplace_back(natural, programming);
    };

private:
    std::vector<Rule> m_units;
    uint64_t m_totalWeight = 0;
    std::vector<uint64_t> m_weights;
};


// we parse lines using a state machine
enum LineParsingMode
{
    FirstLine,
    SecondLine,
    ThirdLine
};


static std::vector<std::string> getFilesInDirectory(const std::string& path)
{

#ifdef _WIN32

    std::vector<std::string> result;
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;

    const std::string searchPattern = concat(path, "/*.", EXTENSION);

    hFind = FindFirstFileA(searchPattern.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        throw CompilationError(concat("Iskierka error: failed to enumerate files in the source directory '", 
            path, "'."));
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            result.emplace_back(concat(path, '/', findFileData.cFileName));
        }
    }
    while (FindNextFileA(hFind, &findFileData) != 0);

    const DWORD error = GetLastError();
    FindClose(hFind);

    if (error != ERROR_NO_MORE_FILES) {
        throw CompilationError(concat("Iskierka error: failed to enumerate files in the source directory '",
            path, "' (Windows error: ", error, ")."));
    }

    return result;

#else

    std::vector<std::string> result;
    DIR* dir;
    struct dirent* entry;
    dir = opendir(path.c_str());

    if (dir == nullptr) {
        throw CompilationError(concat("Iskierka error: the source directory '", path, "' could not be opened."));
    }

    const std::string extension = concat('.', EXTENSION);

    while (true) {
        errno = 0;
        entry = readdir(dir);

        if (entry == nullptr) {
            if (errno != 0) {
                const int error = errno;
                closedir(dir);

                throw CompilationError(concat("Iskierka error: failed to read the source directory '", path,
                    "' (errno ", error, ")."));
            }

            break;
        }

        if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
            continue;
        }

        const std::string filename = entry->d_name;

        if (filename.size() < extension.size() 
            || filename.compare(filename.size() - extension.size(), extension.size(), extension) != 0)
        {
            continue;
        }

        if (entry->d_type == DT_UNKNOWN) {
            const std::string fullPath = concat(path, '/', filename);
            struct stat fileInfo;

            if (stat(fullPath.c_str(), &fileInfo) != 0 || !S_ISREG(fileInfo.st_mode)) {
                continue;
            }
        }

        result.emplace_back(concat(path, '/', filename));
    }

    closedir(dir);
    return result;

#endif

};

static void checkFileNamesCorrectness(const std::vector<std::string>& paths)
{
    std::unordered_set<std::string> lowerNames;
    lowerNames.reserve(paths.size());

    for (const std::string& path : paths) {
        const int64_t end = static_cast<int64_t>(path.size()) - 1;
        int64_t start = -1;

        for (int64_t i = end; i >= 0; --i) {
            if (path[i] == '\\' || path[i] == '/') {
                start = i + 1;
                break;
            }
        }

        if (start == -1) {
            start = 0;
        }

        std::string name = path.substr(start, path.size() - start - EXTENSION.size() - 1);

        if (name.empty()) {
            throw CompilationError(
                concat("Every Iskierka file needs a full name. It cannot be just '.", EXTENSION, "'."));
        }

        if (! variableAllowedStartChar(name[0])) {
            throw CompilationError(
                concat("The name of the file '", path, "' is not correct for an Iskierka file. ",
                    "It should start with letters [a-zA-Z]."));
        }

        for (size_t i = 1; i < name.size(); i++) {
            if (! variableAllowedChar(name[i])) {
                throw CompilationError(
                    concat("The name of the file '", path, "' is not correct for an Iskierka file. ",
                        "Only characters [a-zA-Z0-9] are allowed."));
            }
        }

        std::string lowerName = name;

        for (char& c : lowerName) {
            if (c >= 'A' && c <= 'Z') {
                c += 'a' - 'A';
            }
        }

        if (lowerNames.find(lowerName) != lowerNames.end()) {
            throw CompilationError(
                concat("The name of the file '", path, "' is not correct for an Iskierka file. ",
                "Why? Because in the same folder there is another file with the same name case insensitive. ",
                "Two files with the same name can cause trouble when the project is exported between operating systems."));
        }

        lowerNames.emplace(lowerName);
    }
};

struct WrappedFile
{
public:
    WrappedFile() = delete;
    WrappedFile(const std::string& filePath) : m_file(filePath), m_isOpen(m_file.is_open()) { };
    ~WrappedFile() {
        if (m_isOpen) {
            m_file.close();
        }
    }

    std::ifstream m_file;
    const bool m_isOpen;
};

class IskierkaGen
{
public:
    IskierkaGen() { };
    IskierkaGen(const IskierkaGen&) = delete;
    IskierkaGen& operator=(const IskierkaGen&) = delete;

    // call this to generate a new pair of values
    void next(std::string& natural, std::string& programming)
    {
        if (! m_isCompiled) {
            throw GenerationError("IskierkaGen has not been compiled correctly.");
        }

        uint64_t attempts = 0;

        while (true) {
            if (generationUnit(natural, programming)) {
                break;
            }

            attempts++;

            if (attempts > m_allowedFailuresInRow || (m_allowedFailuresInRow == MAX_UINT64 && attempts == 0)) {
                throw GenerationError("IskierkaGen has failed to generate a pair of values. Too many failures in a row.");
            }
        }
    };

    // set new recursion level limit
    // be careful - too big will cause the program to crash in intense situations
    // you better don't touch that
    void setLevelLimit(const uint64_t limit)
    {
        m_levelLimit = limit;
    };

    // set your own limit of allowed generation failures in a row
    // again be careful with too high values
    void setAllowedFailuresInRow(const uint64_t failures)
    {
        m_allowedFailuresInRow = failures;
    };

    // reset the generator to a deterministic starting point
    void setSeed(const uint64_t seed) 
    {
        m_random.setSeed(seed);
    }
    
    // read source files with Iskierka codes and prepare a generator
    void compile(const uint64_t seed, const std::string& path)
    {
        if (m_hadCompilationAttempt) {
            if (m_isCompiled) {
                throw CompilationError("IskierkaGen has already been compiled. You should construct a fresh new object.");
            }
            else {
                throw CompilationError("IskierkaGen already had a compilation attempt, and it failed. You should construct a fresh new object.");
            }
        }
    
        m_hadCompilationAttempt = true;
        setSeed(seed);

        // get relative paths to all *.iski files in the source directory
        const std::vector<std::string> src = getFilesInDirectory(path);

        checkFileNamesCorrectness(src);

        if (src.empty()) {
            throw CompilationError(concat("Iskierka error: not a single *.iski file has been found in the directory '", ROOT, "'."));
        }

        std::unordered_map<std::string, std::string> variableFiles;

        // then, do the superficial 'first pass'
        for (const std::string& file : src) {
            firstPass(file, variableFiles);
        }

        // escape early if the root variable is not found
        if (m_variables.find(ROOT) == m_variables.end()) {
            throw CompilationError(concat("Iskierka error: not a single instance of the variable '", ROOT, "' has been found."));
        }

        // build all the rules
        for (const std::string& file : src) {
            secondPass(file);
        }
       
        // if everything is fine, set the flag m_isCompiled and load m_rootPtr for future fast access
        m_rootPtr = &m_variables.find(ROOT)->second;
        m_isCompiled = true;
    };

private:

    bool generationUnit(std::string& natural, std::string& programming)
    {
        m_level = 0;
        natural.clear();
        programming.clear();
        return evaluateVariable(*m_rootPtr, natural, programming);
    };

    bool evaluateVariable(Variable& var, std::string& natural, std::string& programming)
    {
        const Rule& expr = var.getRandomRule(m_random);
        return evaluateRule(expr, natural, programming);
    };

    bool evaluateRule(const Rule& expr, std::string& natural, std::string& programming)
    {
        std::unordered_map<Variable*, std::string> naturals;
        std::unordered_map<Variable*, std::string> programmings;
        
        for (Variable* var : expr.m_uniqueIdents) {
            std::string n;
            std::string p;

            m_level++;

            if (m_level >= m_levelLimit) {
                return false;
            }

            if (! evaluateVariable(*var, n, p)) {
                return false;
            }

            m_level--;
            
            naturals.emplace(var, n);
            programmings.emplace(var, p);
        }

        for (const Unit& unit : expr.m_natural) {
            if (unit.isConstant()) {
                natural += unit.getString();

                if (unit.hasSpaces()) {
                    natural += unit.getSpaces();
                }

                continue;
            }

            const std::string& add = naturals.find(unit.getVariablePtr())->second;

            if (! add.empty()) {
                natural += add;

                if (unit.hasSpaces()) {
                    natural += unit.getSpaces();
                }
            }
        }

        for (const Unit& unit : expr.m_programming) {
            if (unit.isConstant()) {
                programming += unit.getString();

                if (unit.hasSpaces()) {
                    programming += unit.getSpaces();
                }

                continue;
            }
            else {
                const std::string& add = programmings.find(unit.getVariablePtr())->second;

                if (! add.empty()) {
                    programming += add;

                    if (unit.hasSpaces()) {
                        programming += unit.getSpaces();
                    }
                }
            }
        }

        return true;
    };

    void compilationError(const std::string& msg, const std::string& filePath, const int64_t line)
    {
        throw CompilationError(concat("Iskierka error in the file '", filePath, "' at line ", std::to_string(line), ": ", msg));
    };

// the first pass scans all source files
// and prepares a map of all used Variables
// this is the only thing we do here (besides some error checks)
    void firstPass(const std::string& filePath, std::unordered_map<std::string, std::string>& variableFiles)
    {
        WrappedFile file(filePath);

        if (! file.m_isOpen) {
            throw CompilationError(concat("Iskierka error: unable to open the file '", filePath, "'."));
        }

        LineParsingMode mode = LineParsingMode::FirstLine;
        std::string line;
        int64_t lineId = 0;

        while (std::getline(file.m_file, line)) {
            trim(line);
            lineId++;

            switch (mode) {
                case LineParsingMode::FirstLine: {
                    if (line.empty() || line[0] != '#' || line == VAR_NOTHING) {
                        break;
                    }

                    if (line.size() == 1 && line[0] == '#') {
                        compilationError("missing variable name after #.", filePath, lineId);
                    }

                    if (line[1] == '#') {
                        compilationError("this version of Iskierka does not support special rules starting with ##.", filePath, lineId);
                    }

                    size_t start = 1;

                    if (! variableAllowedStartChar(line[start])) {
                        compilationError(concat("variable name cannot start with '", line[1], "'. Only letters [a-zA-Z] are allowed."), filePath, lineId);
                    }

                    mode = LineParsingMode::SecondLine;

                    size_t i = start;
                    for (; i < line.size(); i++) {
                        if (isSpace(line[i])) {
                            break;
                        }

                        if (! variableAllowedChar(line[i])) {
                            compilationError(concat("the character '", line[i], "' is not allowed within a variable name. ",
                                "Only characters [a-zA-Z0-9] are allowed."), filePath, lineId);

                        }
                    }

                    const std::string variable = line.substr(start, i - start);
                    const auto find = variableFiles.find(variable);

                    if (find == variableFiles.end()) {
                        variableFiles.emplace(variable, filePath);
                    }
                    else {
                        const std::string& originPath = find->second;
                        if (originPath != filePath) {
                            compilationError(concat("the variable '", variable, "' has already been defined in the file '", 
                                originPath,  "'. For security reasons, all rules of '", variable,
                                "' must appear inside one file."), filePath, lineId);
                        }
                    }

                    m_variables.emplace(variable, Variable());
                    break;
                }
                case LineParsingMode::SecondLine: {
                    if (line.empty()) {
                        compilationError("the second line of this rule is missing.", filePath, lineId);
                    }

                    mode = LineParsingMode::ThirdLine;
                    break;
                }
                case LineParsingMode::ThirdLine: {
                    if (line.empty()) {
                        compilationError("the third line of this rule is missing.", filePath, lineId);
                    }

                    mode = LineParsingMode::FirstLine;
                    break;
                }
            }
        }

        switch (mode) {
            case LineParsingMode::SecondLine: {
                compilationError("the second line of this rule is missing.", filePath, lineId);
                break;
            }
            case LineParsingMode::ThirdLine: {
                compilationError("the third line of this rule is missing.", filePath, lineId);
                break;
            }
            default: {
                break;
            }
        }
    };


// the second pass parses the code
// and builds all the rules
    void secondPass(const std::string& filePath)
    {
        WrappedFile file(filePath);

        if (! file.m_isOpen) {
            throw CompilationError(concat("Iskierka error: unable to open the file '", filePath, "'."));
        }

        LineParsingMode mode = LineParsingMode::FirstLine;
        std::string line;
        int64_t lineId = 0;

        std::string variableName;
        int64_t weight = 1;

        std::vector<Unit> natural;
        std::vector<Unit> programming;

        while (std::getline(file.m_file, line)) {
            trim(line);
            lineId++;

            switch (mode) {
                case LineParsingMode::FirstLine: {
                    if (line.empty() || line[0] != '#' || line == VAR_NOTHING) {
                        break;
                    }

                    if (line.size() == 1 && line[0] == '#') {
                        compilationError("missing variable name after #.", filePath, lineId);
                    }

                    if (line[1] == '#') {
                        compilationError("this version of Iskierka does not support special rules starting with ##.", filePath, lineId);
                    }
                    
                    size_t start = 1;

                    if (! variableAllowedStartChar(line[start])) {
                        compilationError(concat("variable name cannot start with '", line[1], "'. Only letters [a-zA-Z] are allowed."), filePath, lineId);
                    }

                    mode = LineParsingMode::SecondLine;

                    size_t i = start;
                    for (; i < line.size(); i++) {
                        if (isSpace(line[i])) {
                            break;
                        }

                        if (! variableAllowedChar(line[i])) {
                            compilationError(concat("the character '", line[i], "' is not allowed within a variable name. ",
                                "Only characters [a-zA-Z0-9] are allowed."), filePath, lineId);
                        }
                    }

                    variableName = line.substr(start, i - start);

                    if (i == line.size()) {
                        weight = 1;
                        break;
                    }

                    for (; i < line.size(); i++) {
                        if (! isSpace(line[i])) {
                            break;
                        }
                    }

                    const size_t memberStart = i;

                    for (; i < line.size(); i++) {
                        if (isSpace(line[i])) {
                            break;
                        }
                    }

                    const size_t memberLen = i - memberStart;
                    const std::string property = line.substr(memberStart, memberLen);

                    if (property != "weight") {
                        compilationError(concat("'", property, "' is not a property of a rule."), filePath, lineId);
                    }

                    for (; i < line.size(); i++) {
                        if (! isSpace(line[i])) {
                            break;
                        }
                    }

                    if (i == line.size()) {
                        compilationError(concat("the property '", property, "' does not contain an argument."), 
                            filePath, lineId);
                    }

                    if (line[i] == '-') {
                        compilationError(concat("the argument of the property '", property, "' cannot be a negative number."), 
                            filePath, lineId);
                    }
                    
                    const size_t integerStart = i;
                    bool wrongNumber = false;

                    for (; i < line.size(); i++) {
                        if (isSpace(line[i])) {
                            break;
                        }
                        else if (! isDigit(line[i])) {
                            wrongNumber = true;
                        }
                    }

                    for (; i < line.size(); i++) {
                        if (! isSpace(line[i])) {
                            compilationError(concat("the property '", property, "' takes exactly one argument."), 
                                filePath, lineId);
                        }
                    }

                    const size_t integerLen = i - integerStart;
                    const std::string textWithNumber = line.substr(integerStart, integerLen);

                    if (wrongNumber) {
                        compilationError(concat("the value '", textWithNumber, "' is not a correct for weight."), filePath, lineId);
                    }

                    try {
                        weight = std::stoll(textWithNumber);
                    } 
                    catch (const std::out_of_range& e) {
                        compilationError(concat("the number '", property, "' is too big. We are restricted by the range of int64."), 
                            filePath, lineId);
                    }

                    break;
                }
                case LineParsingMode::SecondLine: {
                    if (line.empty()) {
                        compilationError("the second line of this rule is missing.", filePath, lineId);
                    }
                    
                    if (line == VAR_NOTHING) {
                        natural.clear();
                    }
                    else {
                        parseLine(natural, line, filePath, lineId);
                    }

                    mode = LineParsingMode::ThirdLine;
                    break;
                }
                case LineParsingMode::ThirdLine: {
                    if (line.empty()) {
                        compilationError("the third line of this rule is missing.", filePath, lineId);
                    }

                    if (line == VAR_NOTHING) {
                        programming.clear();
                    }
                    else {
                        parseLine(programming, line, filePath, lineId);
                    }

                    Variable& id = m_variables.find(variableName)->second;

                    if (id.weightIntegerOverflow(weight)) {
                        compilationError("the weight of this rule is too big. An integer overflow happened.", filePath, lineId);
                    }

                    id.insert(natural, programming, weight);
                    mode = LineParsingMode::FirstLine;
                    break;
                }
            }
        }

        switch (mode) {
            case LineParsingMode::SecondLine: {
                compilationError("the second line of this rule is missing.", filePath, lineId);
                break;
            }
            case LineParsingMode::ThirdLine: {
                compilationError("the third line of this rule is missing.", filePath, lineId);
                break;
            }
            default: {
                break;
            }
        }
    };

    void trim(std::string& value) const
    {
        leftTrim(value);
        rightTrim(value);
    }

    void leftTrim(std::string& value) const
    {
        for (size_t i = 0; i < value.size(); i++) {
            if (! isSpace(value[i])) {
                if (i != 0) {
                    value = value.substr(i);
                }
                
                return;
            }
        }
    };

    void rightTrim(std::string& value) const
    {
        const int64_t end = static_cast<int64_t>(value.size()) - 1;

        for (int64_t i = end; i >= 0; --i) {
            if (! isSpace(value[i])) {
                if (i == end) {
                    return;
                }

                value = value.substr(0, i + 1);
                return;
            }
        }

        value.clear();
    };

    bool hasOnlyPrefixChars(const std::string& value) const
    {
        for (const char ch : value) {
            if (ch != PREFIX) {
                return false;
            }
        }
        return true;
    }

    void insertVariableUnit(std::vector<Unit>& result, const std::string& temp, 
        const std::string& filePath, const int64_t lineId)
    {
        if (hasOnlyPrefixChars(temp)) {
            result.emplace_back(temp);
            return;
        }

        if (temp[1] == PREFIX) {
            if (temp == VAR_NOTHING) {
                result.emplace_back(nullptr);
                return;
            }

            compilationError(concat("in this version of Iskierka, '", VAR_NOTHING, "' is the only allowed built-in variable."), 
                filePath, lineId);
        }

        const std::string name = temp.substr(1);
        auto var = m_variables.find(name);

        if (var == m_variables.end()) {
            compilationError(concat("the variable '", name, "' has not been defined."), filePath, lineId);
        }

        result.emplace_back(&var->second);
    }

    void parseLine(std::vector<Unit>& result, const std::string& line, 
        const std::string& filePath, const int64_t lineId)
    {
        result.clear();
        std::string temp;

        for (size_t i = 0; i < line.size(); i++) {
            const char ch = line[i];

            if (! temp.empty() && temp[0] == PREFIX) {
                if (isSpace(ch)) {
                    insertVariableUnit(result, temp, filePath, lineId);
                    result[result.size() - 1].appendSpace(ch);
                    temp.clear();
                    continue;
                }

                if (ch == PREFIX) {
                    if (temp[temp.size() - 1] == PREFIX) {
                        temp += PREFIX;
                        continue;
                    }

                    insertVariableUnit(result, temp, filePath, lineId);
                    temp = ch;
                    continue;
                }

                if (temp[temp.size() - 1] == PREFIX) {
                    if (variableAllowedStartChar(ch)) {
                        if (temp.size() > 2) {
                            compilationError(concat("there are too many underscores in a reference to a variable. Only one or two are allowed."), 
                                filePath, lineId);
                        }

                        temp += ch;
                        continue;
                    }

                    result.emplace_back(temp);
                    temp = ch;
                    continue;
                }

                if (variableAllowedChar(ch)) {
                    temp += ch;
                    continue;
                }

                insertVariableUnit(result, temp, filePath, lineId);
                temp = ch;
                continue;
            }
            
            if (ch == PREFIX) {
                if (! temp.empty()) {
                    result.emplace_back(temp);
                }

                temp = ch;
                continue;
            }
            
            temp += ch;
        }

        if (! temp.empty()) {
            if (temp[0] == PREFIX) {
                insertVariableUnit(result, temp, filePath, lineId);
            }
            else {
                result.emplace_back(temp);
            }
        }

        if (result.empty()) {
            return;
        }

        for (size_t i = 0; i < result.size(); i++) {
            const Unit& unit = result[i];

            if (unit.isNothing()) {
                result.erase(result.begin() + i);
                i--;
            }
        }

        if (! result.empty()) {
            result[result.size() - 1].removeSpaces();
        }
    };

    bool m_hadCompilationAttempt = false;
    bool m_isCompiled = false;
    std::unordered_map<std::string, Variable> m_variables;
    Variable* m_rootPtr = nullptr;
    DeterministicRandom m_random;
    
    // this is a runtime stack overflow protection mechanism
    // if too many functions are called recursively
    // the next() function is forced to fail instead of causing program crash
    // to turn it off, set m_levelLimit to some arbitrary huge value
    uint64_t m_levelLimit = DEFAULT_RECURSION_LEVEL_LIMIT;
    uint64_t m_level = 0;

    // IskierkaGen tries to generate values again after a failure, but its patience is not infinite
    uint64_t m_allowedFailuresInRow = DEFAULT_ALLOWED_FAILURES_IN_ROW;
};

};
