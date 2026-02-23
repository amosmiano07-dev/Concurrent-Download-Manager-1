#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <cmath>

// A "lock" so threads don't write to the screen at the exact same time
std::mutex print_mutex;

// 1. The Write Callback (Same as before)
size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::ofstream* out = (std::ofstream*)stream;
    out->write((char*)ptr, size * nmemb);
    return size * nmemb;
}

// 2. The Worker Function
void download_chunk(int id, std::string url, long start, long end) {
    CURL* curl = curl_easy_init();
    std::string filename = "part_" + std::to_string(id);

    if(curl) {
        std::ofstream outfile(filename, std::ios::binary);
        
        // Create the range string: "0-26214399"
        std::string range = std::to_string(start) + "-" + std::to_string(end);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Safe printing using the lock
        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "Thread " << id << " STARTING: Range " << start << " to " << end << std::endl;
        }

        CURLcode res = curl_easy_perform(curl);

        {
            std::lock_guard<std::mutex> lock(print_mutex);
            if(res == CURLE_OK) {
                std::cout << "Thread " << id << " FINISHED!" << std::endl;
            } else {
                std::cerr << "Thread " << id << " FAILED: " << curl_easy_strerror(res) << std::endl;
            }
        }

        outfile.close();
        curl_easy_cleanup(curl);
    }
}

// 3. Helper to get total size (Same as Step 4)
double get_size(std::string url) {
    CURL* curl = curl_easy_init();
    double size = 0.0;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        if(curl_easy_perform(curl) == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);
        }
        curl_easy_cleanup(curl);
    }
    return size;
}

int main() {
    std::string url = "http://speedtest.tele2.net/100MB.zip";
    int num_threads = 4;

    long file_size = (long)get_size(url);
    
    std::cout << "Total File Size: " << file_size << " bytes" << std::endl;
    std::cout << "Splitting into " << num_threads << " threads..." << std::endl;

    long chunk_size = file_size / num_threads;
    std::vector<std::thread> workers;

    // Launch Threads
    for(int i = 0; i < num_threads; i++) {
        long start = i * chunk_size;
        // The last thread must take whatever is left until the very end
        long end = (i == num_threads - 1) ? file_size - 1 : (start + chunk_size - 1);
        
        workers.push_back(std::thread(download_chunk, i, url, start, end));
    }

    // Wait for everyone to finish
    for(auto& t : workers) {
        t.join();
    }

    std::cout << "All threads done! Check for part_0, part_1, etc." << std::endl;
    return 0;
}
