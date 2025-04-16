#include <common/json.hpp>
#include <fstream>

using json = nlohmann::json;


void writeToFile(char *path, json value){
    std::ofstream file;
    file.open(path);

    file << value.dump(1);
   
    file.close();
}
