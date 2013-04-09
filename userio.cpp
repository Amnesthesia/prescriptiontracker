#include "userio.h"


/**
 ** Sanitizes a char (as determined by the interaction enum 'how')
 **
 ** @param string pre       Text to print before catching users input
 ** @param integer how      Determines how the expected char should be
 ** @param char alt1        Used when 'how' is EQUALS, BETWEEN, NOT, or CONTAINS to compare against
 ** @param char alt2        Used when 'how' is EQUALS or BETWEEN in conjunction with alt1
 ** @return char
 **/
char userio::GetChar(std::string pre, int how, char alt1, char alt2)
{
    // Lets inform the user of the limitations -- append appropriate
    // informational message to the pretext.
    std::string append;
    switch(how)
    {
    // Append (x-y) to pretext
    case BETWEEN:
        append = " (";
        append += alt1;
        append += "-";
        append += alt2;
        append += ")";
        break;
    // Append (Accepted input: x[,y]) to pretext
    case CONTAINS:
    case EQUALS:
        append = "(Accepted input: ";
        append += alt1;
        if(alt2 != '-')
        {
            append += ",";
            append += alt2;
        }
        append += ")";
        break;
    // Append (Invalid input: x) to pretext
    case NOT:
        append = "(Invalid input: ";
        append += alt1;
        append += ")";
        break;

    // Append (x-∞) to pretext
    case MIN:
        append = "(";
        append += alt1;
        append += "-\u221E)";
        break;

    // Append (<x) to pretext
    case MAX:
        append = "(<";
        append += alt1;
        append += ")";
        break;
    default:
        append = "";

    }
    pre = FilterPretext(pre);
    // Print pretext and tailing append
    std::cout << pre << " " << append << ": ";
    char c = GetChar();

    // Set up error message variable and copy pretext data to it.
    std::string err = pre;

    // Add a newline to the appendage and insert it before the pretext.
    append += ".\n";
    err.insert(0,append);

    // Now perform the appropriate filtering action ... recurse deeper
    // (and add error message) on invalid input
    switch(how)
    {
    // If the character entered has an ascii value of lower than that of alt1,
    // the input is invalid. By "too early in the alphabet", we mean "too early in the ascii table"
    case MIN:
        err.insert(0, "ERROR: The letter you entered is too early in the alphabet");
        return ((int)c > (int)alt1 ? c : GetChar(err,how,alt1,alt2));
        break;
    // If the character entered has an ascii value of lower than that of alt1, input is invalid
    case MAX:
        err.insert(0,"ERROR: The letter you entered is too late in the alphabet");
        return ((int)c < (int)alt1 ? c : GetChar(err,how,alt1,alt2));
        break;
    // If the character entered is not found between alt1 and alt2 in the ascii table, input is invalid
    case BETWEEN:
        err.insert(0,"ERROR: Sorry, please enter a letter between ");
        return (((int)c < (int)alt2  && (int)c > (int)alt1) ? c : GetChar(err,how,alt1,alt2));
        break;

    // Since char is a single character, CONTAINS becomes the same as EQUALS.
    // If character does not equal alt1 (or alt2 if specified), input is invalid
    case CONTAINS:
    case EQUALS:
        err.insert(0,"ERROR: Sorry, your input was not valid ");
        return (alt2 != '-' ? ( c == alt1 || c == alt2 ? c : GetChar(err,how,alt1,alt2)) : (c == alt1 ? c : GetChar(err,how,alt1,alt2)));
        break;

    // If character equals alt1, input is invalid.
    case NOT:
        err.insert(0,"ERROR: Sorry, your input was not valid ");
        return (c != alt1 ? c : GetChar(err,how,alt1,alt2));
        break;

    // Without filter, there's no such thing as invalid :)
    case NOFILTER:
        return c;
        break;
    }
    // To avoid warnings by compiler, return the same as NOFILTER would return:
    return c;
}


/**
 ** Sanitizes an integer (as determined by the interaction enum 'how')
 **
 ** @param string pre       Text to print before catching users input
 ** @param integer how      Determines how the expected integer should be
 ** @param int alt1         Used when 'how' is EQUALS, BETWEEN, NOT, or CONTAINS to compare against
 ** @param int alt2         Used when 'how' is EQUALS, BETWEEN or NOT in conjunction with alt1
 ** @return int
 **/
int userio::GetInt(std::string pre, int how, int alt1, int alt2)
{
    // Set up appendage for pretext
    std::string append;

    // Set up temporary strings and boolean for comparison later, for the CONTAINS action
    std::string alt1Str = std::to_string(alt1);
    std::string alt2Str = std::to_string(alt2);
    bool contains_num = false;

    // Define error messages
    switch(how)
    {
    case MIN:
        append = "(minimum ";
        append += alt1;
        append += ")";
        break;

    case MAX:
        append = "(maximum ";
        append += alt1;
        append += ")";
        break;

    case BETWEEN:
        append = "(";
        append += alt1Str;
        append += "-";
        append += alt2Str;
        append += ")";
        break;

    case CONTAINS:
        append = "(must contain: ";
        append += alt1;
        if(alt2 != 0)
        {
            append += " or ";
            append += alt2Str;
        }
        append += ")";
        break;

    case EQUALS:
        append = "(must equal: ";
        append += alt1Str;
        append += " or ";
        append += alt2Str;
        append += ")";
        break;

    case NOT:
        append = "(may not equal: ";
        append += alt1Str;
        if(alt2 != 0)
        {
         append += ", ";
         append += alt2Str;
        }
        append += ")";
        break;
    default:
        std::cout << "'how' NOT MATCHED! How == " << how;
    }

    std::cout << pre << append;
    int i = GetNum();

    // Cast i to string for comparison later, if 'how' equals CONTAINS
    std::string tmp = std::to_string(i);

    // If there's an error message in the pretext, remove it.
    pre = FilterPretext(pre);

    // Set up error message container, prepend the appendage to it with newline separation
    std::string err = pre;
    append += ".\n";
    err.insert(0,append);

    // Filter the input as defined by 'how'
    switch(how)
    {
        // If value is less than alt1, try again and add an error message.
    case MIN:
        err.insert(0,"ERROR: Value too low ");
        return (i>alt1 ? i : GetInt(err,how,alt1,alt2));

        // If value is higher than alt1, recurse deeper -- now with an error message
    case MAX:
        err.insert(0,"ERROR: Value too high ");
        return (i<alt1 ? i : GetInt(err,how,alt1,alt2));

        // If value is not between alt1 and alt2, recurse deeper and add an error message
    case BETWEEN:
        err.insert(0, "ERROR: Value must be between ");
        return (i>alt1 && i<alt2 ? i : GetInt(err,how,alt1,alt2));

        // Turn integer into string to compare individual positions against alt1 and alt2,
        // if match is found, return value, else recurse deeper and add error message.
    case CONTAINS:
        err.insert(0, "ERROR: Invalid input ");
        for(unsigned int l=0;l<tmp.size();l++)
            if(alt1Str.compare(std::to_string(tmp.at(l))) == 0 || (alt2 != 0 && alt2Str.compare(std::to_string(tmp.at(l))) == 0))
                contains_num = true;
        return (contains_num ? i : GetInt(err,how,alt1,alt2));

        // If integer does not equal alt1 (or alt2 if specified), recurse deeper and add an error message. Else return value.
    case EQUALS:
        err.insert(0, "ERROR: Invalid input ");
        return (alt2 != 0 ? (i == alt1 || i == alt2 ? i : GetInt(err,how,alt1,alt2)) : (i == alt1 ? i : GetInt(err,how,alt1,alt2)));

        // If integer equals alt1 (or alt2 if specified), recurse deeper and add an error message. Else return value.
    case NOT:
        err.insert(0, "ERROR: Invalid input ");
        return (alt2 != 0 ? (i != alt1 && i != alt2 ? i : GetInt(err,how,alt1,alt2)) : (i != alt1 ? i : GetInt(err,how,alt1,alt2)));

        // No filters to apply == just return the value unfiltered
    default:
    case NOFILTER:
        return i;
    }
}


/**
 ** Sanitizes a string (as determined by the interaction enum 'how')
 **
 ** @param string pre       Text to print before catching users input
 ** @param integer how      Determines how the expected integer should be
 ** @param int str1         Used when 'how' is not set to NOFILTER to compare against
 ** @param int min          Used when how is set to MIN or BETWEEN
 ** @param int max          Used when how is set to MAX or BETWEEN
 ** @return string
 **/
std::string userio::GetString(std::string pre, int how, std::string str1, const int min, unsigned const int max)
{
    // If min or max are set, print the min and max values with the pretext
    std::string append = "";

    // String versions of min and max values
    std::string minStr = std::to_string(min);
    std::string maxStr = std::to_string(max);

    // Set up appendage for pretext
    switch(how)
    {
    case MIN:
        append += "(minimum length: ";
        append += minStr;
        append += ")";
        break;
    case MAX:
        append += "(max length: ";
        append += maxStr;
        append += ")";
        break;
    case BETWEEN:
        append += "(length between: ";
        append += minStr;
        append += "-";
        append += maxStr;
        append += ")";
        break;
    case CONTAINS:
        append += "(must contain: ";
        append += str1;
        append += ")";
        break;
    case EQUALS:
        append += "(must equal: ";
        append += str1;
        append += ")";
        break;
    case NOT:
        append += "(may not equal: ";
        append += str1;
        append += ")";
        break;
    case NOFILTER:
    default:
        break;
    }


    // Print pretext and call GetStr to get a string from the user
    std::cout << pre << append << ": ";
    std::string str = GetStr();

    // Filter away error messages left in pretext, if any
    pre = FilterPretext(pre);

    // Prepend newline to appendage, prepend it to err, and prepare to prepend error message.
    std::string err = pre;
    append += ".\n";
    err.insert(0,append);

    // .. now apply the filters to the string -- either return it, or try again (+ error message) until
    // we can return it.
    switch(how)
    {
    // If str is longer than max or shorter than min, try again with an error message -- else return
    case BETWEEN:
        err.insert(0, "ERROR: Sorry, your string was too long or too short ");
        return (str.size() > max || str.size() < min ? GetString(err, how, str1, min, max) : str);
        break;
    // If str equals str1, return str -- else try again with an error message
    case EQUALS:
        err.insert(0, "ERROR: Sorry, strings do not match ");
        return (str.compare(str1) == 0 ? str : GetString(err,how, str1, min, max));
        break;
    // If str is not equal to str1, return str -- else try again with an error message
    case NOT:
        err.insert(0, "ERROR: Sorry, strings are equal. Please provide a different string ");
        return (str.compare(str1) == 0 ? GetString(err, how, str1, min, max) : str);
        break;
    // If str1 is a substring of str, return str -- else try again with an error message
    case CONTAINS:
        err.insert(0, "ERROR: Sorry, your string was not found as a substring ");
        return (str.find(str1) != std::string::npos ? str : GetString(err, how, str1, min, max));
        break;
    // No sanitation -- just return the string!
    case NOFILTER:
    default:
        return str;
    }
}

/**
 ** Gets a string from the user through the input stream
 ** only if it matches the specified regex pattern.
 **
 ** @param string pre
 ** @param string regex
 ** @return string
 */
std::string userio::GetString(std::string pre, std::string regex, std::string readable_regex)
{
    // Set up our regex variable and try assigning our regex string to it
    boost::regex e;
    try
    {
        e.assign(regex);
    }
    // If it failed, catch and print the error.
    catch (boost::regex_error& e)
    {
        std::cout << e.what();
    }

    // Print the pretext and the readable regex with it.
    std::cout << pre << " (" << readable_regex << "): ";
    std::string str = GetStr();

    // Filter away potential error message
    pre = FilterPretext(pre);

    // Run the match! If it matches, return the string -- else recurse with error message.
    if(boost::regex_match(str,e))
        return str;
    else
    {
        std::string err = "ERROR: Format of string did not match that of ";
        err += readable_regex;
        err += ".\n";
        std::cout << err;
        return GetString(pre, regex, readable_regex);
    }
}



/**
 ** Gets a char from the user through the input stream
 **
 ** @return char
 */
char userio::GetChar()
{
    char c;
    std::cin >> c;
    std::cin.ignore();

    return tolower(c);
}

/**
 ** Gets an integer from the user through the input stream
 **
 ** @return integer
 */
int userio::GetNum()
{
    int i;
    std::cin >> i;
    std::cin.ignore();

    return i;
}

/**
 ** Gets a string from the user through the input stream
 **
 ** @return string
 */
std::string userio::GetStr()
{
    std::string str;
    getline(std::cin,str);
    return str;
}

/**
 ** Creates a timestruct from userprovided hour and time.
 **
 ** @param string pre   Pretext to show before taking input
 ** @return struct tm
 */
struct tm userio::GetTime(std::string pre)
{
    // We want time in the format of hh:mm:ss:[am|pm] (am/pm optional), maximum hour 23, minute 60, etc; so lets regex that:
    //std::string regex ("(2[0-3]|1[0-9]):[0-5][0-9]:[0-5][0-9]");
    std::string regex ("(2[0-3]|1[0-9])?[0-5][0-9]?[0-5][0-9]");
    std::string str = GetString(pre,regex,"hh:mm:ss");

    // Now, find the position of the two :-characters
    int mPos = str.find(":");
    int sPos = str.find(":",mPos+1);



    struct tm clock;
    clock.tm_hour = std::stoi(str.substr(0,mPos));
    clock.tm_min = std::stoi(str.substr(mPos+1,2));
    clock.tm_sec = std::stoi(str.substr(sPos+1,2));

    return clock;
}


/**
 ** Creates a timestruct from userprovided date.
 **
 ** @param string pre   Pretext to show before taking input
 ** @return struct tm
 */
struct tm userio::GetDate(std::string pre)
{
    // We want date in the format of YYYY-MM-DD
    std::string regex ("(20[0-1][0-4]|19[0-9][0-9])-(1[0-2]|0[0-9])-([0-2][0-9]|3[0-1])");
    std::string str = GetString(pre,regex,"YYYY-MM-DD");

    // Now, find the position of the two hyphens
    int mPos = str.find("-");
    int dPos = str.find("-",mPos+1);



    struct tm clock;
    clock.tm_year = std::stoi(str.substr(0,mPos));
    clock.tm_mon = std::stoi(str.substr(mPos+1,2));
    clock.tm_mday = std::stoi(str.substr(dPos+1,2));

    return clock;
}


/**
 ** Creates requests date and time from user, and returns timestamp
 **
 ** @param string pre   Pretext to show before taking input
 ** @return time_t
 */
time_t userio::GetTimestamp(std::string pre)
{
    std::cout << pre;
    struct tm date = GetDate("Enter date");
    struct tm clock = GetTime("Enter time");

    return MakeTimestamp(date,clock);
}

/**
 ** Merges time and date from two different tm structs
 **
 ** @param struct tm clock      Time struct
 ** @param struct tm date       Date struct
 ** @return time_t
 */

time_t userio::MakeTimestamp(struct tm clock, struct tm date)
{
    date.tm_hour = clock.tm_hour;
    date.tm_min = clock.tm_min;
    date.tm_sec = clock.tm_sec;

    time_t merged = mktime(&date);
    return merged;
}




/**
 ** Filters out the pretext from a pretext that may have
 ** come bundled with an error message. Pretexts should always be
 ** one-liners, so by checking for a newline and removing it,
 ** we can return the second part of the string (if a newline exists)
 ** or just return the string if a newline does not exist.
 **
 ** @param string pre
 ** @return string
 */
std::string userio::FilterPretext(std::string pre)
{
    unsigned nlPosition = pre.find("\n");

    if(nlPosition != std::string::npos)
    {
        // Position of \n, plus one, as to start after it.
        nlPosition += 1;

        // ... and return the remainder
        return pre.substr(nlPosition);
    }
    else
        return pre;
}

