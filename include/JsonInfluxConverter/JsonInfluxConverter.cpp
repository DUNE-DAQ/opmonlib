#include "JsonInfluxConverter.h"

using json = nlohmann::json;

std::string JsonConverter::checkDataType(std::string line)
{
    std::string lineOriginal = line;
    line = line.substr(line.find("=") + 1);
    

    if ((line.find_first_not_of("0123456789") == std::string::npos) || line == "true" || line == "false")
    {
        lineOriginal;
    }
    else
    {
        lineOriginal = lineOriginal.substr(0, lineOriginal.find("=")+1);
        lineOriginal = lineOriginal+ '"' + line + '"';
    }

    return lineOriginal;
}



std::vector<std::string> JsonConverter::jsonToInfluxFunction(bool ignoreTags, std::vector<std::string> tagSetVector, std::string timeVariableName, json jsonStream)
{
    

    //flatten json, convert to string the json, then breaks the string into an array
    std::string jsonFlattenedString = jsonStream.flatten().dump();
    //remove first and last characters as they are {}
    jsonFlattenedString = jsonFlattenedString.substr(1, jsonFlattenedString.size() - 2);
    std::vector<std::string> vectorItems;
    std::stringstream data(jsonFlattenedString);
    int numOfCharacters;
    std::string applicationName;

    bool classdefined = false;

    while (std::getline(data, jsonFlattenedString, ','))
    {
        //Breaks the string to array
        jsonFlattenedString.erase(std::remove(jsonFlattenedString.begin(), jsonFlattenedString.end(), '"'), jsonFlattenedString.end());

        //Find delimiter between position and data and count its position
        std::string::size_type pos = jsonFlattenedString.find(":");
        int numOfCharacters = static_cast<int>(pos);

        //erase characters before separator
        jsonFlattenedString = jsonFlattenedString.substr(1);
        std::replace(jsonFlattenedString.begin(), jsonFlattenedString.end(), '/', '.');
        std::replace(jsonFlattenedString.begin(), jsonFlattenedString.end(), ':', '=');

        //Remove the class, saves it
        if (!classdefined) { applicationName = jsonFlattenedString.substr(0, jsonFlattenedString.find(".")); }
        jsonFlattenedString = jsonFlattenedString.substr(jsonFlattenedString.find(".") + 1);

        //add to vector
        vectorItems.push_back(jsonFlattenedString);

    }

    std::string insertCommandTag = "INSERT " + applicationName + ",";
    std::string insertCommandField = "";
    std::string time;

    // different member versions of find in the same order as above:
    std::size_t found;

    //list of insert commands
    std::vector<std::string> vectorInserts;

    //
    bool isTag = false;

    //Shaping to InfluxDB 
    for (int i = 0; i < vectorItems.size(); i++)
    {
        //is the time variable 
        found = vectorItems[i].find(timeVariableName);
        if (found == std::string::npos)
        {
            //if className should be ignored then do not add any tag that has this description
            if (ignoreTags)
            {
                for (int j = 0; j < tagSetVector.size(); j++)
                {
                    if (vectorItems[i].find(tagSetVector[j]) != std::string::npos)
                    {
                        isTag = true;
                    }
                }

                //if the tag is not in the ignore list
                if (!isTag)
                {
                    insertCommandField = insertCommandField + checkDataType(vectorItems[i]) + ",";
                }
                
                isTag = false;
            }
            else
            {
                //if labelled as the tag, goes in the tag variable
                for (int j = 0; j < tagSetVector.size(); j++)
                {
                    if (vectorItems[i].find(tagSetVector[j]) != std::string::npos)
                    {
                        isTag = true;
                    }
                }

                //if the tag is not in the ignore list
                if (!isTag)
                {
                    insertCommandField = insertCommandField + checkDataType(vectorItems[i]) + ",";
                }
                else
                {
                    insertCommandTag = insertCommandTag + checkDataType(vectorItems[i]) + ",";
                }
                isTag = false;

                
            }
            
        }
        else
        {
            time = vectorItems[i].substr(vectorItems[i].find("=") + 1);
            //remove the last character which is a ","
            insertCommandTag = insertCommandTag.substr(0, insertCommandTag.size() - 1);
            insertCommandField = insertCommandField.substr(0, insertCommandField.size() - 1);

            vectorInserts.push_back(insertCommandTag + " " + insertCommandField + " " + time);
            std::cout << insertCommandTag + " " + insertCommandField + " " + time << "\n";
            
            insertCommandTag = "INSERT " + applicationName + ",";
            insertCommandField = "";
        }

    }

    return vectorInserts;
}

