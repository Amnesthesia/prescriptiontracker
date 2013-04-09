#include "userio.h"
#include "person.h"
#include "prescription.h"
#include "localdb.h"
#include "list_tool.h"

using namespace std;

void PrintHelp();

int main()
{

    localdb* db = new localdb;

    PrintHelp();
    char cmd = userio::GetChar("\nAction", userio::NOFILTER);
    while(cmd != 'q')
    {
        switch(cmd)
        {
        case 'r':
            db->AddPrescription();
            break;
        case 'l':
            db->ListPrescriptions(db->GetPrescribed(userio::GetString("Doctor's full name", userio::NOFILTER)));
            break;
        case 'p':
            db->ListPrescriptions(db->GetPrescribed(userio::GetString("Patient's social security number", "[1-9]{11}","11 numbers only")));
            break;
        case 'f':
            db->PurgeOld(userio::GetTimestamp("Set a time and date before which to remove prescriptions"));
            break;
        case 'u':
            db->WriteXML();
            break;
        case 'i':
            break;
        case 'h':
            PrintHelp();
            break;
        }
        cmd = userio::GetChar("\nAction", userio::NOFILTER);
    }

    return 0;
}


void PrintHelp()
{
    std::cout << "\nCommands are as follows:\n\n"
              << "\tR \t\t Register a new prescription\n"
              << "\tL \t\t View prescriptions by a specific doctor\n"
              << "\tP \t\t View prescriptions by a specific patient\n"
              << "\tF \t\t Remove prescriptions older than a specific date \n"
              << "\tU \t\t Write to database file\n"
              << "\tI \t\t Read database from file\n\n";
}
