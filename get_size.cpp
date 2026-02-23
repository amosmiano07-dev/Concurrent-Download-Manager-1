#include <iostream>
#include <curl/curl.h>

// This function performs a HEAD request to get file size in bytes
double get_file_size(std::string url) {
    CURL* curl;
    CURLcode res;
    double file_size = 0.0;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // --- THE KEY SETTINGS ---
        // 1. NOBODY = 1 tells libcurl: "Don't download the body" (HEAD request)
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        // 2. HEADER = 0 tells libcurl: "Don't print headers to stdout"
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        // ------------------------

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        if(res == CURLE_OK) {
            // Ask libcurl to extract the Content-Length from the headers it just received
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &file_size);
            if(res != CURLE_OK) {
                std::cerr << "Failed to get size info." << std::endl;
            }
        } else {
            std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    return file_size;
}

int main() {
    // A 100MB test file from a speedtest server
    std::string url = "http://speedtest.tele2.net/100MB.zip";

    std::cout << "Checking file size for: " << url << std::endl;
    
    double size = get_file_size(url);

    if (size > 0) {
        std::cout << "File Size: " << size << " bytes" << std::endl;
        std::cout << "File Size: " << size / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Error: Could not determine file size." << std::endl;
    }

    return 0;
}
