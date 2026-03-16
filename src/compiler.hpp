#include <cstring>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

uint8_t count_words(const std::string& s) {
    if (s.empty()) return 0;
    uint8_t count = 1;
    for (char c : s) {
        if (c == '_') {
            count++;
            if (count > 3) {
                throw std::runtime_error("Entry exceeds max 3 words");
            }
        }
    }
    return count;
}

template<typename T>
void write_u(std::ofstream& fs, T value) {
    fs.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

void write_cstring(std::ofstream& fs, const std::string& s) {
    fs.write(s.c_str(), s.size() + 1); // always null-terminated
}

void compile_from_buffer(const char* output_path,
                         const char* data,
                         size_t size);

void compile_from_file(const char* output_path,
                       const char* json_path)
{
    std::ifstream file(json_path, std::ios::binary);
    if (!file) {
        return;
    }

    std::vector<char> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    compile_from_buffer(output_path, buffer.data(), buffer.size());
}

void compile_from_buffer(const char* output_path,
                         const char* data,
                         size_t size)
{
    std::ofstream fs(output_path,
                     std::ios::binary | std::ios::trunc);

    if (!fs) {
        return;
    }

    json j = json::parse(data, data + size);

    
    //metadata section 

    write_u(fs, (uint8_t)0xD0);
    auto metadata = j["metadata"];
    
    std::string from = metadata["from"];
     write_u(fs, (uint8_t)0xF0);
     write_u(fs, static_cast<uint8_t>(from.size()));   
     write_cstring(fs, from);      

     std::string to = metadata["to"];
     write_u(fs, (uint8_t)0xF1);
     write_u(fs, static_cast<uint8_t>(to.size()));                             
     write_cstring(fs, to); 
     
     bool isMultibyte = metadata["multibyte"];
     write_u(fs, (uint8_t)0xF2);                         
     write_u(fs, (isMultibyte == true ? 0x00 : 0x01));  

     
     //dictionary section
    write_u(fs, (uint8_t)0xD1);
    for (auto& item : j["dictionary"]) {

        std::string entry = item["entry"];
        std::string translation = item["translation"];
        uint8_t word_type = item["word_type"];
        auto flags = item["word_flags"];
        // entry
        write_u(fs, (uint8_t)0xF0);
        write_u(fs, static_cast<uint8_t>(entry.size()));   
        write_u(fs, count_words(entry));                   
        write_u(fs, (uint8_t)0x00);                       
        write_cstring(fs, entry);                          

        // translation
        write_u(fs, (uint8_t)0xF1);
        write_u(fs, static_cast<uint8_t>(translation.size())); 
        write_u(fs, (uint8_t)0x00);                          
        write_cstring(fs, translation);                         

        // type
        write_u(fs, (uint8_t)0xF2);
        write_u(fs, word_type);

        //flags_target

       write_u(fs, (uint8_t)0xF3); 
            if (flags.is_array()) {
                uint16_t count = flags.size();
                write_u(fs, (uint8_t)(count & 0xFF));
                write_u(fs, (uint8_t)((count >> 8) & 0xFF));
                
                for (const auto& flag : flags) {
                    write_u(fs, flag.get<uint8_t>());
                }
            } else {
                write_u(fs, (uint8_t)0x00);
                write_u(fs, (uint8_t)0x00);
            }    
    }

    // rules section
    write_u(fs, (uint8_t)0xD2);



if (j.contains("original") && j["original"].contains("verb_conjugations")) {
    write_u(fs, (uint8_t)0xD3);
    
    for (auto& verb_rule : j["original"]["verb_conjugations"]) {
        // Get endings array
        auto endings = verb_rule["endings"];
        
        // F0 marker with count of endings
        write_u(fs, (uint8_t)0xF0);
        write_u(fs, static_cast<uint8_t>(endings.size()));
        
        // Write each ending as null-terminated string
        for (const auto& ending : endings) {
            std::string ending_str = ending.get<std::string>();
            write_cstring(fs, ending_str);
        }
        
        // F1 marker - type (SUFFIX/PREFIX)
        write_u(fs, (uint8_t)0xF1);
        std::string type_str = verb_rule.value("type", "SUFFIX");
        uint8_t type_byte = (type_str == "PREFIX") ? 0x01 : 0x00;
        write_u(fs, type_byte);
        
        // F2 marker - form (combination of flags)
        write_u(fs, (uint8_t)0xF2);
        uint8_t form_byte = 0x00;
        
        // Check if form is an array or a string
        if (verb_rule.contains("form")) {
            if (verb_rule["form"].is_array()) {
                // Array of flags - combine them with OR
                for (const auto& flag : verb_rule["form"]) {
                    std::string flag_str = flag.get<std::string>();
                    if (flag_str == "PAST") form_byte |= 0x01;
                    else if (flag_str == "INFINITIVE") form_byte |= 0x02;
                    else if (flag_str == "PRESENT") form_byte |= 0x03;
                    else if (flag_str == "CONTINUOUS") form_byte |= 0x04;
                    else if (flag_str == "FEMININE" || flag_str == "FEMININE_V" || flag_str == "FEMININE_GENDER") form_byte |= 0x10;
                    else if (flag_str == "PLURAL" || flag_str == "PLURAL_V") form_byte |= 0x20;
                    else if (flag_str == "FIRST_PERSON") form_byte |= 0x40;
                    else if (flag_str == "SECOND_PERSON") form_byte |= 0x08;  // Example: bit 3
                    else if (flag_str == "THIRD_PERSON") form_byte |= 0x10;   // Example: bit 4
                    else if (flag_str == "INDICATIVE") form_byte |= 0x00;     // Add your actual bit values
                    else if (flag_str == "SIMPLE") form_byte |= 0x00;         // Add your actual bit values
                    else if (flag_str == "ACTIVE") form_byte |= 0x00;         // Add your actual bit values
                    else if (flag_str == "MASCULINE") form_byte |= 0x01;      // Add your actual bit values
                    else if (flag_str == "SINGULAR") form_byte |= 0x04;       // Add your actual bit values
                    else if (flag_str == "NEUTER" || flag_str == "NEUTRAL_GENDER") form_byte |= 0x80; // Example: bit 7
                }
            } else if (verb_rule["form"].is_string()) {
                // Single string (backward compatibility)
                std::string form_str = verb_rule["form"];
                if (form_str == "PAST") form_byte = 0x01;
                else if (form_str == "INFINITIVE") form_byte = 0x02;
                else if (form_str == "PRESENT") form_byte = 0x03;
                else if (form_str == "CONTINUOUS") form_byte = 0x04;
                else if (form_str == "FEMININE") form_byte = 0x10;
                else if (form_str == "PLURAL") form_byte = 0x20;
                else if (form_str == "FIRST_PERSON") form_byte = 0x40;
            }
        }
        
        write_u(fs, form_byte);
    }
}

if (j.contains("target") && j["target"].contains("verb_conjugations")) {
    write_u(fs, (uint8_t)0xD4);
    
    for (auto& verb_conj : j["target"]["verb_conjugations"]) {
       
        std::string required_ending = verb_conj.value("required_ending", "");
        std::string affix = verb_conj.value("affix", "");
        
        write_u(fs, (uint8_t)0xF0);
        write_u(fs, (uint8_t)0x02); 
        
        write_cstring(fs, required_ending);
        write_cstring(fs, affix);
        
        // F1 marker type (SUFFIX/PREFIX/OTHERS I HAVENT IMPLEMENTE YET)
        write_u(fs, (uint8_t)0xF1);
        std::string type_str = verb_conj.value("type", "SUFFIX");
        uint8_t type_byte = (type_str == "PREFIX") ? 0x01 : 0x00;
        write_u(fs, type_byte);
        
        // F2 marker form
        write_u(fs, (uint8_t)0xF2);
        uint8_t form_byte = 0x00;
        
        // Check if form is an array or a string
        if (verb_conj.contains("form")) {
            if (verb_conj["form"].is_array()) {
                // Array of flags 
                for (const auto& flag : verb_conj["form"]) {
                    std::string flag_str = flag.get<std::string>();
                    if (flag_str == "PAST") form_byte |= 0x01;
                    else if (flag_str == "INFINITIVE") form_byte |= 0x02;
                    else if (flag_str == "PRESENT") form_byte |= 0x03;
                    else if (flag_str == "CONTINUOUS") form_byte |= 0x04;
                    else if (flag_str == "FEMININE" || flag_str == "FEMININE_V" || flag_str == "FEMININE_GENDER") form_byte |= 0x10;
                    else if (flag_str == "PLURAL" || flag_str == "PLURAL_V") form_byte |= 0x20;
                    else if (flag_str == "FIRST_PERSON") form_byte |= 0x40;
                    else if (flag_str == "SECOND_PERSON") form_byte |= 0x08;
                    else if (flag_str == "THIRD_PERSON") form_byte |= 0x10;
                    else if (flag_str == "INDICATIVE") form_byte |= 0x00;
                    else if (flag_str == "SIMPLE") form_byte |= 0x00;
                    else if (flag_str == "ACTIVE") form_byte |= 0x00;
                    else if (flag_str == "MASCULINE") form_byte |= 0x01;
                    else if (flag_str == "SINGULAR") form_byte |= 0x04;
                    else if (flag_str == "NEUTER" || flag_str == "NEUTRAL_GENDER") form_byte |= 0x80;
                }
            } else if (verb_conj["form"].is_string()) {
                // Single string (I STARTED IT WRONG LOL)
                std::string form_str = verb_conj["form"];
                if (form_str == "PAST") form_byte = 0x01;
                else if (form_str == "INFINITIVE") form_byte = 0x02;
                else if (form_str == "PRESENT") form_byte = 0x03;
                else if (form_str == "CONTINUOUS") form_byte = 0x04;
            }
        }
        
        write_u(fs, form_byte);
    }
}
    // NORMALIZING rules section

    write_u(fs, (uint8_t)0xD5);
    for (auto& item : j["replace"]) {
        std::string original = item["original"];
        std::string target = item["target"];
        uint8_t where = item["where"];
        
        write_u(fs, (uint8_t)0xF0);
        write_u(fs, static_cast<uint8_t>(original.size()));                             
        write_cstring(fs, original);    

               
        write_u(fs, (uint8_t)0xF1);
        write_u(fs, static_cast<uint8_t>(target.size()));                             
        write_cstring(fs, target);  

        write_u(fs, (uint8_t)0xF2);
        write_u(fs, where);
                       
    }
    
    fs.close();
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        return 1;
    }

    compile_from_file(argv[1], argv[2]);
}