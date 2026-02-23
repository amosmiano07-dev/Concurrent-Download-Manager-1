#include <iostream>
#include <fstream>
#include <curl/curl.h>

// FIX 1: The function is now defined BEFORE main()
size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream) {
    size_t written = size * nmemb;
    std::ofstream* out = (std::ofstream*)stream;
    out->write((char*)ptr, written);
    return written;
}

int main() {
    // Initialize standard C++ file stream
    std::string filename = "simple_test.zip";
    std::ofstream outfile(filename, std::ios::binary);

    // Initialize CURL
    CURL* curl = curl_easy_init();
    std::string url = "http://speedtest.tele2.net/100MB.zip";

    if(curl) {
        std::cout << "Starting download..." << std::endl;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);

        if(res == CURLE_OK) {
            std::cout << "Download complete!" << std::endl;
        } else {
            std::cout << "Error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    
    outfile.close();
    return 0;
}
