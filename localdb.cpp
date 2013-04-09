#include "localdb.h"


localdb::localdb()
{
    verify_rxnorm_id = new List(Sorted);
    doctors = new People;
    patients = new People;
}

localdb::~localdb()
{
    //dtor
}



/**
 ** Searches rxnorm's medical database for the substance defined by
 ** the search parameter. Either by ID, name or estimated name match
 ** as defined by the search_type parameter.
 **
 ** @param const string search          Contains the search string
 ** @param const integer search_type    How to search the API
 ** @return integer
 **/

MedList localdb::GetDrugs(const std::string search, const int search_type)
{
    // This will be the URL we'll append our search to
    std::string url = rxnorm_api_base_url;

    // ... and this will be the string to contain all information we fetched
    std::string raw_response;

    // This will be our xml document, and our first node respectively
    pugi::xml_document response;
    pugi::xml_node node;

    // Medicine list to contain all data when we're done :)
    MedList search_results;

    // Switch on the search_type to set the API URL accordingly
    switch(search_type)
    {
    case APPROX_MATCH:
        url += "approx?term=";
        break;
    case STRENGTH:
    case BY_ID:
        url += "rxcui/";
        break;
    default:
    case BY_NAME:
        url += "drugs?name=";
        break;
    }

    // Append our search string to the newly adjusted URL
    url += search;

    // If we're searching for the strength of a certain ID, we'll need to append /strength as well.
    if(search_type == STRENGTH)
        url += "/strength";

    // Store our API XML response in raw_response, then turn that into a usable
    // XML document
    raw_response = GetAPIResponse(url);

    // We need to do this to be able to actually use "response" as an XML document
    pugi::xml_parse_result result = response.load(raw_response.c_str());

    // We know what the results should look like, so we start by setting our "base node"
    // to the second child in the hierarchy, and define a basic medicine struct for later use
    node = response.first_child().first_child();
    medicine substance;

    // Switch on search type again -- this time to pick the right information from our
    // now usable XML response document.
    switch(search_type)
    {
        // For the APPROX_MATCH, we search the database "approximately",
        // and the information we need is stored in <candidate></candidate> nodes
        // found under <rxnormdata><approxGroup>
    case APPROX_MATCH:
        node = response.child("rxnormdata").child("approxGroup").child("candidate");

        // Now iterate through all sibling nodes, to get all <candidate>'s
        for(pugi::xml_node current = node; current; current = current.next_sibling())
        {
            // For every node that has an ID, we need more information. So with the ID,
            // we call this method again and instead search by ID -- but only if
            // we haven't searched that ID before; we don't want to call the same
            // page multiple times.
            int rxcui = std::stoi(current.child("rxcui").child_value());

            // With the ID converted to an int, we check if it's in the List(tool)
            if(!verify_rxnorm_id->in_list(rxcui))
            {
                // First thing we do is push our ID into the list since it wasn't there...
                verify_rxnorm_id->add(new Num_element(rxcui));

                // Then we fetch the information for each ID to get more specific info
                MedList tmplist = GetDrugs(current.child("rxcui").child_value(),BY_ID);

                // We set up a temporary medicine struct, and copy it to the single
                // element that was returned by searching for the ID.
                medicine* med = new medicine;
                medicine tmp = tmplist.back();

                med->rxnorm_id = tmp.rxnorm_id;
                med->market_name = tmp.market_name;
                std::cout << "tmplist has a market name of " << med->market_name << "\n";

                // ... with that done, throw a copy of its values onto our search_results STL list, and
                // delete our temporary struct.
                search_results.push_back(*med);
                delete med;
            }
        }
        break;
        // When searching by ID, we know we'll only get one result -- anything else is a superfluous;
        // and that's all we need.
    case BY_ID:
        {

        substance.rxnorm_id = node.child("rxnormId").child_value();
        substance.market_name = node.child("name").child_value();

        // Make another call and get the strength attribute!
        MedList tmplist = GetDrugs(substance.rxnorm_id,STRENGTH);
        substance.strength = (tmplist.back()).strength;

        search_results.push_back(substance);
        break;
        }
        // When searching for strength by the ID, we just want the strength attribute (and keep the ID of course)
    case STRENGTH:
        // We know we find it under <rxnormdata><strengthGroup>, so let's set our base node to that.
        node = response.child("rxnormdata").child("strengthGroup");
        substance.strength = node.child("strength").child_value();
        substance.rxnorm_id = node.child("rxcui").child_value();

        // This may seem useless, but will be called in conjunction with BY_ID and BY_NAME for a more
        // complete overview. Push it to the back of our list!
        search_results.push_back(substance);
        break;

        // This is by far the most useful of these cases. We get *a lot* more information when searching
        // by name. Unfortunately, the names returned by searching by ID is not useful for searching for
        // name, which is why we're not calling BY_NAME from the BY_ID case. :(
    default:
    case BY_NAME:
        // Set our base node to <rxnormdata><drugGroup><conceptGroup>, because that's where we find our info.
        node = response.child("rxnormdata").child("drugGroup").child("conceptGroup").next_sibling();

        // Loop through all siblings, pick out the information and throw a copy of our medicine struct onto
        // the back of our search_results list
        for(pugi::xml_node current = node.first_child(); current; current = current.next_sibling())
        {
            medicine* substance = new medicine;
            substance->name = current.child("name").child_value();
            substance->market_name = current.child("synonym").child_value();
            substance->rxnorm_id = current.child("rxcui").child_value();

            // ... before we push it on the back of our list, we need to get the strength attribute.
            MedList tmplist = GetDrugs(substance->rxnorm_id,STRENGTH);
            substance->strength = (tmplist.back()).strength;

            // ... there we go, now we can continue. :)
            search_results.push_back(*substance);
            delete substance;
        }
        break;
    }

    // .... aaaaand we're done. Return our list of search results!
    return search_results;
}

/**
 ** Fetches the URL's content to a string and returns the string
 **
 **
 ** @param const string url         The URL to fetch
 ** @return std::string
 */
std::string localdb::GetAPIResponse(const std::string url)
{
    curl_global_init(CURL_GLOBAL_ALL);

    // This stringstream will be responsible for turning
    // our fetched data into a string (which will be used for XML document creation)
    std::stringstream stream;
    std::string p;


    if(CURLE_OK == GetUrl(url,stream))
    {
        // Load contents from stringstream into our string
        p = stream.str();
    }

    curl_global_cleanup();

    return p;
}


/**
 ** Uses cURL to fetch a URL, and outputs it to the provided stream
 **
 ** @param const string &url        Reference to the string with the URL to fetch
 ** @param ostream &stream          Reference to the stream to output to (filestream, iostream, stringstream..)
 ** @param long timeout             Timeout in seconds for the URL fetching attempt
 ** @return CURLcode
 **/
CURLcode localdb::GetUrl(const std::string& url, std::ostream& stream, long timeout)
{
    std::cout << "Calling URL: " << url;
    CURLcode returned_code(CURLE_FAILED_INIT);
    CURL* curl = curl_easy_init();

    if(curl)
	{
		if(CURLE_OK == (returned_code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlOutput))
		&& CURLE_OK == (returned_code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
		&& CURLE_OK == (returned_code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
		&& CURLE_OK == (returned_code = curl_easy_setopt(curl, CURLOPT_FILE, &stream))
		&& CURLE_OK == (returned_code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
		&& CURLE_OK == (returned_code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())))
		{
			returned_code = curl_easy_perform(curl);
		}
		std::cout << "Returned code was: " << returned_code;
		curl_easy_cleanup(curl);
	}
	return returned_code;
}

/**
 ** Handles cURLs writing to its provided outputstream (if userp is provided)
 ** This is a pretty standard cURL callback method, and will be called by cURL.
 **
 ** @param void* buffer         Pointer to cURL's data
 ** @param size_t size          Multiplier for nmemb
 ** @param size_t nmemb         Size of the buffer
 ** @param void* userp          Stream to write to
 ** @return length              Returns amount of bytes written
 **
 */
size_t localdb::CurlOutput(void* buffer, size_t size, size_t nmemb, void* userp)
{
	if(userp)
	{
		std::ostream& os = *static_cast<std::ostream*>(userp);
		std::streamsize length = size * nmemb;
		if(os.write(static_cast<char*>(buffer), length))
			return length;
	}

	return 0;
}

/**
 ** Requests information from the user to add a new prescription.
 ** Also adds new doctors / patients if these don't already exist.
 **
 **
 */
void localdb::AddPrescription()
{
    // Set up dr_name and pt_ssn (doctor and patient) and get users input for doctor (we'll do patients soon..)
    std::string dr_name = userio::GetString("Name of doctor (or type 'list' to list all available): ", userio::NOFILTER);
    std::string pt_ssn;

    // Force it lowercase! ugh ... high overhead here, iterating through all of it...
    std::transform(dr_name.begin(),dr_name.end(), dr_name.begin(), ::tolower);

    // Alright, so if we got "list", we list all doctors and restart this method...
    if(dr_name.compare("list")==0)
    {
        ListPeople(DOCTOR);
        AddPrescription();
        return;
    }
    else if(!doctors->count(dr_name))
    {
        std::cout << "\nDoctor was not found in the database -- I will request some information needed for creation.\n";
        AddPerson(DOCTOR);
    }


    // Now we'll do the exact same thing for the patient -- so get the users input for SSN
    pt_ssn = userio::GetString("Patient's SSN (or type 'list' to list all patients)", "^[a-z1-9]*", "numbers only");


    // Alright, so if we got "list", we list all patients to let the user see available SSNs
    // then we ask once more ...
    if(pt_ssn.compare("list")==0)
    {
        ListPeople(PATIENT);
        pt_ssn = userio::GetString("Patient's SSN", "^[1-9]*", "numbers only");
    }

    // If we couldn't find the patient in our list, we'll assume the user wants to add a new one...
    if(!patients->count(pt_ssn))
    {
        std::cout << "\nPatient was not found in the database -- I will request some information needed for creation.\n";
        AddPerson(PATIENT);
    }

    // Now we should be able to find the doctor, patient, and drug the user wanted.
    person* dr = &doctors->find(dr_name)->second;
    person* patient = &patients->find(pt_ssn)->second;
    medicine* drug = new medicine;
    *drug = SelectDrug();

    // And with that, we can create a prescription ...
    prescription* p = new prescription(dr, patient, drug);

    // ... which we'll make sure to store
    StorePrescription(*p);
}

/**
 ** Requests information from the user to add a new
 ** person (DOCTOR or PATIENT as defined by type parameter)
 **
 ** @param int type         Determines if we add a doctor or a patient
 */
void localdb::AddPerson(int type)
{
    // Lets set up a temporary person
    person* pr = new person();

    // ... then ask the user to populate it
    pr->SetFirstName(userio::GetString("First name", "^[a-zA-Z]*","a-z"));
    pr->SetLastName(userio::GetString("Last name", "^[a-zA-Z]*","a-z"));
    pr->SetAddress(userio::GetString("Address", "^[a-zA-Z]*","a-z"));
    pr->SetZip(userio::GetString("Zip code", "^[1-9]*","numbers only"));
    pr->SetPhone(userio::GetString("Phone number", "^[1-9]*","numbers only"));
    pr->SetSSN(userio::GetString("Social Security Number", "^[1-9]*","numbers only"));

    // If this method was called with a DOCTOR type, we concatinate
    // the doctor's name to use as key in our doctors-map where we'll put it.
    if(type == DOCTOR)
    {
     std::string name = pr->GetFirstName() + " " + pr->GetLastName();
     std::transform(name.begin(),name.end(), name.begin(), ::tolower);
     doctors->insert(doctors->end(),std::pair<std::string,person>(name,*pr));
    }
    // else it's a patient, so we'll use the SSN as key for our patients-map!
    else
    {
        patients->insert(patients->end(),std::pair<std::string,person>(pr->GetSSN(),*pr));
    }

    // delete our person since we already put a copy in our list
    delete pr;
}

/**
 ** Lets the user select a drug by entering the drug ID
 ** The drug ID can be found online (as provided to the user should he enter 'search')
 ** or locally, if a previous prescription has been made for that drug (as provided to the user
 ** should he enter 'list').
 ** Returns the selected drug as a medicine struct
 **
 ** @return medicine
 **
 */
medicine localdb::SelectDrug()
{
    // Let's set up two strings -- drug_id and cmd. These are essentially the same, except
    // cmd will be forced to lowercase
    std::string drug_id,cmd;

    // We'll also need a temporary medical map for this. If the user searches, we'll store the results in it.
    MedMap temporary_drug_map;

    // Now, ask the user to provide a drug ID, or to enter 'search' or 'list'
    drug_id = cmd = userio::GetString("Enter drug ID (or type 'search' to search, or 'list' to list available)", userio::NOFILTER);

    // .. oh, and force the user's potential command to lowercase.
    std::transform(cmd.begin(),cmd.end(),cmd.begin(), ::tolower);


    // If the list command was entered, list all drugs and then recurse
    if(cmd.compare("list")==0)
    {
        ListDrugs();
        return SelectDrug();
    }
    // ... and if it was "search", take new input and let the user search the drug repositories
    else if(cmd.compare("search")==0)
    {
        // Request a search string
        drug_id = userio::GetString("Search",userio::NOFILTER);

        // Get the results from our search
        MedList result = GetDrugs(drug_id,BY_NAME);

        // Loop through our results, and show them to the user.
        for(MedList::iterator i = result.begin(); i != result.end(); i++)
        {
            // While we do this, we also put them in our temporary drug map with the drug's ID as key
            temporary_drug_map.insert(temporary_drug_map.begin(),std::pair<std::string,medicine>((*i).rxnorm_id,(*i)));
            std::cout << "\nName: " << (*i).name << "(marketed as: " << (*i).market_name
                      << "\nStrength: " << (*i).strength
                      << "\nDrug ID: " << (*i).rxnorm_id << "\n";
        }

        // Now ask the user one more time to enter an ID, or to type 'retry' if he wants to redo it
        drug_id = cmd = userio::GetString("Enter ID from above results (or type 'retry' if no match was found): ",userio::NOFILTER);

        // ... and once again, force cmd to lowercase
        std::transform(cmd.begin(),cmd.end(),cmd.begin(), ::tolower);

        while(cmd.compare("retry")!=0 && !temporary_drug_map.count(drug_id))
            drug_id = cmd = userio::GetString("Enter ID from above results (or type 'retry' if no match was found): ",userio::NOFILTER);

        // If we got 'retry' (or the ID doesn't exist), recurse by returning the value returned by calling this method again
        if(cmd.compare("retry")==0 || !temporary_drug_map.count(drug_id))
            return SelectDrug();
        // otherwise return the drug we should have found
        else
            return temporary_drug_map.find(drug_id)->second;
    }
    // ... and if we got a valid ID, just loop through our current medicines, and return it
    else
    {
        for(MedList::iterator i = drugs->begin(); i != drugs->end(); i++)
        {
            if((*i).rxnorm_id.compare(drug_id))
                return (*i);
        }

        // If we haven't returned a value after looping through our medicines, we'll just have to
        // call this method again, and print the error message that no drug was found with that ID
        std::cout << "\nNo drug with that ID found.";
        SelectDrug();
    }
    return SelectDrug();
}

/**
 ** Lists all people of type (DOCTOR or PATIENT)
 ** Lists all people if no type is specified.
 **
 ** @param int type     Indicates if we're listing doctors or patients
 **/
void localdb::ListPeople(int type)
{
    if(type == PATIENT)
    {
        for(People::iterator i = patients->begin(); i != patients->end(); i++)
        {
            std::cout << "Name: " << (*i).second.GetFirstName() << " " << (*i).second.GetLastName()
                      << "Address: " << (*i).second.GetAddress()
                      << "Phone: " << (*i).second.GetPhone()
                      << "SSN: " << (*i).second.GetSSN();
        }
    }
    else if(type == DOCTOR)
    {
        for(People::iterator i = doctors->begin(); i != doctors->end(); i++)
        {
            std::cout << "Name: " << (*i).second.GetFirstName() << " " << (*i).second.GetLastName()
                      << "Address: " << (*i).second.GetAddress()
                      << "Phone: " << (*i).second.GetPhone()
                      << "SSN: " << (*i).second.GetSSN();
        }
    }
    else
    {
        ListPeople(DOCTOR);
        ListPeople(PATIENT);
    }
}

/**
 ** Lists all drugs in our medical list
 **
 **
 */
void localdb::ListDrugs()
{
    // Loop through drugs and list the information on each
    for(MedList::iterator i = drugs->begin(); i != drugs->end(); i++)
    {
        std::cout << "\nName: " << (*i).name << "(marketed as: " << (*i).market_name
                      << "\nStrength: " << (*i).strength
                      << "\nDrug ID: " << (*i).rxnorm_id << "\n";
    }
}

/**
 ** Lists all prescriptions in a prescriptionlist
 **
 ** @param  list<prescription>
 **
 */
void localdb::ListPrescriptions(std::list<prescription> pr)
{
    if(pr.empty())
        return;

  // Loop through each of the prescriptions
    for(std::list<prescription>::iterator k = pr.begin(); k != pr.end(); k++)
    {
        std::cout << "\nDrug: \t" << (*k).GetDrug()->market_name << " [" << (*k).GetDrug()->name
                  << "\nStrength: \t" << (*k).GetDrug()->strength << "\n"
                  << "\nPrescribed to: " << (*k).GetPatient().GetFirstName() << " " << (*k).GetPatient().GetLastName()
                  << " (" << (*k).GetPatient().GetSSN() << ")\n"
                  << "\nPrescribed by: " << (*k).GetDoctor().GetFirstName() << " " << (*k).GetDoctor().GetLastName() << "\n";
    }
}

/**
 ** Takes a reference to a prescription and stores it
 ** in our prescription map, using the drugs ID as key.
 ** This method must be called whenever a new prescription
 ** has been created.
 **
 ** @param prescription& pres
 */
void localdb::StorePrescription(prescription& pres)
{
    // If there are no other prescriptions for this drug ...
    if(!Prescriptions->count(pres.GetDrug()->rxnorm_id))
    {
        // ... we'll have to create the list!
        std::list<prescription> pl;

        // Push this prescription on the back of this list
        pl.push_back(pres);

        // ... and insert the whole list under [drug_id]
        // (this is so that all prescriptions for a certain drug become easy to locate)
        Prescriptions->insert(std::pair<std::string,std::list<prescription>>(pres.GetDrug()->rxnorm_id,pl));
    }
    // If there's already at least one prescription for that drug, we just push it
    // onto its list -- found in Prescriptions[drug_id]
    else
        (Prescriptions->find(pres.GetDrug()->rxnorm_id)->second).push_back(pres);
}

/**
 ** This method just routes the call depending on the 'how' parameter.
 ** i.e if how == BY_DRUG it will return the returnvalue for GetPrescriptionByDrugID
 ** It then returns a list of prescriptions for that particular drug / doctor / patient
 **
 ** @param string           Search string (i.e doctors name, patient SSN, or drug ID)
 ** @param int how          What criteria to find prescriptions for (BY_DRUG, BY_NAME, BY_DOCTOR)
 ** @return list<prescription>
 */

std::list<prescription> localdb::GetPrescribed(std::string search, int how)
{
    switch(how)
    {
    case BY_DRUG:
        return GetPrescriptionsByDrugID(search);
    case BY_DOCTOR:
        return GetPrescriptionsByPatientSSN(search);
    default:
    case BY_NAME:
        return GetPrescriptionsByDoctorFullName(search);

    }
}


/**
 ** Returns a prescription list based on drug ID
 **
 ** @param string search        Drug ID to locate
 ** @return list<prescription>
 */
std::list<prescription> localdb::GetPrescriptionsByDrugID(std::string search)
{
    std::list<prescription> bare;

    if(Prescriptions->count(search)<1)
        return bare;
    // Fortunately, this is easy to find, since all prescriptions are already
    // sorted by lists found under Prescriptions[drug_id]
    return Prescriptions->find(search)->second;
}


/**
 ** This method finds all prescriptions made to a patient with a specific SSN
 ** and returns a list containing those.
 **
 ** @param string search        Patient's social security number
 ** @return list<prescription>
 */
std::list<prescription> localdb::GetPrescriptionsByPatientSSN(std::string search)
{
    // First, set up a temporary list which we'll populate
    std::list<prescription> res;


    // Then, loop through all medicines that have been prescribed
    for(PrescriptionList::iterator i = Prescriptions->begin(); i != Prescriptions->end(); i++)
    {
        // Now loop through each medicine, and check if the patient it was prescribed for was the one
        // we're looking for
        for(std::list<prescription>::iterator k = (*i).second.begin(); k != (*i).second.end(); k++)
        {
            // ... and if it is, push that to our temporary list
            if(search.compare((*k).GetPatient().GetSSN()) == 0)
                res.push_back(*k);
        }
    }

    // Chances are we might get an empty list back though :/
    return res;
}



/**
 ** This method finds all prescriptions made by a particular doctor
 ** and returns a list with those
 **
 ** @param string   search      Doctors full name
 ** @return list<prescription>
 */
std::list<prescription> localdb::GetPrescriptionsByDoctorFullName(std::string search)
{
    // Lets set up a temporary list
    std::list<prescription> res;

    // Force our search string to lowercase
    std::transform(search.begin(),search.end(),search.begin(), ::tolower);

    // Loop through our prescribed drugs
    for(PrescriptionList::iterator i = Prescriptions->begin(); i != Prescriptions->end(); i++)
    {
        // Loop through each of the prescribed drugs ...
        if((*i).second.size())
        {
            for(std::list<prescription>::iterator k = (*i).second.begin(); k != (*i).second.end(); k++)
            {
                // Merge the prescribing doctors first and last name and make it lowercase
                std::string dr_full_name = (*k).GetDoctor().GetFirstName() + " " + (*k).GetDoctor().GetLastName();
                std::transform(dr_full_name.begin(),dr_full_name.end(),dr_full_name.begin(), ::tolower);

                // Now check if the doctors full name in lowercase, equals our search string in lowercase
                // and push it on the back of our temporary list if it does
                if(search.compare(dr_full_name) == 0)
                    res.push_back(*k);
            }
        }
    }
    return res;
}

/**
 ** Purges old prescriptions
 **
 ** @param time_t dt        Timestamp-limit -- anything older than this will be purged
 **
 */
void localdb::PurgeOld(time_t dt)
{
    // Loop through all prescribed drugs, then loop through each prescription
    for(PrescriptionList::iterator i = Prescriptions->begin(); i != Prescriptions->end();)
    {
        for(std::list<prescription>::iterator k = (*i).second.begin(); k != (*i).second.end(); k++)
        {
            // Check if the prescription date is older than our timestamp limit
            if((*k).GetDate() < dt)
            {
                // If it is, set up a medicine
                medicine* tmp = (*k).GetDrug();

                // Erase it from the original list
                k = (*i).second.erase(k);

                // We use the temporary medicine object to check if this drug has
                // been prescribed to others. If this was the ONLY prescription, we have
                // to also remove this medicine from our medicine list.
                if(GetPrescriptionsByDrugID((*k).GetDrug()->rxnorm_id).size()==1)
                {
                    // Yep. Away with the list from Prescriptions
                    i = Prescriptions->erase(Prescriptions->find(tmp->rxnorm_id));

                    // ... and remove it from our medicine list as well.
                    for(MedList::iterator c = drugs->begin(); c != drugs->end();)
                    {
                        // Yup yup. Remove from medicine list!
                        if((*c).rxnorm_id.compare(tmp->rxnorm_id)==0)
                        {
                            // oh... and set our iterator to the proper value whilst we're at it
                            c = drugs->erase(c);
                        }
                        else
                            c++;
                    }
                }

                // Remove our temporary object -- dont want dangling pointers...
                delete tmp;

            }
            else
                k++;
        }
    }
}

/**
 ** Reads the content of our local XML database file
 ** and populates the MedLists and People lists accordingly
 **
 */
void localdb::ReadXML()
{
    // Get our database file
    pugi::xml_document database;

    // ... gotta do this to parse it
    pugi::xml_parse_result results = database.load_file(this->db_file.c_str());

    // If parsing failed, abort mission -- file is corrupt or non-existant!
    if(!results)
        return;

    // Set up our four base nodes -- medicine, doctors, patients and prescriptions.
    pugi::xml_node medNode = database.child("localdb").child("drugs").first_child();
    pugi::xml_node docNode = database.child("localdb").child("doctors").first_child();
    pugi::xml_node ptNode = database.child("localdb").child("patients").first_child();
    pugi::xml_node prescNode = database.child("localdb").child("prescriptions").first_child();

    // Now iterate the siblings of each of these nodes and set up our data

    // ... start with drugs (because they have no dependencies - pun intended)
    for(pugi::xml_node current = medNode; current; current = current.next_sibling())
    {
        // Set up a temporary medicinal object
        std::string key = current.attribute("key").value();
        medicine temporary_drug;

        // Check for the existance of each property, and add it to the temporary object
        if(current.child("rxnorm_id"))
            temporary_drug.rxnorm_id = current.child("rxnorm_id").child_value();
        if(current.child("name"))
            temporary_drug.name = current.child("name").child_value();
        if(current.child("market_name"))
            temporary_drug.market_name = current.child("market_name").child_value();
        if(current.child("strength"))
            temporary_drug.strength = current.child("strength").child_value();
        if(current.child("quantity"))
            temporary_drug.quantity = current.child("quantity").child_value();
        if(current.child("concept_id"))
            temporary_drug.quantity = current.child("concept_id").child_value();

        // Push it to the back of our drugs list
        drugs->push_back(temporary_drug);
    }

    // ... next up, lets do doctors and patients!
    for(pugi::xml_node current = docNode; current; current = current.next_sibling())
    {
        // Set up a temporary person, and get the key for this person
        std::string key = current.attribute("key").value();
        person temporary_person;

        // Check for the existance of each property, and add it to the temporary object
        if(current.child("first_name"))
            temporary_person.SetFirstName(current.child("first_name").child_value());
        if(current.child("last_name"))
            temporary_person.SetLastName(current.child("last_name").child_value());
        if(current.child("ssn"))
            temporary_person.SetSSN(current.child("ssn").child_value());
        if(current.child("address"))
            temporary_person.SetAddress(current.child("address").child_value());
        if(current.child("zip"))
            temporary_person.SetZip(current.child("zip").child_value());
        if(current.child("phone"))
            temporary_person.SetPhone(current.child("phone").child_value());

        // Push it to the back of our drugs list
        doctors->insert(std::pair<std::string,person>(key,temporary_person));
    }

    for(pugi::xml_node current = ptNode; current; current = current.next_sibling())
    {
        // Set up a temporary person, and get the key for this person
        std::string key = current.attribute("key").value();
        person temporary_person;

        // Check for the existance of each property, and add it to the temporary object
        if(current.child("first_name"))
            temporary_person.SetFirstName(current.child("first_name").child_value());
        if(current.child("last_name"))
            temporary_person.SetLastName(current.child("last_name").child_value());
        if(current.child("ssn"))
            temporary_person.SetSSN(current.child("ssn").child_value());
        if(current.child("address"))
            temporary_person.SetAddress(current.child("address").child_value());
        if(current.child("zip"))
            temporary_person.SetZip(current.child("zip").child_value());
        if(current.child("phone"))
            temporary_person.SetPhone(current.child("phone").child_value());

        // Push it to the back of our drugs list
        patients->insert(std::pair<std::string,person>(key,temporary_person));
    }


    // Now let's do prescriptions! This is a bit trickier, because prescriptions are just
    // a relation between a drug, a doctor and a patient. That's why we had those set up first!
    for(pugi::xml_node current = prescNode; current; current = current.next_sibling())
    {
        person* temporary_patient = &patients->find(current.child("patient").child_value())->second;
        person* temporary_doctor = &patients->find(current.child("doctor").child_value())->second;
        medicine* temporary_medicine = &GetDrugs(current.child("id").child_value(), BY_ID).front();
        time_t temporary_time = (time_t)std::stoi(current.child("time").child_value());

        StorePrescription(*new prescription(temporary_patient, temporary_doctor, temporary_medicine,temporary_time));
    }
}


/**
 ** Outputs all stored data to local XML database file
 ** Pretty much the same as the ReadXML() method, but reversed
 **
 */
void localdb::WriteXML()
{
    pugi::xml_document document;

    pugi::xml_node wrapper = document.append_child("localdb");
    pugi::xml_node drugs_category = wrapper.append_child("drugs");
    pugi::xml_node patients_category = wrapper.append_child("patients");
    pugi::xml_node doctors_category = wrapper.append_child("doctors");
    pugi::xml_node prescriptions_category = wrapper.append_child("prescriptions");

    if(drugs->size()>0)
    {
        for(MedList::iterator i = drugs->begin(); i != drugs->end(); i++)
        {
            // Create a <substance> </substance> group-node for each drug ...
            pugi::xml_node tmp_drug_node = drugs_category.append_child("substance");
            tmp_drug_node.append_attribute("key") = (*i).rxnorm_id.c_str();

            // Now set up our properties...
            pugi::xml_node tmp_id_node = tmp_drug_node.append_child("rxnorm_id");
            pugi::xml_node tmp_name_node = tmp_drug_node.append_child("name");
            pugi::xml_node tmp_market_name_node = tmp_drug_node.append_child("market_name");
            pugi::xml_node tmp_strength_node = tmp_drug_node.append_child("strength");
            pugi::xml_node tmp_qty_node = tmp_drug_node.append_child("quantity");
            pugi::xml_node tmp_concept_id_node = tmp_drug_node.append_child("concept_id");

            // ... set the value of each property to the value found in our medicine struct
            tmp_id_node.set_value((*i).rxnorm_id.c_str());
            tmp_name_node.set_value((*i).name.c_str());
            tmp_market_name_node.set_value((*i).market_name.c_str());
            tmp_strength_node.set_value((*i).strength.c_str());
            tmp_qty_node.set_value((*i).quantity.c_str());
            tmp_concept_id_node.set_value((*i).concept_id.c_str());
        }
    }

    // ... aaaand now we do the same with patients and doctors!

    if(patients->size()>0)
    {
        for(People::iterator c = patients->begin(); c != patients->end(); c++)
        {
            // Create a <person> </person> group-node for each drug ...
            pugi::xml_node tmp_patients_node = patients_category.append_child("person");
            tmp_patients_node.append_attribute("key") = (*c).second.GetSSN().c_str();

            // Now set up our properties...
            pugi::xml_node tmp_first_name_node = tmp_patients_node.append_child("first_name");
            pugi::xml_node tmp_last_name_node = tmp_patients_node.append_child("last_name");
            pugi::xml_node tmp_ssn_node = tmp_patients_node.append_child("ssn");
            pugi::xml_node tmp_address_node = tmp_patients_node.append_child("address");
            pugi::xml_node tmp_zip_node = tmp_patients_node.append_child("zip");
            pugi::xml_node tmp_phone_node = tmp_patients_node.append_child("phone");

            // ... set the value of each property to the value found in our patient object
            tmp_first_name_node.set_value((*c).second.GetFirstName().c_str());
            tmp_last_name_node.set_value((*c).second.GetLastName().c_str());
            tmp_ssn_node.set_value((*c).second.GetSSN().c_str());
            tmp_address_node.set_value((*c).second.GetAddress().c_str());
            tmp_zip_node.set_value((*c).second.GetZip().c_str());
            tmp_phone_node.set_value((*c).second.GetPhone().c_str());

        }
    }

    // and the same once more, for doctors ...
    if(doctors->size()>0)
    {
        for(People::iterator d = patients->begin(); d != patients->end(); d++)
        {
            // Create a <person> </person> group-node for each drug ...
            pugi::xml_node tmp_doctors_node = doctors_category.append_child("person");
            tmp_doctors_node.append_attribute("key") = (*d).second.GetSSN().c_str();

            // Now set up our properties...
            pugi::xml_node tmp_first_name_node = tmp_doctors_node.append_child("first_name");
            pugi::xml_node tmp_last_name_node = tmp_doctors_node.append_child("last_name");
            pugi::xml_node tmp_ssn_node = tmp_doctors_node.append_child("ssn");
            pugi::xml_node tmp_address_node = tmp_doctors_node.append_child("address");
            pugi::xml_node tmp_zip_node = tmp_doctors_node.append_child("zip");
            pugi::xml_node tmp_phone_node = tmp_doctors_node.append_child("phone");

            // ... set the value of each property to the value found in our doctors object
            tmp_first_name_node.set_value((*d).second.GetFirstName().c_str());
            tmp_last_name_node.set_value((*d).second.GetLastName().c_str());
            tmp_ssn_node.set_value((*d).second.GetSSN().c_str());
            tmp_address_node.set_value((*d).second.GetAddress().c_str());
            tmp_zip_node.set_value((*d).second.GetZip().c_str());
            tmp_phone_node.set_value((*d).second.GetPhone().c_str());

        }
    }

    // ... and now to the final part ... get the prescriptions -- and use keys to store them
    // since we dont want to double-save objects.

    if(Prescriptions->size()>0)
    {
        // Because prescriptions are stored in lists, stored in a map with the drugs ID as key,
        // we will first loop through all prescribed drugs, then each prescription for that drug.
        // This will cause all drugs to be grouped by ID in our XML file. :)
        for(PrescriptionList::iterator f = Prescriptions->begin(); f != Prescriptions->end(); f++)
        {
            for(std::list<prescription>::iterator g = (*f).second.begin(); g != (*f).second.end(); g++)
            {
                // Create a <prescription> </prescription> group-node for each prescription ...
                pugi::xml_node tmp_presc_node = prescriptions_category.append_child("prescription");

                // Get our doctors first and last name, separated by whitespaced and forced to lowercase
                std::string tmp_dr_name = (*g).GetDoctor().GetFirstName();
                tmp_dr_name += " ";
                tmp_dr_name += (*g).GetDoctor().GetLastName();

                std::transform(tmp_dr_name.begin(), tmp_dr_name.end(), tmp_dr_name.begin(), ::tolower);

                // Set the drug ID as key for this prescription group node
                tmp_presc_node.append_attribute("key") = (*g).GetDrug()->rxnorm_id.c_str();

                pugi::xml_node drug_id = tmp_presc_node.append_child("id");
                pugi::xml_node patient_key = tmp_presc_node.append_child("patient");
                pugi::xml_node doctor_key = tmp_presc_node.append_child("doctor");
                pugi::xml_node timestamp = tmp_presc_node.append_child("time");

                // Set values of our nodes...
                drug_id.set_value((*g).GetDrug()->rxnorm_id.c_str());
                patient_key.set_value((*g).GetPatient().GetSSN().c_str());
                doctor_key.set_value(tmp_dr_name.c_str());
                timestamp.set_value(std::to_string((int)(*g).GetDate()).c_str());
            }
        }
    }

    // and done! Save!
    std::cout << "\n\nSaving local XML database: " << document.save_file(db_file.c_str()) << "\n";
}
