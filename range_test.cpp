#include <iostream>
#include <fstream>
#include <curl/curl.h>

size_t write_data(void* ptr, size_t size, size_t nmemb, void* userdata) {
    std::ofstream* outfile = (std::ofstream*)userdata;
    outfile->write((char*)ptr, size * nmemb);
    return size * nmemb;
}

int main() {
    CURL* curl;
    CURLcode res;
    
    // We will use a text file this time so we can read the partial output
    std::string url = "https://raw.githubusercontent.com/git/git/master/README.md";
    std::string output_filename = "partial_readme.txt";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        std::ofstream outfile(output_filename, std::ios::binary);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
        
        // --- THIS IS THE MAGIC LINE ---
        // We ask for bytes 0 to 499 (The first 500 bytes)
        curl_easy_setopt(curl, CURLOPT_RANGE, "0-499");
        // ------------------------------

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        std::cout << "Requesting only the first 500 bytes..." << std::endl;
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            std::cerr << "Download failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Success! Check " << output_filename << std::endl;
        }

        outfile.close();
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}
