#ifndef USERIO_H
#define USERIO_H
#include <string>   // string because they're not as limited and can be made into cstrings by c_str anyways
#include <iostream> // iostream for user input/output
#include <time.h>   // include time.h for struct tm and mktime functions
#include <boost/regex.hpp>  // as much as I dislike boost, it's still the only
                            // remotely useful regex library. Sorry.


class userio
{
    public:
        // User input

        // Returns a sanitized [type], sanitization determined by interaction enum.
        static char GetChar(std::string pre, int how, char alt1 = '-', char alt2 = '-');
        static int GetInt(std::string pre, int how, int alt1, int alt2 = 0);
        static std::string GetString(std::string pre, int how, std::string str1 = "", const int min = 0, unsigned const int max = 0);
        static std::string GetString(std::string pre, std::string regex, std::string readable_regex);

        // Returns a timestamp by requesting date and time from the user
        static time_t GetTimestamp(std::string pre);

        // Returns time only by requesting hour / minute / seconds from user
        static struct tm GetTime(std::string pre);

        // Returns date only by requesting year / month / date from user
        static struct tm GetDate(std::string pre);

        // Merge two time structs (one with hh:mm:ss and one with yyyy:mm:dd)
        static time_t MakeTimestamp(struct tm clock, struct tm date);

        // Returns

        // Get regular types (no sanitation)
        static char GetChar();
        static int GetNum();
        static char* GetpChar();
        static std::string GetStr();

        // Filter types for sanitation
        enum
        {
            NOFILTER,
            MIN,
            MAX,
            BETWEEN,
            EQUALS,
            NOT,
            CONTAINS
        };

    protected:
    private:
        static std::string FilterPretext(std::string pre);
};

#endif // USERIO_H
