#include <iostream>
#include <string>
#include <curl/curl.h>

// This function asks the server for the file size WITHOUT downloading the body
double get_file_size(std::string url) {
    CURL* curl = curl_easy_init();
    double size = 0.0;

    if(curl) {
        // 1. Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // 2. CRITICAL: Ask for HEADERS ONLY (No Body)
        // This sends a "HEAD" request instead of a "GET" request.
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        
        // 3. Output headers to nowhere (suppress printing them to screen)
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, +[](void*, size_t s, size_t n, void*){return s*n;});

        // 4. Perform the request
        CURLcode res = curl_easy_perform(curl);

        if(res == CURLE_OK) {
            // 5. Ask libcurl to extract the "Content-Length" from the response
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);
        } else {
            std::cerr << "Error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    return size;
}

int main() {
    // A standard test file (100MB) from Tele2
    std::string url = "http://speedtest.tele2.net/100MB.zip";

    std::cout << "Step 1: Connecting to server..." << std::endl;
    double file_size = get_file_size(url);

    if (file_size > 0) {
        std::cout << "Success!" << std::endl;
        std::cout << "Target URL: " << url << std::endl;
        std::cout << "Total Size: " << file_size << " bytes" << std::endl;
        std::cout << "In Megabytes: " << file_size / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Failed to get file size." << std::endl;
    }

    return 0;
}
