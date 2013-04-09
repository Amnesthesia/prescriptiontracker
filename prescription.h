#ifndef PRESCRIPTION_H
#define PRESCRIPTION_H
#include "person.h"
#include <time.h>
#include <map>
#include <list>

enum
{
    DEPO,
    INSTANT
};



typedef struct{
    std::string name;
    std::string market_name;
    std::string rxnorm_id;
    std::string concept_id;
    std::string strength;
    std::string quantity;
} medicine;

typedef std::list<medicine> MedList;

typedef std::map<std::string,MedList[]> SortedMap;
typedef std::map<std::string,medicine> MedMap;

class prescription
{
    private:
        time_t date;
        struct tm * tmpDate;

        person* patient;
        person* doctor;
        medicine* drug;

    public:
        prescription();
        prescription(struct tm date);
        prescription(person* dr, person* ptnt, time_t dt = time(NULL));
        prescription(person* dr, person* ptnt, medicine* drg, time_t dt = time(NULL));
        virtual ~prescription(){ delete drug; delete patient; delete doctor; }

        time_t GetDate() { return date; }
        std::string GetDosage() { return drug->strength; }
        person& GetDoctor() { return *doctor; };
        person& GetPatient() { return *patient; };
        medicine* GetDrug() { return drug; }


        void SetHour(int h);
        void SetMinute(int m);
        void SetSecond(int s);

        void SetDay(int d);
        void SetMonth(int m);
        void SetYear(int y);

        void SetDate(time_t d);

        void SetTime(time_t date = time(NULL));


        void SetDrug(medicine* med){ drug = med; }
        void SetDoctor(person* dr){ doctor = dr; }
        void SetPatient(person* ptnt){ patient = ptnt; }

    protected:

};



#endif // PRESCRIPTION_H
