#ifndef LOCALDB_H
#define LOCALDB_H
#include <string>
#include <curl/curl.h>
#include <pugixml.hpp> // Read and write XML
#include <sstream> // Write URL content to string
#include <list>
#include <map>
#include <iostream>
#include "list_tool.h"
#include "person.h"
#include "userio.h"
#include "prescription.h"

typedef std::map<std::string,person> People;

enum search_option
{
    // Search options for API search
    APPROX_MATCH,
    BY_ID,
    BY_NAME,
    STRENGTH,

    // Search options for drugs/prescriptions
    BY_DRUG,
    BY_DOCTOR,
    BY_PATIENT,
    BY_MONTH
};

enum{ DOCTOR, PATIENT };

typedef std::map<std::string,std::list<prescription>> PrescriptionList;

/**
 ** This class is used to map everything -- it's the local database
 ** Through this class, interaction with the stored data takes place.
 **
 ** @class localdb
 */

class localdb
{
    public:
        localdb();
        virtual ~localdb();

        MedList GetDrugs(const std::string search, const int search_type);
        medicine SelectDrug();

        // Add prescriptions / doctors / patients
        void AddPrescription();
        void AddPerson(int type);

        // List drugs / prescriptions / doctors / patients
        void ListPeople(int type);
        void ListDrugs();
        void ListPrescriptions(std::list<prescription> pr);

        // Maintenance (purge old, read XML file, write XML file)
        void PurgeOld(time_t dt);
        void ReadXML();
        void WriteXML();

        // Search for a prescription using different criteria
        std::list<prescription> GetPrescribed(std::string search,int how = BY_DRUG);

        // List of prescriptions (sorted by drug ID key)
        PrescriptionList* Prescriptions;




    protected:
    private:
        std::string db_file = "db.xml";
        std::string rxnorm_api_base_url = "http://rxnav.nlm.nih.gov/REST/";

        // cURL methods for API interaction
        std::string GetAPIResponse(const std::string url);
        CURLcode GetUrl(const std::string& url, std::ostream& stream, long timeout = 30);
        static size_t CurlOutput(void* buffer, size_t size, size_t nmemb, void* userp);

        // Store a prescription in our local database
        void StorePrescription(prescription& pres);

        // Return a list of prescriptions by different criteria
        std::list<prescription> GetPrescriptionsByPatientSSN(std::string search);
        std::list<prescription> GetPrescriptionsByDoctorFullName(std::string search);
        std::list<prescription> GetPrescriptionsByDrugID(std::string search);


        // List of prescribed drugs
        MedList* drugs;

        // Maps of available doctors and patients (with name as key for doctors, and SSN for patients)
        People* doctors;
        People* patients;

        // List to keep track of IDs in search results (so we can avoid calling the API twice
        //                                              for the same ID)
        List* verify_rxnorm_id;

};

#endif // LOCALDB_H
