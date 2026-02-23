#include <iostream>
#include <curl/curl.h>

int main() {
    
    curl_global_init(CURL_GLOBAL_ALL);

    
    CURL* curl = curl_easy_init();

    if(curl) {
        
        std::cout << "SUCCESS: Libcurl is installed and initialized correctly!" << std::endl;
        
        
        curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
        std::cout << "Libcurl Version: " << info->version << std::endl;
        std::cout << "SSL Version: " << info->ssl_version << std::endl; 

        
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "ERROR: Failed to initialize libcurl." << std::endl;
        return 1;
    }

    
    curl_global_cleanup();
    
    return 0;
}
