#ifndef PERSON_H
#define PERSON_H
#include <string>

class person
{
    public:
        person();

        // Getters
        std::string GetFirstName() { return first_name; }
        std::string GetLastName() { return last_name; }
        std::string GetAddress() { return address; }
        std::string GetSSN() { return SSN; }
        std::string GetZip() { return zip; }
        std::string GetPhone() { return phone; }

        // Setters
        void SetFirstName(std::string val) { first_name = val; }
        void SetLastName(std::string val) { last_name = val; }
        void SetAddress(std::string val) { address = val; }
        void SetSSN(std::string val) { SSN = val; }
        void SetZip(std::string val) { zip = val; }
        void SetPhone(std::string val) { phone = val; }
    protected:
    private:
        std::string first_name;
        std::string last_name;
        std::string address;
        std::string SSN;
        std::string zip;
        std::string phone;
};

#endif // PERSON_H
